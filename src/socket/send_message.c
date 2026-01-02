#include "send_message.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int send_msg(int client_fd, const char *message) {
    if (client_fd < 0) {
        fprintf(stderr, "Invalid client file descriptor.\n");
        return -1;
    }

    ssize_t sent_bytes = send(client_fd, message, strlen(message), 0);
    
    if (sent_bytes < 0) {
        perror("Send failed");
        return -1;
    }

    printf("Message sent: %s\n", message);
    return 0;
}