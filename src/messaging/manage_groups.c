#include "headers/manage_groups.h"
#include "../writer/headers/read_write_data.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_GROUP_CAPACITY 16

GroupManager* group_manager_init(void) {
    GroupManager* manager = (GroupManager*)malloc(sizeof(GroupManager));
    if (!manager) {
        return NULL;
    }

    manager->groups = (Group**)malloc(INITIAL_GROUP_CAPACITY * sizeof(Group*));
    if (!manager->groups) {
        free(manager);
        return NULL;
    }

    manager->count = 0;
    manager->capacity = INITIAL_GROUP_CAPACITY;

    return manager;
}

void group_manager_free(GroupManager* manager) {
    if (!manager) {
        return;
    }

    if (manager->groups) {
        for (size_t i = 0; i < manager->count; i++) {
            if (manager->groups[i]) {
                group_free(manager->groups[i]);
            }
        }
        free(manager->groups);
    }

    free(manager);
}

Group* create_group(const char* group_id, Topic* topic) {
    if (!group_id || !topic || strlen(group_id) == 0) {
        return NULL;
    }

    Group* group = (Group*)malloc(sizeof(Group));
    if (!group) {
        return NULL;
    }

    group->group_id = strdup(group_id);
    if (!group->group_id) {
        free(group);
        return NULL;
    }

    group->attached_topic = topic;
    group->read_pointer = 0;
    group->last_read_size = 0;

    return group;
}

static int expand_group_manager(GroupManager* manager) {
    size_t new_capacity = manager->capacity * 2;
    Group** new_groups = (Group**)realloc(manager->groups, new_capacity * sizeof(Group*));
    
    if (!new_groups) {
        return -1;
    }

    manager->groups = new_groups;
    manager->capacity = new_capacity;
    return 0;
}

int add_group(GroupManager* manager, Group* group) {
    if (!manager || !group) {
        return -1;
    }

    if (find_group(manager, group->group_id) != NULL) {
        return -2;
    }

    if (manager->count >= manager->capacity) {
        if (expand_group_manager(manager) != 0) {
            return -1;
        }
    }

    manager->groups[manager->count] = group;
    manager->count++;

    return 0;
}

int remove_group(GroupManager* manager, const char* group_id) {
    if (!manager || !group_id) {
        return -1;
    }

    for (size_t i = 0; i < manager->count; i++) {
        if (manager->groups[i] && strcmp(manager->groups[i]->group_id, group_id) == 0) {
            group_free(manager->groups[i]);
            
            for (size_t j = i; j < manager->count - 1; j++) {
                manager->groups[j] = manager->groups[j + 1];
            }
            
            manager->count--;
            manager->groups[manager->count] = NULL;
            return 0;
        }
    }

    return -1;
}

Group* find_group(GroupManager* manager, const char* group_id) {
    if (!manager || !group_id) {
        return NULL;
    }

    for (size_t i = 0; i < manager->count; i++) {
        if (manager->groups[i] && strcmp(manager->groups[i]->group_id, group_id) == 0) {
            return manager->groups[i];
        }
    }

    return NULL;
}

int set_group_pointer(Group* group, size_t offset) {
    if (!group || !group->attached_topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)group->attached_topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    if (offset > chunk->size) {
        offset = chunk->size;
    }

    group->read_pointer = offset;
    return 0;
}

size_t get_group_pointer(Group* group) {
    if (!group) {
        return 0;
    }

    return group->read_pointer;
}

int advance_group_pointer(Group* group, size_t bytes) {
    if (!group || !group->attached_topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)group->attached_topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    size_t new_pointer = group->read_pointer + bytes;
    if (new_pointer > chunk->size) {
        new_pointer = chunk->size;
    }

    group->read_pointer = new_pointer;
    return 0;
}

int reset_group_pointer(Group* group) {
    return set_group_pointer(group, 0);
}

long read_from_group(Group* group, void* buffer, size_t size) {
    if (!group || !buffer || !group->attached_topic) {
        return -1;
    }

    Chunk* chunk = (Chunk*)group->attached_topic->chunk_handle;
    if (!chunk) {
        return -1;
    }

    if (chunk_load(chunk) != 0) {
        return -1;
    }

    long bytes_read = chunk_read(chunk, buffer, size, group->read_pointer);
    
    if (bytes_read > 0) {
        group->last_read_size = (size_t)bytes_read;
        group->read_pointer += (size_t)bytes_read;
    }

    return bytes_read;
}

int group_has_more_data(Group* group) {
    if (!group || !group->attached_topic) {
        return 0;
    }

    Chunk* chunk = (Chunk*)group->attached_topic->chunk_handle;
    if (!chunk) {
        return 0;
    }

    if (chunk_load(chunk) != 0) {
        return 0;
    }

    return (group->read_pointer < chunk->size) ? 1 : 0;
}

void group_free(Group* group) {
    if (!group) {
        return;
    }

    if (group->group_id) {
        free(group->group_id);
    }

    free(group);
}

