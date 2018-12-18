/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <microhttpd.h>

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "urlparser.h"

#define PORT 8888

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




