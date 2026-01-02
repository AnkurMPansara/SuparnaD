#ifndef READ_WRITE_DATA_H
#define READ_WRITE_DATA_H

#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define the Chunk structure
typedef struct {
    char *file_path;            // Path to the associated file
    unsigned char *data;        // Pointer to data in RAM
    size_t size;                // Current size of data in RAM
    size_t capacity;            // Current allocated capacity
    size_t persisted_size;      // Size of data already saved to disk (for delta calculation)
    pthread_mutex_t mutex;      // Mutex for thread safety
} Chunk;

/**
 * Initialize a new chunk associated with a specific file path.
 * @param file_path The path to the file on disk.
 * @return Pointer to the new Chunk, or NULL on failure.
 */
Chunk* chunk_init(const char *file_path);

/**
 * Free the memory associated with the chunk.
 * @param chunk Pointer to the chunk to free.
 */
void chunk_free(Chunk *chunk);

/**
 * Load data from the chunk's file path into RAM.
 * Replaces any existing data in the chunk.
 * @param chunk Pointer to the chunk.
 * @return 0 on success, -1 on failure.
 */
int chunk_load(Chunk *chunk);

/**
 * Append data to the chunk in RAM.
 * This operation is thread-safe and handles memory resizing.
 * @param chunk Pointer to the chunk.
 * @param data Pointer to the data to append.
 * @param size Size of the data to append.
 * @return 0 on success, -1 on failure.
 */
int chunk_append(Chunk *chunk, const void *data, size_t size);

/**
 * Read data from the chunk in RAM at a specific offset.
 * @param chunk Pointer to the chunk.
 * @param buffer Destination buffer to copy data into.
 * @param size Number of bytes to read.
 * @param offset Offset from the beginning of the chunk data.
 * @return Number of bytes actually read, or -1 on failure.
 */
long chunk_read(Chunk *chunk, void *buffer, size_t size, size_t offset);

/**
 * Save only the new data (delta) from RAM to the file path.
 * Appends data added since the last load or save operation.
 * @param chunk Pointer to the chunk.
 * @return 0 on success, -1 on failure.
 */
int chunk_save_delta(Chunk *chunk);

#ifdef __cplusplus
}
#endif

#endif