#include "headers/consume_packet.h"
#include "../writer/headers/read_write_data.h"
#include <stdlib.h>
#include <string.h>

#define PACKET_SIZE_HEADER_SIZE sizeof(uint32_t)
#define MAX_PACKET_SIZE (10 * 1024 * 1024) // 10MB max packet size

static int read_packet_size_header(Group* group, Topic* topic, uint32_t* size_out) {
    if (!group || !topic || !size_out) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    if (group->read_pointer + PACKET_SIZE_HEADER_SIZE > chunk->size) {
        return -1;
    }

    uint32_t size_buffer;
    long bytes_read = chunk_read(chunk, &size_buffer, PACKET_SIZE_HEADER_SIZE, group->read_pointer);
    
    if (bytes_read != PACKET_SIZE_HEADER_SIZE) {
        return -1;
    }

    *size_out = size_buffer;
    return 0;
}

int peek_packet_size(Group* group, Topic* topic, uint32_t* packet_size) {
    if (!group || !topic || !packet_size) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    if (group->read_pointer + PACKET_SIZE_HEADER_SIZE > chunk->size) {
        return -1;
    }

    uint32_t size_buffer;
    long bytes_read = chunk_read(chunk, &size_buffer, PACKET_SIZE_HEADER_SIZE, group->read_pointer);
    
    if (bytes_read != PACKET_SIZE_HEADER_SIZE) {
        return -1;
    }

    *packet_size = size_buffer;
    return 0;
}

Packet* consume_packet(Group* group, Topic* topic) {
    if (!group || !topic) {
        return NULL;
    }

    if (group->attached_topic != topic) {
        return NULL;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return NULL;
    }

    if (chunk_load(chunk) != 0) {
        return NULL;
    }

    if (group->read_pointer >= chunk->size) {
        return NULL;
    }

    uint32_t packet_size;
    if (read_packet_size_header(group, topic, &packet_size) != 0) {
        return NULL;
    }

    if (packet_size == 0 || packet_size > MAX_PACKET_SIZE) {
        return NULL;
    }

    if (group->read_pointer + PACKET_SIZE_HEADER_SIZE + packet_size > chunk->size) {
        return NULL;
    }

    Packet* packet = (Packet*)malloc(sizeof(Packet));
    if (!packet) {
        return NULL;
    }

    packet->packet_size = packet_size;
    packet->offset_in_topic = group->read_pointer;
    packet->data = (uint8_t*)malloc(packet_size);
    
    if (!packet->data) {
        free(packet);
        return NULL;
    }

    size_t read_offset = group->read_pointer + PACKET_SIZE_HEADER_SIZE;
    long bytes_read = chunk_read(chunk, packet->data, packet_size, read_offset);
    
    if (bytes_read != (long)packet_size) {
        free(packet->data);
        free(packet);
        return NULL;
    }

    packet->data_size = packet_size;

    group->read_pointer += PACKET_SIZE_HEADER_SIZE + packet_size;
    group->last_read_size = PACKET_SIZE_HEADER_SIZE + packet_size;

    return packet;
}

Packet* consume_packet_with_size(Group* group, Topic* topic, size_t packet_size) {
    if (!group || !topic || packet_size == 0 || packet_size > MAX_PACKET_SIZE) {
        return NULL;
    }

    if (group->attached_topic != topic) {
        return NULL;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return NULL;
    }

    if (chunk_load(chunk) != 0) {
        return NULL;
    }

    if (group->read_pointer + packet_size > chunk->size) {
        return NULL;
    }

    Packet* packet = (Packet*)malloc(sizeof(Packet));
    if (!packet) {
        return NULL;
    }

    packet->packet_size = (uint32_t)packet_size;
    packet->offset_in_topic = group->read_pointer;
    packet->data = (uint8_t*)malloc(packet_size);
    
    if (!packet->data) {
        free(packet);
        return NULL;
    }

    long bytes_read = chunk_read(chunk, packet->data, packet_size, group->read_pointer);
    
    if (bytes_read != (long)packet_size) {
        free(packet->data);
        free(packet);
        return NULL;
    }

    packet->data_size = packet_size;

    group->read_pointer += packet_size;
    group->last_read_size = packet_size;

    return packet;
}

void packet_free(Packet* packet) {
    if (!packet) {
        return;
    }

    if (packet->data) {
        free(packet->data);
    }

    free(packet);
}