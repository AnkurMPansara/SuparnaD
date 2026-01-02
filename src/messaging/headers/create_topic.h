#ifndef CREATE_TOPIC_H
#define CREATE_TOPIC_H

#include <stddef.h>

typedef struct {
    char* topic_name;
    char* file_path;
    void* chunk_handle;
} Topic;

Topic* create_topic(const char* topic_name, const char* base_path);

int topic_exists(const char* topic_name, const char* base_path);

int delete_topic(Topic* topic);

void topic_free(Topic* topic);

#endif

