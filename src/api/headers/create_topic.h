#ifndef API_CREATE_TOPIC_H
#define API_CREATE_TOPIC_H

#include <microhttpd.h>
#include <stddef.h>

enum MHD_Result handle_create_topic_request(struct MHD_Connection *connection,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls);

#endif

