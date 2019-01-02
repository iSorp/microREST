#include <string.h>
#include <stdio.h>
#include <jansson.h>

#include "util.h"
#include "auth.h"
#include "resttools.h"

const char *USER = "admin";
const char *PASSWORD = "1234";
const char *TOKEN = "4c078c9db4a85c59f1fd1544122586";


static int 
validate_token(char *token) {
  if (token == NULL)
    return -1;
  return strcmp(TOKEN, token);
}

const char *
generate_token() {
    return TOKEN;
}

int 
validate_user (char *user , char *pass) {
  if (pass == NULL)
    return -1;
  return strcmp(PASSWORD, pass);
}

/*
* Token validation 
*
* @param connection
* @param status resutl of verifing
* @return #MHD_NO on error
*/
int
verify_token(struct MHD_Connection *connection, int *status) {
    int status_code, ret = 1;
    json_t *j;
    char *s;
    *status = 1;
    const char *auth = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check whether bearer token authentication header is found
    if (auth == NULL || strstr(auth, "Bearer")==NULL) {
        j = json_pack("{s:s,s:s}", "status", "error", "message", "Please send valid bearer token");
        status_code = MHD_HTTP_UNAUTHORIZED;
        *status = 0;
    }
    else {
        char *token = strchr(auth, ' ');
        token = token +1;
        if (0 != validate_token(token)) {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "Unauthorized access");
            status_code = MHD_HTTP_UNAUTHORIZED;
            *status = 0;
        }
    }

    if (0 == *status) {
        // make response
        s = json_dumps(j , 0);
        json_decref(j);
        ret = buffer_queue_response(connection, s, JSON_CONTENT_TYPE, status_code);
        free(s);
    }
    else logger(INFO, "token verification ok");
    return ret;
}
