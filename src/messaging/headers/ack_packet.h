#ifndef ACK_PACKET_H
#define ACK_PACKET_H

#include <stddef.h>
#include <stdint.h>
#include "manage_groups.h"
#include "consume_packet.h"

int ack_packet(Group* group, Packet* packet);

int ack_packet_by_size(Group* group, Topic* topic, size_t packet_size);

int ack_packet_by_offset(Group* group, Topic* topic, size_t offset, size_t packet_size);

int ack_packet_batch(Group* group, Topic* topic, size_t count);

int ack_packet_batch_by_size(Group* group, Topic* topic, size_t* packet_sizes, size_t count);

size_t get_acknowledged_bytes(Group* group);

#endif

