#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>

/* Start HTTP server on the given port */
int start_server(uint16_t port);

/* Stop server gracefully */
void stop_server(void);

#endif
