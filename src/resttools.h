#include <microhttpd.h>

#define PARAM_PREFIX '{'
#define PARAM_SUFFIX '}'

#define MAX_ROUTES 20
#define MAX_ROUTE_PARTS 20
#define MAX_ROUTE_PARAM 10
#define MAX_URL_SIZE 256
#define MAX_PAYLOAD 1024
#define JSON_CONTENT_TYPE "application/json"

/**
 * Structure containing argumens for the route functions
 */
struct func_args_t {
    struct MHD_Connection *connection;
    const char *url;
    const char *method;
    const char *upload_data;
    const char **route_values;
};

/*
* REST function type definition
*/
typedef int 
(*fctptr)(struct func_args_t);

/**
 * Structure containing route functions
 */
struct routes_map_t {
    fctptr rest_func; 
    char *url_pattern;
    char *method;
    char *url_regex;
};
struct routes_map_t rtable[MAX_ROUTES];

/*
* Structure to persist data for http hansshakes 
*/
struct con_info_t
{
    char *data ;
    int routes_map_index;
};

/**
* The function finds values in a url defined by the url pattern.
* For example /myroute/<myparam>/otherroute/
* it returns the value of <myparam>
*
* @param values
* @param url 
* @param pattern 
* @return actual number of values, on error -1
*/
int
get_route_values(const char **values, char* url, const char *pattern);

/**
* Returns the index of the route table on which the url_pattern matches a given url.
* @param url 
* @return -1 on error
*/
int
route_table_index (const char *url);

/**
* Creates Rexgex Patters for all URL. The URL are allocated in new memory locations.
* This Function has to be called ONCE.
*/
void
regexify_routes();

/*
* Creates a response from buffer and queue it to be transmitted to the client.
* @param connection
* @param message
* @param content_type
* @param status_code
* @return #MHD_NO on error
*/
int 
buffer_queue_response(struct MHD_Connection *connection, char *message, char *content_type, int status_code);
int 
buffer_queue_response_ok(struct MHD_Connection *connection, char *message, char *content_type);

/*
* Responses an error message
*
* @param connection 
* @param message 
* @param status_code
* @return #MHD_NO on error
*/
int
report_error(struct MHD_Connection *connection, char *message, int status_code);