#include "headers/server_event_handler.h"
#include "headers/consumer_handler.h"
#include "send_message.h"
#include "../../messaging/headers/consume_packet.h"
#include "../../messaging/headers/ack_packet.h"
#include "../../messaging/headers/create_topic.h"
#include "../../writer/headers/read_write_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096
#define COMMAND_CONSUME "CONSUME"
#define COMMAND_ACK "ACK"
#define COMMAND_SET_GROUP "SET_GROUP"
#define COMMAND_SET_TOPIC "SET_TOPIC"
#define DEFAULT_TOPIC_BASE_PATH "./topics"

static int read_command(int client_fd, char* buffer, size_t buffer_size) {
    if (client_fd < 0 || !buffer || buffer_size == 0) {
        return -1;
    }

    ssize_t bytes_read = recv(client_fd, buffer, buffer_size - 1, 0);
    if (bytes_read < 0) {
        perror("Failed to read command");
        return -1;
    }
    if (bytes_read == 0) {
        return 0;
    }

    buffer[bytes_read] = '\0';
    return (int)bytes_read;
}

static char* extract_json_value(const char* json, const char* key) {
    if (!json || !key) {
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
    while (*start && (*start == ' ' || *start == '\t')) {
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
    return result;
}

int handle_consume_command(int client_fd, ClientSession* session) {
    if (client_fd < 0 || !session) {
        return -1;
    }

    if (!session->group || !session->topic) {
        const char* error = "{\"error\":\"Group or topic not set. Use SET_GROUP and SET_TOPIC commands first\"}\n";
        send(client_fd, error, strlen(error), 0);
        return -1;
    }

    if (!group_has_more_data(session->group)) {
        const char* response = "{\"status\":\"no_data\",\"message\":\"No more data available\"}\n";
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    Packet* packet = consume_packet(session->group, session->topic);
    if (!packet) {
        const char* response = "{\"status\":\"no_packet\",\"message\":\"No packet available\"}\n";
        send(client_fd, response, strlen(response), 0);
        return 0;
    }

    int result = send_packet_to_consumer(client_fd, packet->data, packet->data_size);
    
    if (result == 0) {
        printf("Sent packet to client (size: %zu bytes)\n", packet->data_size);
    } else {
        fprintf(stderr, "Failed to send packet to client\n");
    }

    packet_free(packet);
    return result;
}

int handle_ack_command(int client_fd, ClientSession* session, const char* command_data) {
    if (client_fd < 0 || !session) {
        return -1;
    }

    if (!session->group || !session->topic) {
        const char* error = "{\"error\":\"Group or topic not set\"}\n";
        send(client_fd, error, strlen(error), 0);
        return -1;
    }

    if (command_data && strlen(command_data) > 0) {
        char* packet_size_str = extract_json_value(command_data, "packet_size");
        if (packet_size_str) {
            size_t packet_size = (size_t)atoi(packet_size_str);
            free(packet_size_str);
            
            if (ack_packet_by_size(session->group, session->topic, packet_size) == 0) {
                const char* response = "{\"status\":\"success\",\"message\":\"Packet acknowledged\"}\n";
                send(client_fd, response, strlen(response), 0);
                printf("Packet acknowledged by client (size: %zu)\n", packet_size);
                return 0;
            }
        }
    }

    if (ack_packet_batch(session->group, session->topic, 1) == 0) {
        const char* response = "{\"status\":\"success\",\"message\":\"Packet acknowledged\"}\n";
        send(client_fd, response, strlen(response), 0);
        printf("Packet acknowledged by client\n");
        return 0;
    }

    const char* error = "{\"error\":\"Failed to acknowledge packet\"}\n";
    send(client_fd, error, strlen(error), 0);
    return -1;
}

int process_client_command(int client_fd, ClientSession* session, const char* command) {
    if (client_fd < 0 || !session || !command) {
        return -1;
    }

    if (strncmp(command, COMMAND_CONSUME, strlen(COMMAND_CONSUME)) == 0) {
        return handle_consume_command(client_fd, session);
    } else if (strncmp(command, COMMAND_ACK, strlen(COMMAND_ACK)) == 0) {
        const char* ack_data = strchr(command, '\n');
        if (ack_data) {
            ack_data++;
        } else {
            ack_data = command + strlen(COMMAND_ACK);
            while (*ack_data && (*ack_data == ' ' || *ack_data == '\t')) {
                ack_data++;
            }
        }
        return handle_ack_command(client_fd, session, ack_data);
    } else if (strncmp(command, COMMAND_SET_GROUP, strlen(COMMAND_SET_GROUP)) == 0) {
        const char* group_name = command + strlen(COMMAND_SET_GROUP);
        while (*group_name && (*group_name == ' ' || *group_name == '\t' || *group_name == '\n')) {
            group_name++;
        }
        
        if (session->group_id) {
            free(session->group_id);
        }
        session->group_id = strdup(group_name);
        
        if (session->topic) {
            if (session->group) {
                group_free(session->group);
            }
            session->group = create_group(session->group_id, session->topic);
        }
        
        const char* response = "{\"status\":\"success\",\"message\":\"Group set\"}\n";
        send(client_fd, response, strlen(response), 0);
        return 0;
    } else if (strncmp(command, COMMAND_SET_TOPIC, strlen(COMMAND_SET_TOPIC)) == 0) {
        const char* topic_name = command + strlen(COMMAND_SET_TOPIC);
        while (*topic_name && (*topic_name == ' ' || *topic_name == '\t' || *topic_name == '\n')) {
            topic_name++;
        }
        
        if (session->topic_name) {
            free(session->topic_name);
        }
        session->topic_name = strdup(topic_name);
        
        if (topic_exists(session->topic_name, DEFAULT_TOPIC_BASE_PATH)) {
            if (session->topic) {
                topic_free(session->topic);
            }
            
            session->topic = (Topic*)malloc(sizeof(Topic));
            if (session->topic) {
                session->topic->topic_name = strdup(session->topic_name);
                char file_path[512];
                snprintf(file_path, sizeof(file_path), "%s/%s.topic", DEFAULT_TOPIC_BASE_PATH, session->topic_name);
                session->topic->file_path = strdup(file_path);
                session->topic->chunk_handle = chunk_init(file_path);
                if (session->topic->chunk_handle) {
                    chunk_load(session->topic->chunk_handle);
                }
            }
        } else {
            session->topic = create_topic(session->topic_name, DEFAULT_TOPIC_BASE_PATH);
        }
        
        if (session->topic && session->group_id) {
            if (session->group) {
                group_free(session->group);
            }
            session->group = create_group(session->group_id, session->topic);
        }
        
        const char* response = "{\"status\":\"success\",\"message\":\"Topic set\"}\n";
        send(client_fd, response, strlen(response), 0);
        return 0;
    } else {
        const char* error = "{\"error\":\"Unknown command\"}\n";
        send(client_fd, error, strlen(error), 0);
        return -1;
    }
}

void client_session_free(ClientSession* session) {
    if (!session) {
        return;
    }

    if (session->group) {
        group_free(session->group);
    }

    if (session->topic) {
        topic_free(session->topic);
    }

    if (session->group_id) {
        free(session->group_id);
    }

    if (session->topic_name) {
        free(session->topic_name);
    }

    free(session);
}

int handle_client_session(int client_fd, GroupManager* group_manager, const char* default_topic_base_path) {
    if (client_fd < 0) {
        return -1;
    }

    ClientSession* session = (ClientSession*)calloc(1, sizeof(ClientSession));
    if (!session) {
        return -1;
    }

    session->client_fd = client_fd;
    (void)group_manager;
    (void)default_topic_base_path;

    char buffer[BUFFER_SIZE];
    int keep_alive = 1;

    printf("Client session started (fd: %d)\n", client_fd);

    while (keep_alive) {
        int bytes_read = read_command(client_fd, buffer, BUFFER_SIZE);
        
        if (bytes_read <= 0) {
            printf("Client disconnected or read error\n");
            break;
        }

        buffer[bytes_read] = '\0';
        
        char* command = buffer;
        while (*command && (*command == ' ' || *command == '\t' || *command == '\n' || *command == '\r')) {
            command++;
        }

        if (strlen(command) == 0) {
            continue;
        }

        if (strncmp(command, "QUIT", 4) == 0 || strncmp(command, "EXIT", 4) == 0) {
            const char* response = "{\"status\":\"goodbye\"}\n";
            send(client_fd, response, strlen(response), 0);
            keep_alive = 0;
            break;
        }

        int result = process_client_command(client_fd, session, command);
        if (result < 0) {
            fprintf(stderr, "Error processing command\n");
        }
    }

    printf("Client session ended (fd: %d)\n", client_fd);
    client_session_free(session);
    close(client_fd);
    return 0;
}

