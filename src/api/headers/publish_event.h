#ifndef API_PUBLISH_EVENT_H
#define API_PUBLISH_EVENT_H

#include <microhttpd.h>
#include <stddef.h>

enum MHD_Result handle_publish_request(struct MHD_Connection *connection,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls);

#endif

