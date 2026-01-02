#ifndef SERVER_EVENT_HANDLER_H
#define SERVER_EVENT_HANDLER_H

#include <stddef.h>
#include "../../messaging/headers/manage_groups.h"
#include "../../messaging/headers/create_topic.h"

typedef struct {
    int client_fd;
    Group* group;
    Topic* topic;
    char* group_id;
    char* topic_name;
} ClientSession;

int handle_client_session(int client_fd, GroupManager* group_manager, const char* default_topic_base_path);

int process_client_command(int client_fd, ClientSession* session, const char* command);

int handle_consume_command(int client_fd, ClientSession* session);

int handle_ack_command(int client_fd, ClientSession* session, const char* command_data);

void client_session_free(ClientSession* session);

#endif

