#include "http_server.h"
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

/* Request router */
#include "handler.h"

static struct MHD_Daemon *g_daemon = NULL;

static int request_handler(void *cls,
                           struct MHD_Connection *connection,
                           const char *url,
                           const char *method,
                           const char *version,
                           const char *upload_data,
                           size_t *upload_data_size,
                           void **con_cls)
{
    /* route_request should return an MHD_Result (MHD_YES / MHD_NO) */
    return route_request(cls, connection, url, method, version, upload_data, upload_data_size, con_cls);
}

int start_server(uint16_t port)
{
    if (g_daemon != NULL) {
        fprintf(stderr, "Server already running\n");
        return 0;
    }

    g_daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
        port,
        NULL, NULL,
        request_handler, NULL,
        MHD_OPTION_END);

    if (g_daemon == NULL) {
        fprintf(stderr, "Failed to start libmicrohttpd daemon on port %u\n", (unsigned)port);
        return -1;
    }

    printf("HTTP server started on port %u\n", (unsigned)port);
    return 0;
}

void stop_server(void)
{
    if (g_daemon) {
        MHD_stop_daemon(g_daemon);
        g_daemon = NULL;
        printf("HTTP server stopped\n");
    }
}
