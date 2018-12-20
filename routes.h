#include <microhttpd.h>

static const int MAX_ROUTES = 20;

struct func_args {
    struct MHD_Connection *connection;
    const char *url;
    const char *method;
    const char *version;
    const char *upload_data;
    size_t *upload_data_size;
    void **con_cls;
};

struct routes_map {
    int (*fctptr)(struct func_args); 
    const char *url;
    const char *method;
};

struct routes_map rtable[MAX_ROUTES];

int
route_func(struct func_args args);

int
report_error(struct func_args args, char *message, int status_code);