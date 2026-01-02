#include "handler.h"
#include "response_builder.h"
#include <string.h>
#include <stdio.h>


enum MHD_Result route_request(void *cls,
                              struct MHD_Connection *connection,
                              const char *url,
                              const char *method,
                              const char *version,
                              const char *upload_data,
                              size_t *upload_data_size,
                              void **con_cls)
{
    (void)cls; (void)version; /* unused in this example */

    if (strcmp(method, "GET") == 0) {
        if (strcmp(url, "/") == 0) {
            const char *body = "{\"message\":\"Hello from C http server\"}";
            struct MHD_Response *resp = build_response_from_buffer(200, body, strlen(body), "application/json");
            if (!resp) return MHD_NO;
            enum MHD_Result ret = (enum MHD_Result) MHD_queue_response(connection, MHD_HTTP_OK, resp);
            MHD_destroy_response(resp);
            return ret;
        } else if (strcmp(url, "/ping") == 0) {
            const char *body = "pong\n";
            struct MHD_Response *resp = build_response_from_buffer(200, body, strlen(body), "text/plain");
            if (!resp) return MHD_NO;
            enum MHD_Result ret = (enum MHD_Result) MHD_queue_response(connection, MHD_HTTP_OK, resp);
            MHD_destroy_response(resp);
            return ret;
        }
    }

    /* fallback 404 */
    {
        const char *body = "Not Found\n";
        struct MHD_Response *resp = build_response_from_buffer(404, body, strlen(body), "text/plain");
        if (!resp) return MHD_NO;
        enum MHD_Result ret = (enum MHD_Result) MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, resp);
        MHD_destroy_response(resp);
        return ret;
    }
}
