#include "headers/create_topic.h"
#include "../writer/headers/read_write_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

static char* build_topic_path(const char* topic_name, const char* base_path) {
    if (!topic_name || !base_path) {
        return NULL;
    }

    size_t base_len = strlen(base_path);
    size_t topic_len = strlen(topic_name);
    size_t total_len = base_len + topic_len + 10; // Extra space for separators and extension

    char* full_path = (char*)malloc(total_len);
    if (!full_path) {
        return NULL;
    }

    snprintf(full_path, total_len, "%s%s%s.topic", base_path, PATH_SEPARATOR, topic_name);
    return full_path;
}

static int ensure_directory_exists(const char* path) {
    if (!path) {
        return -1;
    }

    char* path_copy = strdup(path);
    if (!path_copy) {
        return -1;
    }

    char* p = path_copy;
    size_t len = strlen(path_copy);

    for (size_t i = 0; i < len; i++) {
        if (path_copy[i] == '/' || path_copy[i] == '\\') {
            char temp = path_copy[i];
            path_copy[i] = '\0';
            
            struct stat st = {0};
            if (stat(path_copy, &st) == -1) {
                if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                    free(path_copy);
                    return -1;
                }
            }
            
            path_copy[i] = temp;
        }
    }

    struct stat st = {0};
    if (stat(path_copy, &st) == -1) {
        if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
            free(path_copy);
            return -1;
        }
    }

    free(path_copy);
    return 0;
}

Topic* create_topic(const char* topic_name, const char* base_path) {
    if (!topic_name || !base_path || strlen(topic_name) == 0) {
        return NULL;
    }

    if (topic_exists(topic_name, base_path)) {
        return NULL;
    }

    if (ensure_directory_exists(base_path) != 0) {
        return NULL;
    }

    char* file_path = build_topic_path(topic_name, base_path);
    if (!file_path) {
        return NULL;
    }

    Chunk* chunk = chunk_init(file_path);
    if (!chunk) {
        free(file_path);
        return NULL;
    }

    const char* initial_data = "{}";
    if (chunk_append(chunk, initial_data, strlen(initial_data)) != 0) {
        chunk_free(chunk);
        free(file_path);
        return NULL;
    }

    if (chunk_save_delta(chunk) != 0) {
        chunk_free(chunk);
        free(file_path);
        return NULL;
    }

    Topic* topic = (Topic*)malloc(sizeof(Topic));
    if (!topic) {
        chunk_free(chunk);
        free(file_path);
        return NULL;
    }

    topic->topic_name = strdup(topic_name);
    if (!topic->topic_name) {
        chunk_free(chunk);
        free(file_path);
        free(topic);
        return NULL;
    }

    topic->file_path = file_path;
    topic->chunk_handle = chunk;

    return topic;
}

int topic_exists(const char* topic_name, const char* base_path) {
    if (!topic_name || !base_path) {
        return 0;
    }

    char* file_path = build_topic_path(topic_name, base_path);
    if (!file_path) {
        return 0;
    }

    FILE* file = fopen(file_path, "r");
    int exists = (file != NULL);
    
    if (file) {
        fclose(file);
    }
    
    free(file_path);
    return exists;
}

int delete_topic(Topic* topic) {
    if (!topic) {
        return -1;
    }

    if (topic->chunk_handle) {
        chunk_free((Chunk*)topic->chunk_handle);
        topic->chunk_handle = NULL;
    }

    if (topic->file_path) {
        if (remove(topic->file_path) != 0) {
            return -1;
        }
    }

    return 0;
}

void topic_free(Topic* topic) {
    if (!topic) {
        return;
    }

    if (topic->chunk_handle) {
        chunk_free((Chunk*)topic->chunk_handle);
    }

    if (topic->topic_name) {
        free(topic->topic_name);
    }

    if (topic->file_path) {
        free(topic->file_path);
    }

    free(topic);
}

