#include "headers/consumer_handler.h"
#include "send_message.h"
#include "handle_ack.h"
#include "../../messaging/headers/consume_packet.h"
#include "../../messaging/headers/ack_packet.h"
#include "../../messaging/headers/manage_groups.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

#define PACKET_HEADER_SIZE 8
#define MAX_PACKET_SIZE (10 * 1024 * 1024) // 10MB

static int send_packet_header(int client_fd, size_t packet_size) {
    if (client_fd < 0) {
        return -1;
    }

    uint32_t size = (uint32_t)packet_size;
    uint32_t magic = 0x5041434B; // "PACK" magic number

    uint8_t header[PACKET_HEADER_SIZE];
    memcpy(header, &magic, sizeof(uint32_t));
    memcpy(header + sizeof(uint32_t), &size, sizeof(uint32_t));

    ssize_t sent = send(client_fd, header, PACKET_HEADER_SIZE, 0);
    if (sent != PACKET_HEADER_SIZE) {
        perror("Failed to send packet header");
        return -1;
    }

    return 0;
}

int send_packet_to_consumer(int client_fd, const void* data, size_t data_size) {
    if (client_fd < 0 || !data || data_size == 0) {
        return -1;
    }

    if (data_size > MAX_PACKET_SIZE) {
        fprintf(stderr, "Packet size exceeds maximum\n");
        return -1;
    }

    if (send_packet_header(client_fd, data_size) != 0) {
        return -1;
    }

    size_t total_sent = 0;
    while (total_sent < data_size) {
        ssize_t sent = send(client_fd, (const char*)data + total_sent, data_size - total_sent, 0);
        if (sent < 0) {
            perror("Failed to send packet data");
            return -1;
        }
        if (sent == 0) {
            fprintf(stderr, "Connection closed while sending packet\n");
            return -1;
        }
        total_sent += sent;
    }

    printf("Sent packet to consumer: %zu bytes\n", data_size);
    return 0;
}

int consume_and_send_packet(int client_fd, Group* group, Topic* topic) {
    if (client_fd < 0 || !group || !topic) {
        return -1;
    }

    if (group->attached_topic != topic) {
        fprintf(stderr, "Group is not attached to the specified topic\n");
        return -1;
    }

    Packet* packet = consume_packet(group, topic);
    if (!packet) {
        return 0;
    }

    int result = send_packet_to_consumer(client_fd, packet->data, packet->data_size);
    
    if (result == 0) {
        int ack_result = wait_for_ack(client_fd);
        if (ack_result > 0) {
            if (ack_packet(group, packet) == 0) {
                printf("Packet acknowledged successfully\n");
            } else {
                fprintf(stderr, "Failed to acknowledge packet\n");
                result = -1;
            }
        } else {
            fprintf(stderr, "Consumer did not acknowledge packet\n");
            result = -1;
        }
    }

    packet_free(packet);
    return result;
}

int wait_and_acknowledge_packet(int client_fd, Group* group, Topic* topic, void* packet_data, size_t packet_size) {
    if (client_fd < 0 || !group || !topic) {
        return -1;
    }

    int ack_result = wait_for_ack(client_fd);
    if (ack_result > 0) {
        if (ack_packet_by_size(group, topic, packet_size) == 0) {
            printf("Packet acknowledged successfully\n");
            return 0;
        } else {
            fprintf(stderr, "Failed to acknowledge packet\n");
            return -1;
        }
    } else {
        fprintf(stderr, "Consumer did not acknowledge packet\n");
        return -1;
    }
}

int handle_consumer_request(int client_fd, Group* group, Topic* topic) {
    if (client_fd < 0 || !group || !topic) {
        return -1;
    }

    if (group->attached_topic != topic) {
        fprintf(stderr, "Group is not attached to the specified topic\n");
        return -1;
    }

    int packets_sent = 0;
    int max_packets = 100;
    int error_count = 0;
    const int max_errors = 3;

    while (packets_sent < max_packets && error_count < max_errors) {
        if (!group_has_more_data(group)) {
            printf("No more data available for group\n");
            break;
        }

        int result = consume_and_send_packet(client_fd, group, topic);
        
        if (result == 0) {
            packets_sent++;
            error_count = 0;
        } else if (result < 0) {
            error_count++;
            fprintf(stderr, "Error sending packet (error count: %d)\n", error_count);
            if (error_count >= max_errors) {
                fprintf(stderr, "Too many errors, stopping consumer handler\n");
                break;
            }
        } else {
            break;
        }
    }

    printf("Consumer handler completed. Sent %d packets\n", packets_sent);
    return packets_sent;
}

