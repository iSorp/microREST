#include <microhttpd.h>

#define MAX_ROUTES 20
#define MAX_URL_SIZE 255
#define MAX_URL_PARAM 10
#define MAX_PAYLOAD 1024


/**
 * Structure containing argumens for the route functions
 */
struct func_args_t {
    struct MHD_Connection *connection;
    const char *url;
    const char *method;
    const char *upload_data;
};

/**
 * Structure containing route functions
 */
struct routes_map_t {
    int (*fctptr)(struct func_args_t, ...); 
    const char *url;
    const char *method;
    struct {
        const char *regexurl;
    };
};
struct routes_map_t rtable[MAX_ROUTES];

/*
* Responses an error message
*
* @param args 
* @param message 
* @param status_code
* @return #MHD_NO on error
*/
int
report_error(struct func_args_t args, char *message, int status_code);

/**
 * Calling a function depending on the given URL
 *
 * @param args 
 * @return #MHD_NO on error
 */
int
route_func(struct func_args_t args);

/**
* Creates Rexgex Patters for all URL. The URL are allocated in new memory locations.
* This Function has to be called ONCE.
*/
void
regexify_routes();