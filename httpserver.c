/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

#include <zlib.h>
#include <jansson.h>
#include <microhttpd.h>

#include "routes.h"

#define PORT 8888


/**
 * Responses an error message
 *
 * @param args 
 * @param message 
 * @param status_code
 * @return MHD_Response
 */
struct MHD_Response *
report_error (struct route_args args, const char *message, int status_code)
{
    struct MHD_Response * response;
    json_t *j = json_pack("{s:s,s:s}", "status", "error", "message", message);
    char *s = json_dumps(j , 0);

    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_queue_response(args.connection , status_code, response);
    return response;
}

/**
 * Calling a function depending on the given URL
 *
 * @param args 
 * @return MHD_Response
 */
struct MHD_Response *
route_func(struct route_args args){
    int i;
    for(i=0;i<MAX_ROUTES; i++){
        if (0 == strcmp(rtable[i].url, args.url)) {
            if (0 == strcmp(rtable[i].method, args.method)) 
                return rtable[i].fctptr(args);
            else
                return report_error(args, "Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);
        }
    }
    return report_error(args, "Route not found", MHD_HTTP_NOT_FOUND);
}

/**
 * Request entry function
 */
int 
answer_to_connection(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  struct MHD_Response *response;
  int ret;
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)url;               /* Unused. Silent compiler warning. */
  (void)method;            /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */
  (void)con_cls;           /* Unused. Silent compiler warning. */

  struct route_args args;
  args.connection = connection;
  args.url = url;
  args.method =method;
  args.version =version;
  args.upload_data =upload_data;
  args.upload_data_size =upload_data_size;
  args.con_cls = con_cls;

  response = route_func(args);
  MHD_destroy_response(response);
  return 1;
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




