#ifndef ENCODER_H
#define ENCODER_H

#include <stddef.h>
#include <stdint.h>

int encode_string(const char* json_string, size_t json_size, 
                  uint8_t** compressed_data, size_t* compressed_size, 
                  int compression_level);

int decode_string(const uint8_t* compressed_data, size_t compressed_size,
                  char** decompressed_string, size_t* decompressed_size);

int encode_string_with_dict(const char* json_string, size_t json_size,
                            const void* dictionary, size_t dictionary_size,
                            uint8_t** compressed_data, size_t* compressed_size,
                            int compression_level);

int decode_string_with_dict(const uint8_t* compressed_data, size_t compressed_size,
                            const void* dictionary, size_t dictionary_size,
                            char** decompressed_string, size_t* decompressed_size);

int save_dictionary(const void* dictionary, size_t dictionary_size, const char* filepath);

int fetch_dictionary(const char* filepath, void** dictionary, size_t* dictionary_size);

int train_dictionary(const char** samples, const size_t* sample_sizes, 
                     size_t num_samples, size_t dictionary_size,
                     void** dictionary, size_t* actual_dict_size);

#endif