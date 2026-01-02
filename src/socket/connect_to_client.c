#include "connect_to_client.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int accept_client(int server_fd) {
    int client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    printf("Waiting for client to connect...\n");

    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        return -1;
    }

    printf("Client connected successfully.\n");
    return client_fd;
}