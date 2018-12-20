#include <microhttpd.h>


static const int MAX_ROUTES = 20;

/**
 * Structure containing argumens for the route functions
 */
struct func_args {
    struct MHD_Connection *connection;
    const char *url;
    const char *method;
    const char *version;
    const char *upload_data;
    size_t *upload_data_size;
    void **con_cls;
};

/**
 * Structure containing route functions
 */
struct routes_map {
    int (*fctptr)(struct func_args); 
    const char *url;
    const char *method;
};
struct routes_map rtable[MAX_ROUTES];

/*
* Responses an error message
*
* @param args 
* @param message 
* @param status_code
* @return #MHD_NO on error
*/
int
report_error(struct func_args args, char *message, int status_code);