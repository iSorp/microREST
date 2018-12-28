#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <microhttpd.h>
#ifndef NO_LOGGER
    #include <log4c.h>
#endif
#ifndef NO_SENSOR
    #include <bmp280i2c.h>
#endif
#include "urltools.h"


#define PORT 8888
#define POST    1


struct con_info_t
{
    int method;
    const char* data;
};

/*
 * Request-end function
 */
static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct con_info_t *con_info = *con_cls;
    if (con_cls != NULL) {
        free(con_info);
        *con_cls = NULL;
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
    int ret = 0;
    struct func_args_t args = {
        connection, url, method};


    // Check max payload
    const char *cl = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);
    if (NULL != cl && atoi(cl) > MAX_PAYLOAD)
        ret = report_error(args, "max payload 1024", MHD_HTTP_PAYLOAD_TOO_LARGE);
    
    // Prepare connection info
    if (NULL == *con_cls) {
        struct con_info_t *con_info;
        con_info = malloc(sizeof(con_info));    
        con_info->method = POST;
        *con_cls = (void *)con_info;
        return MHD_YES; 
    }

    // prepare for upload data
    struct con_info_t *con_info = *con_cls;
    if (*upload_data_size != 0) {
        con_info->data = upload_data;
        *upload_data_size = 0;
        return MHD_YES;
    }

    // REST function call
    args.upload_data = con_info->data;
    if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0 && strcmp(method, "PUT") != 0){
        ret = report_error(args, "Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);
    }
    else {     
        // Calling Route function
        ret = route_func(args);
        *upload_data_size = 0;  
    }
    return ret;
}

int
main (void)
{
#ifndef NO_LOGGER
    int rc = 0;
    log4c_category_t* mycat = NULL;

    if (log4c_init()) {
        printf("log4c_init() failed");
        rc = 1;  
    } 
    else{
        mycat = log4c_category_get("log4c.examples.helloworld");

        log4c_category_log(mycat, LOG4C_PRIORITY_ERROR, "Hello World!");

        /* Explicitly call the log4c cleanup routine */
        if ( log4c_fini()){
            printf("log4c_fini() failed");
        }
    }
#endif

#ifndef NO_SENSOR
    initI2C();
    initBmc280();
#endif

    regexify_routes();

    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                                &answer_to_connection, NULL,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                                NULL, MHD_OPTION_END);
    if (NULL == daemon)
        return 1;

    (void) getchar ();

    MHD_stop_daemon (daemon);
    return 0;
}
