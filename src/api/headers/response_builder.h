#ifndef RESPONSE_BUILDER_H
#define RESPONSE_BUILDER_H

#include <microhttpd.h>
#include <stddef.h>


struct MHD_Response *build_response_from_buffer(int status_code,
                                                const char *body,
                                                size_t body_len,
                                                const char *content_type);

struct MHD_Response *build_response_from_file(const char *file_path,
                                              const char *content_type);

#endif
