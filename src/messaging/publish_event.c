#include "headers/publish_event.h"
#include "../writer/headers/read_write_data.h"
#include "headers/encoder.h"
#include <stdlib.h>
#include <string.h>

#define PACKET_SIZE_HEADER_SIZE sizeof(uint32_t)
#define MAX_EVENT_SIZE (10 * 1024 * 1024) // 10MB max event size

int publish_event(Topic* topic, const void* data, size_t data_size) {
    if (!topic || !data || data_size == 0) {
        return -1;
    }

    if (data_size > MAX_EVENT_SIZE) {
        return -2;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    uint32_t packet_size = (uint32_t)data_size;

    if (chunk_append(chunk, &packet_size, PACKET_SIZE_HEADER_SIZE) != 0) {
        return -1;
    }

    if (chunk_append(chunk, data, data_size) != 0) {
        return -1;
    }

    if (chunk_save_delta(chunk) != 0) {
        return -1;
    }

    return 0;
}

int publish_event_string(Topic* topic, const char* data) {
    if (!topic || !data) {
        return -1;
    }

    size_t data_size = strlen(data);
    if (data_size == 0) {
        return -1;
    }

    return publish_event(topic, data, data_size);
}

int publish_event_compressed(Topic* topic, const void* data, size_t data_size, int compression_level) {
    if (!topic || !data || data_size == 0) {
        return -1;
    }

    if (data_size > MAX_EVENT_SIZE) {
        return -2;
    }

    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;

    int encode_result = encode_string((const char*)data, data_size, 
                                     &compressed_data, &compressed_size, 
                                     compression_level);
    
    if (encode_result != 0 || !compressed_data) {
        return -1;
    }

    int result = publish_event(topic, compressed_data, compressed_size);
    
    free(compressed_data);
    
    return result;
}

int publish_event_batch(Topic* topic, const void** data_array, const size_t* sizes, size_t count) {
    if (!topic || !data_array || !sizes || count == 0) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (!data_array[i] || sizes[i] == 0 || sizes[i] > MAX_EVENT_SIZE) {
            return -1;
        }

        uint32_t packet_size = (uint32_t)sizes[i];

        if (chunk_append(chunk, &packet_size, PACKET_SIZE_HEADER_SIZE) != 0) {
            return -1;
        }

        if (chunk_append(chunk, data_array[i], sizes[i]) != 0) {
            return -1;
        }
    }

    if (chunk_save_delta(chunk) != 0) {
        return -1;
    }

    return 0;
}

int flush_topic(Topic* topic) {
    if (!topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    return chunk_save_delta(chunk);
}

size_t get_topic_size(Topic* topic) {
    if (!topic) {
        return 0;
    }

    Chunk* chunk = (Chunk*)topic->chunk_handle;
    if (!chunk) {
        return 0;
    }

    if (chunk_load(chunk) != 0) {
        return 0;
    }

    return chunk->size;
}

