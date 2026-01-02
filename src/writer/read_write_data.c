#include "read_write_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 1024

Chunk* chunk_init(const char *file_path) {
    if (!file_path) return NULL;

    Chunk *chunk = (Chunk*)malloc(sizeof(Chunk));
    if (!chunk) return NULL;

    chunk->file_path = strdup(file_path);
    if (!chunk->file_path) {
        free(chunk);
        return NULL;
    }

    chunk->data = NULL;
    chunk->size = 0;
    chunk->capacity = 0;
    chunk->persisted_size = 0;

    if (pthread_mutex_init(&chunk->mutex, NULL) != 0) {
        free(chunk->file_path);
        free(chunk);
        return NULL;
    }

    return chunk;
}

void chunk_free(Chunk *chunk) {
    if (!chunk) return;

    pthread_mutex_lock(&chunk->mutex);
    if (chunk->data) free(chunk->data);
    if (chunk->file_path) free(chunk->file_path);
    pthread_mutex_unlock(&chunk->mutex);
    
    pthread_mutex_destroy(&chunk->mutex);
    free(chunk);
}

int chunk_load(Chunk *chunk) {
    if (!chunk) return -1;

    pthread_mutex_lock(&chunk->mutex);

    FILE *f = fopen(chunk->file_path, "rb");
    if (!f) {
        chunk->size = 0;
        chunk->persisted_size = 0;
        pthread_mutex_unlock(&chunk->mutex);
        return 0; 
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        fclose(f);
        pthread_mutex_unlock(&chunk->mutex);
        return -1;
    }

    if ((size_t)fsize > chunk->capacity) {
        unsigned char *new_data = realloc(chunk->data, fsize);
        if (!new_data) {
            fclose(f);
            pthread_mutex_unlock(&chunk->mutex);
            return -1;
        }
        chunk->data = new_data;
        chunk->capacity = fsize;
    }

    size_t read_count = fread(chunk->data, 1, fsize, f);
    fclose(f);

    if (read_count != (size_t)fsize) {
        pthread_mutex_unlock(&chunk->mutex);
        return -1;
    }

    chunk->size = read_count;
    chunk->persisted_size = read_count; // Synced with disk

    pthread_mutex_unlock(&chunk->mutex);
    return 0;
}

int chunk_append(Chunk *chunk, const void *data, size_t size) {
    if (!chunk || !data || size == 0) return -1;

    pthread_mutex_lock(&chunk->mutex);

    size_t required_capacity = chunk->size + size;

    if (required_capacity > chunk->capacity) {
        size_t new_capacity = chunk->capacity == 0 ? INITIAL_CAPACITY : chunk->capacity * 2;
        while (new_capacity < required_capacity) {
            new_capacity *= 2;
        }

        unsigned char *new_ptr = realloc(chunk->data, new_capacity);
        if (!new_ptr) {
            pthread_mutex_unlock(&chunk->mutex);
            return -1;
        }
        chunk->data = new_ptr;
        chunk->capacity = new_capacity;
    }

    memcpy(chunk->data + chunk->size, data, size);
    chunk->size += size;

    pthread_mutex_unlock(&chunk->mutex);
    return 0;
}

long chunk_read(Chunk *chunk, void *buffer, size_t size, size_t offset) {
    if (!chunk || !buffer) return -1;

    pthread_mutex_lock(&chunk->mutex);

    if (offset >= chunk->size) {
        pthread_mutex_unlock(&chunk->mutex);
        return 0;
    }

    size_t available = chunk->size - offset;
    size_t to_read = (size < available) ? size : available;

    memcpy(buffer, chunk->data + offset, to_read);

    pthread_mutex_unlock(&chunk->mutex);
    return (long)to_read;
}

int chunk_save_delta(Chunk *chunk) {
    if (!chunk) return -1;

    pthread_mutex_lock(&chunk->mutex);

    if (chunk->size <= chunk->persisted_size) {
        pthread_mutex_unlock(&chunk->mutex);
        return 0;
    }

    size_t delta_size = chunk->size - chunk->persisted_size;
    unsigned char *delta_ptr = chunk->data + chunk->persisted_size;

    FILE *f = fopen(chunk->file_path, "ab");
    if (!f) {
        pthread_mutex_unlock(&chunk->mutex);
        return -1;
    }

    size_t written = fwrite(delta_ptr, 1, delta_size, f);
    fclose(f);

    if (written != delta_size) {
        pthread_mutex_unlock(&chunk->mutex);
        return -1;
    }

    chunk->persisted_size = chunk->size;

    pthread_mutex_unlock(&chunk->mutex);
    return 0;
}