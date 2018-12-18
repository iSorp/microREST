#include <jansson.h>
#include <string.h>
#include <stdio.h>
#include "urlparser.h"
#include "routes.h"


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