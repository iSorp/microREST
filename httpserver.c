/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include <string.h>
#include <zlib.h>
#include <jansson.h>
#include <microhttpd.h>
#include "routes.h"


#define PORT 8888

/**
 * Calling a function depending on the given URL
 *
 * @param args 
 * @return #MHD_NO on error
 */
int
route_func(struct func_args args){
    int i;
    for(i=0;i<MAX_ROUTES; i++){
        if (NULL != rtable[i].url && 0 == strcmp(rtable[i].url, args.url)) {
            if (0 == strcmp(rtable[i].method, args.method)) {
                return rtable[i].fctptr(args);
            }
            else
                return report_error(args, "Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);
        }
    }
    return report_error(args, "Route not found", MHD_HTTP_NOT_FOUND);
}

/*
 * Request entry function
 */
int 
answer_to_connection(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
    struct func_args args = {
        connection, url, method, version, upload_data, upload_data_size, con_cls };

    // Calling Route function
    int ret = route_func(args);
    return ret;
}

int
main (void)
{
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                                &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == daemon)
        return 1;

    (void) getchar ();

    MHD_stop_daemon (daemon);
    return 0;
}




