#include "headers/encoder.h"
#include <zstd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_COMPRESSION_LEVEL 3
#define MIN_DICTIONARY_SIZE 1024
#define MAX_DICTIONARY_SIZE (1024 * 1024) // 1MB max

int encode_string(const char* json_string, size_t json_size, 
                  uint8_t** compressed_data, size_t* compressed_size, 
                  int compression_level) {
    if (!json_string || !compressed_data || !compressed_size || json_size == 0) {
        return -1;
    }

    if (compression_level < 1 || compression_level > ZSTD_maxCLevel()) {
        compression_level = DEFAULT_COMPRESSION_LEVEL;
    }

    size_t const max_compressed_size = ZSTD_compressBound(json_size);
    
    *compressed_data = (uint8_t*)malloc(max_compressed_size);
    if (!*compressed_data) {
        return -2;
    }

    size_t const result = ZSTD_compress(*compressed_data, max_compressed_size,
                                       json_string, json_size,
                                       compression_level);

    if (ZSTD_isError(result)) {
        free(*compressed_data);
        *compressed_data = NULL;
        return -3;
    }

    *compressed_size = result;
    return 0;
}

int decode_string(const uint8_t* compressed_data, size_t compressed_size,
                  char** decompressed_string, size_t* decompressed_size) {
    if (!compressed_data || !decompressed_string || !decompressed_size || compressed_size == 0) {
        return -1;
    }

    unsigned long long const decompressed_size_ll = ZSTD_getFrameContentSize(compressed_data, compressed_size);
    
    if (decompressed_size_ll == ZSTD_CONTENTSIZE_ERROR) {
        return -2;
    }
    
    if (decompressed_size_ll == ZSTD_CONTENTSIZE_UNKNOWN) {
        size_t estimated_size = compressed_size * 4;
        *decompressed_string = (char*)malloc(estimated_size);
        if (!*decompressed_string) {
            return -3;
        }

        size_t const result = ZSTD_decompress(*decompressed_string, estimated_size,
                                             compressed_data, compressed_size);
        
        if (ZSTD_isError(result)) {
            free(*decompressed_string);
            *decompressed_string = NULL;
            return -4;
        }

        *decompressed_size = result;
        return 0;
    }

    *decompressed_string = (char*)malloc((size_t)decompressed_size_ll + 1);
    if (!*decompressed_string) {
        return -3; 
    }

    size_t const result = ZSTD_decompress(*decompressed_string, (size_t)decompressed_size_ll,
                                         compressed_data, compressed_size);

    if (ZSTD_isError(result)) {
        free(*decompressed_string);
        *decompressed_string = NULL;
        return -4;
    }

    (*decompressed_string)[result] = '\0';
    *decompressed_size = result;
    return 0;
}

int encode_string_with_dict(const char* json_string, size_t json_size,
                            const void* dictionary, size_t dictionary_size,
                            uint8_t** compressed_data, size_t* compressed_size,
                            int compression_level) {
    if (!json_string || !compressed_data || !compressed_size || json_size == 0) {
        return -1;
    }

    if (compression_level < 1 || compression_level > ZSTD_maxCLevel()) {
        compression_level = DEFAULT_COMPRESSION_LEVEL;
    }

    ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    if (!cctx) {
        return -2;
    }

    if (dictionary && dictionary_size > 0) {
        size_t const dict_result = ZSTD_CCtx_loadDictionary(cctx, dictionary, dictionary_size);
        if (ZSTD_isError(dict_result)) {
            ZSTD_freeCCtx(cctx);
            return -3;
        }
    }

    size_t const level_result = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compression_level);
    if (ZSTD_isError(level_result)) {
        ZSTD_freeCCtx(cctx);
        return -4;
    }

    size_t const max_compressed_size = ZSTD_compressBound(json_size);
    
    *compressed_data = (uint8_t*)malloc(max_compressed_size);
    if (!*compressed_data) {
        ZSTD_freeCCtx(cctx);
        return -5;
    }

    size_t const result = ZSTD_compress2(cctx, *compressed_data, max_compressed_size,
                                         json_string, json_size);

    ZSTD_freeCCtx(cctx);

    if (ZSTD_isError(result)) {
        free(*compressed_data);
        *compressed_data = NULL;
        return -6;
    }

    *compressed_size = result;
    return 0;
}

int decode_string_with_dict(const uint8_t* compressed_data, size_t compressed_size,
                            const void* dictionary, size_t dictionary_size,
                            char** decompressed_string, size_t* decompressed_size) {
    if (!compressed_data || !decompressed_string || !decompressed_size || compressed_size == 0) {
        return -1;
    }

    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    if (!dctx) {
        return -2;
    }

    if (dictionary && dictionary_size > 0) {
        size_t const dict_result = ZSTD_DCtx_loadDictionary(dctx, dictionary, dictionary_size);
        if (ZSTD_isError(dict_result)) {
            ZSTD_freeDCtx(dctx);
            return -3;
        }
    }

    unsigned long long const decompressed_size_ll = ZSTD_getFrameContentSize(compressed_data, compressed_size);
    
    size_t buffer_size;
    if (decompressed_size_ll == ZSTD_CONTENTSIZE_ERROR) {
        ZSTD_freeDCtx(dctx);
        return -4;
    } else if (decompressed_size_ll == ZSTD_CONTENTSIZE_UNKNOWN) {
        buffer_size = compressed_size * 4;
    } else {
        buffer_size = (size_t)decompressed_size_ll;
    }

    *decompressed_string = (char*)malloc(buffer_size + 1); // +1 for null terminator
    if (!*decompressed_string) {
        ZSTD_freeDCtx(dctx);
        return -5;
    }

    size_t const result = ZSTD_decompressDCtx(dctx, *decompressed_string, buffer_size,
                                             compressed_data, compressed_size);

    ZSTD_freeDCtx(dctx);

    if (ZSTD_isError(result)) {
        free(*decompressed_string);
        *decompressed_string = NULL;
        return -6;
    }

    (*decompressed_string)[result] = '\0';
    *decompressed_size = result;
    return 0;
}

int save_dictionary(const void* dictionary, size_t dictionary_size, const char* filepath) {
    if (!dictionary || dictionary_size == 0 || !filepath) {
        return -1;
    }

    FILE* file = fopen(filepath, "wb");
    if (!file) {
        return -2;
    }

    size_t written = fwrite(dictionary, 1, dictionary_size, file);
    fclose(file);

    if (written != dictionary_size) {
        return -3;
    }

    return 0;
}

int fetch_dictionary(const char* filepath, void** dictionary, size_t* dictionary_size) {
    if (!filepath || !dictionary || !dictionary_size) {
        return -1;
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return -2; // Failed to open file
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -3;
    }

    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return -4;
    }

    if (file_size == 0) {
        fclose(file);
        return -5;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -3;
    }

    *dictionary = malloc((size_t)file_size);
    if (!*dictionary) {
        fclose(file);
        return -6;
    }

    size_t read_size = fread(*dictionary, 1, (size_t)file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(*dictionary);
        *dictionary = NULL;
        return -7;
    }

    *dictionary_size = (size_t)file_size;
    return 0;
}

int train_dictionary(const char** samples, const size_t* sample_sizes, 
                     size_t num_samples, size_t dictionary_size,
                     void** dictionary, size_t* actual_dict_size) {
    if (!samples || !sample_sizes || num_samples == 0 || !dictionary || !actual_dict_size) {
        return -1;
    }

    if (dictionary_size < MIN_DICTIONARY_SIZE || dictionary_size > MAX_DICTIONARY_SIZE) {
        dictionary_size = 1024 * 10;
    }

    void** sample_buffers = (void**)malloc(num_samples * sizeof(void*));
    if (!sample_buffers) {
        return -2;
    }

    for (size_t i = 0; i < num_samples; i++) {
        sample_buffers[i] = malloc(sample_sizes[i]);
        if (!sample_buffers[i]) {
            for (size_t j = 0; j < i; j++) {
                free(sample_buffers[j]);
            }
            free(sample_buffers);
            return -2;
        }
        memcpy(sample_buffers[i], samples[i], sample_sizes[i]);
    }

    *dictionary = malloc(dictionary_size);
    if (!*dictionary) {
        for (size_t i = 0; i < num_samples; i++) {
            free(sample_buffers[i]);
        }
        free(sample_buffers);
        return -2;
    }

    size_t const result = ZSTD_trainFromBuffer(*dictionary, dictionary_size,
                                               sample_buffers, sample_sizes,
                                               num_samples);

    for (size_t i = 0; i < num_samples; i++) {
        free(sample_buffers[i]);
    }
    free(sample_buffers);

    if (ZSTD_isError(result)) {
        free(*dictionary);
        *dictionary = NULL;
        return -3;
    }

    *actual_dict_size = result;
    return 0;
}

