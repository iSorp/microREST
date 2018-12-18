#include <microhttpd.h>

static const int MAX_ROUTES = 20;

//typedef struct MHD_Response *(*eval_fctptr)(struct route_args args);
struct routes_map {
    void* (*fctptr)(struct route_args); 
    char url[200];
    char method[5];
};

extern struct routes_map rtable[];