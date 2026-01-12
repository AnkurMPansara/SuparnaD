#include "headers/create_topic.h"
#include "response_builder.h"
#include "../../messaging/headers/create_topic.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_BODY_SIZE (10 * 1024 * 1024) // 10MB max body size
#define DEFAULT_TOPIC_BASE_PATH "./topics"
#define INITIAL_BUFFER_SIZE 4096

typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
} RequestBuffer;

static void free_request_buffer(void* cls) {
    RequestBuffer* buf = (RequestBuffer*)cls;
    if (buf) {
        if (buf->buffer) {
            free(buf->buffer);
        }
        free(buf);
    }
}

static int append_to_buffer(RequestBuffer* buf, const char* data, size_t data_size) {
    if (!buf || !data || data_size == 0) {
        return -1;
    }

    size_t new_size = buf->size + data_size;
    if (new_size > MAX_BODY_SIZE) {
        return -1;
    }

    if (new_size > buf->capacity) {
        size_t new_capacity = buf->capacity == 0 ? INITIAL_BUFFER_SIZE : buf->capacity * 2;
        while (new_capacity < new_size) {
            new_capacity *= 2;
        }

        char* new_buffer = realloc(buf->buffer, new_capacity);
        if (!new_buffer) {
            return -1;
        }

        buf->buffer = new_buffer;
        buf->capacity = new_capacity;
    }

    memcpy(buf->buffer + buf->size, data, data_size);
    buf->size = new_size;
    return 0;
}

static char* extract_json_string(const char* json, const char* key, size_t* out_len) {
    if (!json || !key || !out_len) {
        return NULL;
    }

    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char* key_pos = strstr(json, search_key);
    if (!key_pos) {
        return NULL;
    }

    const char* colon = strchr(key_pos, ':');
    if (!colon) {
        return NULL;
    }

    const char* start = colon + 1;
    while (*start && isspace(*start)) {
        start++;
    }

    if (*start != '"') {
        return NULL;
    }

    start++;
    const char* end = start;
    while (*end && *end != '"' && *end != '\0') {
        if (*end == '\\' && *(end + 1)) {
            end += 2;
        } else {
            end++;
        }
    }

    if (*end != '"') {
        return NULL;
    }

    size_t len = end - start;
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    memcpy(result, start, len);
    result[len] = '\0';
    *out_len = len;
    return result;
}

enum MHD_Result handle_create_topic_request(struct MHD_Connection *connection,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    if (!connection || !upload_data_size) {
        return MHD_NO;
    }

    if (*upload_data_size == 0) {
        if (*con_cls) {
            RequestBuffer* buf = (RequestBuffer*)*con_cls;
            if (buf && buf->buffer && buf->size > 0) {
                buf->buffer[buf->size] = '\0';

                size_t topic_len = 0;
                size_t base_path_len = 0;
                char* topic_name = extract_json_string(buf->buffer, "topic", &topic_len);
                char* base_path = extract_json_string(buf->buffer, "base_path", &base_path_len);

                if (!topic_name) {
                    const char* error_body = "{\"error\":\"Invalid request. Expected JSON with 'topic' field\"}";
                    struct MHD_Response* resp = build_response_from_buffer(400, error_body, strlen(error_body), "application/json");
                    if (resp) {
                        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, resp);
                        MHD_destroy_response(resp);
                        if (base_path) free(base_path);
                        free_request_buffer(*con_cls);
                        *con_cls = NULL;
                        return ret;
                    }
                    if (base_path) free(base_path);
                    free_request_buffer(*con_cls);
                    *con_cls = NULL;
                    return MHD_NO;
                }

                const char* path_to_use = base_path ? base_path : DEFAULT_TOPIC_BASE_PATH;

                if (topic_exists(topic_name, path_to_use)) {
                    const char* error_body = "{\"error\":\"Topic already exists\"}";
                    struct MHD_Response* resp = build_response_from_buffer(409, error_body, strlen(error_body), "application/json");
                    if (resp) {
                        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_CONFLICT, resp);
                        MHD_destroy_response(resp);
                        free(topic_name);
                        if (base_path) free(base_path);
                        free_request_buffer(*con_cls);
                        *con_cls = NULL;
                        return ret;
                    }
                    free(topic_name);
                    if (base_path) free(base_path);
                    free_request_buffer(*con_cls);
                    *con_cls = NULL;
                    return MHD_NO;
                }

                Topic* topic = create_topic(topic_name, path_to_use);
                
                if (topic) {
                    char success_body[512];
                    snprintf(success_body, sizeof(success_body), 
                            "{\"status\":\"success\",\"message\":\"Topic created successfully\",\"topic\":\"%s\",\"path\":\"%s\"}",
                            topic_name, topic->file_path);
                    struct MHD_Response* resp = build_response_from_buffer(200, success_body, strlen(success_body), "application/json");
                    if (resp) {
                        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);
                        MHD_destroy_response(resp);
                        topic_free(topic);
                        free(topic_name);
                        if (base_path) free(base_path);
                        free_request_buffer(*con_cls);
                        *con_cls = NULL;
                        return ret;
                    }
                    topic_free(topic);
                } else {
                    const char* error_body = "{\"error\":\"Failed to create topic\"}";
                    struct MHD_Response* resp = build_response_from_buffer(500, error_body, strlen(error_body), "application/json");
                    if (resp) {
                        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp);
                        MHD_destroy_response(resp);
                        free(topic_name);
                        if (base_path) free(base_path);
                        free_request_buffer(*con_cls);
                        *con_cls = NULL;
                        return ret;
                    }
                }

                free(topic_name);
                if (base_path) free(base_path);
                free_request_buffer(*con_cls);
                *con_cls = NULL;
                return MHD_NO;
            }
        }
        if (*con_cls) {
            free_request_buffer(*con_cls);
            *con_cls = NULL;
        }
        return MHD_NO;
    }

    if (!*con_cls) {
        RequestBuffer* buf = (RequestBuffer*)malloc(sizeof(RequestBuffer));
        if (!buf) {
            return MHD_NO;
        }
        buf->buffer = NULL;
        buf->size = 0;
        buf->capacity = 0;
        *con_cls = buf;
    }

    RequestBuffer* buf = (RequestBuffer*)*con_cls;
    if (append_to_buffer(buf, upload_data, *upload_data_size) != 0) {
        const char* error_body = "{\"error\":\"Request body too large\"}";
        struct MHD_Response* resp = build_response_from_buffer(413, error_body, strlen(error_body), "application/json");
        if (resp) {
            enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_REQUEST_ENTITY_TOO_LARGE, resp);
            MHD_destroy_response(resp);
            return ret;
        }
        return MHD_NO;
    }

    *upload_data_size = 0;
    return MHD_YES;
}

