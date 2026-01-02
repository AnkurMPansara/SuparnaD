#include "response_builder.h"
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct MHD_Response *build_response_from_buffer(int status_code __attribute__((unused)),
                                                const char *body,
                                                size_t body_len,
                                                const char *content_type)
{
    if (!body) return NULL;
    struct MHD_Response *response = MHD_create_response_from_buffer(body_len, (void *)body, MHD_RESPMEM_MUST_COPY);
    if (!response) return NULL;
    if (content_type) {
        MHD_add_response_header(response, "Content-Type", content_type);
    } else {
        MHD_add_response_header(response, "Content-Type", "text/plain");
    }
    return response;
}

struct MHD_Response *build_response_from_file(const char *file_path, const char *content_type)
{
    FILE *f = fopen(file_path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long len = ftell(f);
    rewind(f);

    int fd = fileno(f);
    if (fd < 0) { fclose(f); return NULL; }

    struct MHD_Response *response = MHD_create_response_from_fd_at_offset64(len, fd, 0);
    if (!response) { fclose(f); return NULL; }

    if (content_type) {
        MHD_add_response_header(response, "Content-Type", content_type);
    }

    fclose(f);
    return response;
}
