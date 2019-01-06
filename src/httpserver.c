#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>

#include "resttools.h"
#include "util.h"
#include "auth.h"

#ifndef NO_SENSOR
    #include <bmp280i2c.h>
#endif

#define PORT 8888

static unsigned int nr_of_uploading_clients = 0;

/*
* Structure to persist data for http handshakes 
*/
struct con_info_t
{
    char *data;
    int data_size;
    int current_data_size;
    int routes_map_index;
};

/*
 * Request-end function
 */
static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
    logger(INFO, "request completed");
    struct con_info_t *con_info = *con_cls;

    if (con_cls != NULL && con_info != NULL) {

        if (con_info->data_size > 0)
            nr_of_uploading_clients--;

        if (con_info->data != NULL)
            free(con_info->data);
        
        free(con_info);
        con_info->data = NULL;
        con_info = NULL;
    }
}

/*
 *  Assembling data chunks
 */
static int 
process_upload_data(struct con_info_t *con_info, const char *upload_data, int upload_data_size) {
    // prepare memory allocation
    if (con_info->data == NULL) {
        con_info->data = (char*)malloc(con_info->data_size + 1);
        memset(con_info->data, '\0', con_info->data_size + 1);
    }
    // Check actual data size
    if (con_info->current_data_size + upload_data_size > con_info->data_size)
        return 0;
    
    // Copy data to the connection info object
    memcpy(con_info->data + con_info->current_data_size, upload_data, upload_data_size);
    con_info->current_data_size += upload_data_size;

    return 1;
}

/*
 * Request entry function
 */
static int 
answer_to_connection(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{

    // Prepare connection info
    if (NULL == *con_cls) {
        logger(INFO, "client request");
        logger(INFO, url);

        // Save connection info
        struct con_info_t *con_info = (struct con_info_t*)malloc(sizeof(struct con_info_t));      
        *con_cls = (void *)con_info;
        con_info->data = NULL;
        con_info->data_size = 0;
        con_info->current_data_size = 0;
        con_info->routes_map_index = -1;
        con_info->routes_map_index = route_table_index(url);

        // Route not found
        if (con_info->routes_map_index < 0)
            return report_error(connection, "Route not found", MHD_HTTP_NOT_FOUND);

        // Method not allowed
        if (0 != strcmp(rtable[con_info->routes_map_index].method, method))
            return report_error(connection, "Method not allowed", MHD_HTTP_UNPROCESSABLE_ENTITY);//MHD_HTTP_METHOD_NOT_ALLOWED);

        // User authentication
        if (AUTH == rtable[con_info->routes_map_index].auth_type) {
            int valid = 1, res = 0;
            res = user_auth(connection, &valid);
            if (valid == 0) {
                return res;
            }
        }

        // Check max payload
        const char *cl = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);
        if (NULL != cl) {
            int size = atoi(cl);
            if (size > MAX_PAYLOAD)
                return report_error(connection, "max payload 1024", MHD_HTTP_PAYLOAD_TOO_LARGE);
            else {
                // Limit max uploading clients
                if (nr_of_uploading_clients >= 1) 
                    return report_error(connection, "Server is busy", MHD_HTTP_SERVICE_UNAVAILABLE);

                if (size > 0) {
                    nr_of_uploading_clients++;
                    con_info->data_size = size;
                }
            }
        }
        return MHD_YES;
    }

    // restore connection info
    struct con_info_t *con_info = *con_cls;

    // Data upload
    if (*upload_data_size != 0) {   
        
        int data_ret = process_upload_data(con_info, upload_data, *upload_data_size);
        *upload_data_size = 0;
        if (!data_ret)
            return report_error(connection, "payload too large", MHD_HTTP_PAYLOAD_TOO_LARGE);

        // Continue with upload -> next chunk
        return MHD_YES;
    }

    // Search for route parameters
    char *url_val_buf = strdup(url);
    const char *route_values[MAX_ROUTE_PARAM] = {0};
    if (0 > get_route_values(route_values, url_val_buf, rtable[con_info->routes_map_index].url_pattern)){
        free(url_val_buf);
        return report_error(connection, "too many route parameters", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
        
    // Route function call
    struct func_args_t args = { connection, url, method, version, con_info->data, route_values };
    int ret = rtable[con_info->routes_map_index].route_func(&args);
    free(url_val_buf);
    return ret;
}

int
main (void)
{

    #ifndef NO_SENSOR
        logger(INFO, "run with sensor");
        initI2C();
        initBmc280();
    #else
        logger(INFO, "run without sensor");
    #endif

    regexify_routes();

    logger(INFO, "run server");
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                                &answer_to_connection, NULL,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                                NULL, MHD_OPTION_END);

    if (NULL == daemon)
        return 1;

    (void) getchar ();

    MHD_stop_daemon (daemon);
    logger(INFO, "byby");
    return 0;
}
