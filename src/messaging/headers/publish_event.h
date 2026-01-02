#ifndef PUBLISH_EVENT_H
#define PUBLISH_EVENT_H

#include <stddef.h>
#include <stdint.h>
#include "create_topic.h"

int publish_event(Topic* topic, const void* data, size_t data_size);

int publish_event_string(Topic* topic, const char* data);

int publish_event_compressed(Topic* topic, const void* data, size_t data_size, int compression_level);

int publish_event_batch(Topic* topic, const void** data_array, const size_t* sizes, size_t count);

int flush_topic(Topic* topic);

size_t get_topic_size(Topic* topic);

#endif

