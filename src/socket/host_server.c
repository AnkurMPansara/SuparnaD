#include "host_server.h"
#include "headers/server_event_handler.h"
#include "connect_to_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int start_hosting(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }

    printf("Server hosted successfully on port %d\n", port);
    return server_fd;
}

typedef struct {
    int client_fd;
    GroupManager* group_manager;
} ClientThreadData;

static void* handle_client_thread(void* arg) {
    ClientThreadData* thread_data = (ClientThreadData*)arg;
    
    handle_client_session(thread_data->client_fd, thread_data->group_manager, "./topics");
    free(thread_data);
    return NULL;
}

int run_server_loop(int server_fd, GroupManager* group_manager) {
    if (server_fd < 0) {
        return -1;
    }

    printf("Server loop started. Waiting for clients...\n");

    while (1) {
        int client_fd = accept_client(server_fd);
        if (client_fd < 0) {
            continue;
        }

        ClientThreadData* thread_data = malloc(sizeof(ClientThreadData));
        if (!thread_data) {
            close(client_fd);
            continue;
        }

        thread_data->client_fd = client_fd;
        thread_data->group_manager = group_manager;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client_thread, thread_data) != 0) {
            perror("Failed to create thread");
            free(thread_data);
            close(client_fd);
            continue;
        }

        pthread_detach(thread);
    }

    return 0;
}