#include "headers/ack_packet.h"
#include "../writer/headers/read_write_data.h"
#include <stdlib.h>
#include <string.h>

#define PACKET_SIZE_HEADER_SIZE sizeof(uint32_t)

int ack_packet(Group* group, Packet* packet) {
    if (!group || !packet) {
        return -1;
    }

    if (!group->attached_topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)group->attached_topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    size_t total_packet_size = PACKET_SIZE_HEADER_SIZE + packet->packet_size;
    
    if (packet->offset_in_topic + total_packet_size > chunk->size) {
        return -1;
    }

    if (group->read_pointer < packet->offset_in_topic + total_packet_size) {
        group->read_pointer = packet->offset_in_topic + total_packet_size;
    }

    return 0;
}

int ack_packet_by_size(Group* group, Topic* topic, size_t packet_size) {
    if (!group || !topic || packet_size == 0) {
        return -1;
    }

    if (group->attached_topic != topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    if (group->read_pointer + PACKET_SIZE_HEADER_SIZE + packet_size > chunk->size) {
        return -1;
    }

    group->read_pointer += PACKET_SIZE_HEADER_SIZE + packet_size;
    group->last_read_size = PACKET_SIZE_HEADER_SIZE + packet_size;

    return 0;
}

int ack_packet_by_offset(Group* group, Topic* topic, size_t offset, size_t packet_size) {
    if (!group || !topic || packet_size == 0) {
        return -1;
    }

    if (group->attached_topic != topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    size_t total_packet_size = PACKET_SIZE_HEADER_SIZE + packet_size;
    
    if (offset + total_packet_size > chunk->size) {
        return -1;
    }

    if (group->read_pointer < offset + total_packet_size) {
        group->read_pointer = offset + total_packet_size;
    }

    return 0;
}

int ack_packet_batch(Group* group, Topic* topic, size_t count) {
    if (!group || !topic || count == 0) {
        return -1;
    }

    if (group->attached_topic != topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    size_t current_offset = group->read_pointer;
    size_t packets_acked = 0;

    for (size_t i = 0; i < count; i++) {
        if (current_offset + PACKET_SIZE_HEADER_SIZE > chunk->size) {
            break;
        }

        uint32_t packet_size;
        long bytes_read = chunk_read(chunk, &packet_size, PACKET_SIZE_HEADER_SIZE, current_offset);
        
        if (bytes_read != PACKET_SIZE_HEADER_SIZE) {
            break;
        }

        size_t total_packet_size = PACKET_SIZE_HEADER_SIZE + packet_size;
        
        if (current_offset + total_packet_size > chunk->size) {
            break;
        }

        current_offset += total_packet_size;
        packets_acked++;
    }

    if (packets_acked > 0) {
        size_t old_pointer = group->read_pointer;
        group->read_pointer = current_offset;
        group->last_read_size = current_offset - old_pointer;
    }

    return (packets_acked == count) ? 0 : -1;
}

int ack_packet_batch_by_size(Group* group, Topic* topic, size_t* packet_sizes, size_t count) {
    if (!group || !topic || !packet_sizes || count == 0) {
        return -1;
    }

    if (group->attached_topic != topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    size_t total_bytes = 0;

    for (size_t i = 0; i < count; i++) {
        if (packet_sizes[i] == 0) {
            return -1;
        }
        total_bytes += PACKET_SIZE_HEADER_SIZE + packet_sizes[i];
    }

    if (group->read_pointer + total_bytes > chunk->size) {
        return -1;
    }

    group->read_pointer += total_bytes;
    group->last_read_size = total_bytes;

    return 0;
}

size_t get_acknowledged_bytes(Group* group) {
    if (!group) {
        return 0;
    }

    return group->read_pointer;
}

