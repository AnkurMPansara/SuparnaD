#ifndef CONSUMER_HANDLER_H
#define CONSUMER_HANDLER_H

#include <stddef.h>
#include "../../messaging/headers/manage_groups.h"
#include "../../messaging/headers/create_topic.h"

int send_packet_to_consumer(int client_fd, const void* data, size_t data_size);

int handle_consumer_request(int client_fd, Group* group, Topic* topic);

int consume_and_send_packet(int client_fd, Group* group, Topic* topic);

int wait_and_acknowledge_packet(int client_fd, Group* group, Topic* topic, void* packet_data, size_t packet_size);

#endif

