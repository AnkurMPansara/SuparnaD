#ifndef CONSUME_PACKET_H
#define CONSUME_PACKET_H

#include <stddef.h>
#include <stdint.h>
#include "manage_groups.h"

typedef struct {
    uint32_t packet_size;
    uint8_t* data;
    size_t data_size;
    size_t offset_in_topic;
} Packet;

Packet* consume_packet(Group* group, Topic* topic);

Packet* consume_packet_with_size(Group* group, Topic* topic, size_t packet_size);

int peek_packet_size(Group* group, Topic* topic, uint32_t* packet_size);

void packet_free(Packet* packet);

#endif

