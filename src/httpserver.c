#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>

#include "resttools.h"
#include "util.h"

#ifndef NO_SENSOR
    #include <bmp280i2c.h>
#endif


#define PORT 8888
#define POST    1


/*
 * Request-end function
 */
static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
    logger(INFO, "request completed");
    struct con_info_t *con_info = *con_cls;

    // free all memory
    if (con_cls != NULL && con_info != NULL) {
        if (con_info->data != NULL)
            free(con_info->data);
        free(con_info);
        con_info->data = NULL;
        con_info = NULL;
    }
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
        struct con_info_t *con_info;
        con_info = malloc(sizeof(con_info));   
        con_info->data = NULL;
        con_info->routes_map_index = -1;

        // get route table index
        con_info->routes_map_index = route_table_index(url);

        // Route not found
        if (con_info->routes_map_index < 0)
            return report_error(connection, "Route not found", MHD_HTTP_NOT_FOUND);

        // Route method not allowed
        if (0 != strcmp(rtable[con_info->routes_map_index].method, method))
            return report_error(connection, "Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);

        // Check max payload
        const char *cl = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);
        if (NULL != cl && atoi(cl) > 1024)
            return report_error(connection, "max payload 1024", MHD_HTTP_PAYLOAD_TOO_LARGE);

        // Save connection info for continue
        *con_cls = (void *)con_info;

        // Continue (100) if message contains a payload
        if (NULL != cl && atoi(cl) < MAX_PAYLOAD)
            return MHD_YES;
    }

    // prepare for data upload 
    struct con_info_t *con_info = *con_cls;
    if (*upload_data_size != 0) {
        con_info->data = (char*)malloc(*upload_data_size+1);
        memset(con_info->data, '\0', *upload_data_size+1);
        memcpy(con_info->data, upload_data, *upload_data_size);
        *upload_data_size = 0;

        // Continue (100) payload is uploaded
        return MHD_YES;
    }

    // Search for route parameters
    const char *route_values[MAX_ROUTE_PARAM] = {0};
    char val_buf[MAX_URL_SIZE] = {0};
    snprintf(val_buf, sizeof(val_buf), "%s%s", val_buf, url);
    get_route_values(route_values, val_buf, rtable[con_info->routes_map_index].url_pattern);
    
    // REST function call
    struct func_args_t args = { connection, url, method, con_info->data, route_values};
    int ret = rtable[con_info->routes_map_index].rest_func(args);
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
    daemon = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
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
