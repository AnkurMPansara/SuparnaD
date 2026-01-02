#ifndef MANAGE_GROUPS_H
#define MANAGE_GROUPS_H

#include <stddef.h>
#include "create_topic.h"

typedef struct {
    char* group_id;
    Topic* attached_topic;
    size_t read_pointer;
    size_t last_read_size;
} Group;

typedef struct {
    Group** groups;
    size_t count;
    size_t capacity;
} GroupManager;

GroupManager* group_manager_init(void);

void group_manager_free(GroupManager* manager);

Group* create_group(const char* group_id, Topic* topic);

int add_group(GroupManager* manager, Group* group);

int remove_group(GroupManager* manager, const char* group_id);

Group* find_group(GroupManager* manager, const char* group_id);

int set_group_pointer(Group* group, size_t offset);

size_t get_group_pointer(Group* group);

int advance_group_pointer(Group* group, size_t bytes);

int reset_group_pointer(Group* group);

long read_from_group(Group* group, void* buffer, size_t size);

int group_has_more_data(Group* group);

void group_free(Group* group);

#endif

