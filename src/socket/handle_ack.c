#include "handle_ack.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int wait_for_ack(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};

    ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
    
    if (valread < 0) {
        perror("Read ACK failed");
        return -1;
    }
    
    if (valread == 0) {
        printf("Client disconnected while waiting for ACK.\n");
        return -1;
    }

    printf("ACK received from client: %s\n", buffer);

    if (strstr(buffer, "ACK") != NULL || strstr(buffer, "OK") != NULL) {
        return 1;
    }

    return 0;
}