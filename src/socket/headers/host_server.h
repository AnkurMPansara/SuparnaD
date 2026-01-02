#ifndef HOST_SERVER_H
#define HOST_SERVER_H

#include "../../messaging/headers/manage_groups.h"

int start_hosting(int port);

int run_server_loop(int server_fd, GroupManager* group_manager);

#endif
