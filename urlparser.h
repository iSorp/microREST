#include <microhttpd.h>

struct route_args {
    struct MHD_Connection *connection;
    char *url;
    char *method;
    char *version;
    char *upload_data;
    size_t *upload_data_size;
    void **con_cls;
};

struct MHD_Response *
route_func(struct route_args args);
