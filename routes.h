#include <microhttpd.h>

static const int MAX_ROUTES = 20;

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

//typedef struct MHD_Response *(*eval_fctptr)(struct route_args args);
struct routes_map {
    void* (*fctptr)(struct route_args); 
    char url[200];
    char method[5];
};

extern struct routes_map rtable[];