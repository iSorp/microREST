#include <string.h>
#include <stdio.h>
#include <jansson.h>

#include "util.h"
#include "auth.h"
#include "resttools.h"

// need be generated for each request
#define MY_OPAQUE_STR "11733b200778ce33060f"

#define USER "admin"
#define PASSWORD "1234"
#define TOKEN "4c078c9db4a85c59f1fd1544122586"


/*
* test function for token validation
*
* @return Bearer token
*/
static int 
verify_token(char *token) {
  if (token == NULL)
    return -1;
  return strcmp(TOKEN, token);
}

/*
* test function for password verifying
*
* @param user
* @param pass
* @return 0 on success
*/
static int 
verify_user_password(char *user , char *pass) {
  if (user == NULL || pass == NULL)
    return -1;
  return strcmp(PASSWORD, pass);
}

/*
* test function token generation
*
* @return Bearer token
*/
const char *
generate_token() {
    return TOKEN;
}

/*
* Generic authentication 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
user_auth(struct MHD_Connection *connection, int *valid) {
    #ifdef BEARER_AUTH
        return bearer_auth(connection, valid);
    #endif
    #ifdef DIGEST_AUTH
        return digest_auth(connection, valid);
    #endif
    #ifdef BASIC_AUTH
        return basic_auth(connection, valid);
    #endif
}

/*
* Basic authentication 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
basic_auth(struct MHD_Connection *connection, int *valid) {
    *valid = 0;
    int ret = 0;
    char *message, *pass;
    struct MHD_Response * response;
    char *username = MHD_basic_auth_get_username_password(connection , &pass);
    
    if (0 != verify_user_password(username, pass)) {
        logger(WARNING, "basic authentication failed");
        json_t *j = json_pack("{s:s,s:s}", "status", "error", "message", "basic authentication failed");
        message = json_dumps(j , 0);
        json_decref(j);
        response = MHD_create_response_from_buffer(strlen(message) ,message ,MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_basic_auth_fail_response(connection, "my_realm", response);
        MHD_destroy_response(response);
        free(message);
        return ret;
    }

    logger(INFO, "basic authentication succeeded");
    *valid = 1;
    return MHD_YES;
}

/*
* Digest authentication 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
digest_auth(struct MHD_Connection *connection, int *valid) {
    struct MHD_Response * response;
    char *username = MHD_digest_auth_get_username(connection);

    // generate response with WWW-Authenticate header
    if (NULL == username) { 
        *valid = 0;
        logger(INFO, "requesting for digest auth");
        json_t *j = json_pack("{s:s,s:s}", "status", "info", "message", "requesting for digest authentication");
        char *message = json_dumps(j , 0);
        json_decref(j);
        response = MHD_create_response_from_buffer(strlen(message), message , MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_auth_fail_response(connection , "my_realm", MY_OPAQUE_STR, response, MHD_NO);
        MHD_destroy_response(response);
        free(message);
        return ret;
    }

    // validate user credentials
    *valid = MHD_digest_auth_check(connection , "my_realm", username, PASSWORD, 300); 
    MHD_free(username);
    if ((*valid==MHD_INVALID_NONCE) || (*valid == MHD_NO)) {
        logger(WARNING, "digest authentication failed");
        json_t *j = json_pack("{s:s,s:s}", "status", "error", "message", "digest authentication failed");
        char *message = json_dumps(j , 0);
        json_decref(j);
        response = MHD_create_response_from_buffer(strlen(message) , message , MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_auth_fail_response(connection , "" , MY_OPAQUE_STR, response , (*valid ==MHD_INVALID_NONCE) ? MHD_YES : MHD_NO);
        MHD_destroy_response(response);
        free(message);
        return ret;
    }
    logger(INFO, "digest authentication succeeded");
    return MHD_YES;
}

/*
* Bearer Token validation 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
bearer_auth(struct MHD_Connection *connection, int *valid) {
    int status_code, ret = 1;
    json_t *j;
    char *s;
    *valid = 1;
    const char *auth = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check whether bearer token authentication header is found
    if (auth == NULL || strstr(auth, "Bearer")==NULL) {
        j = json_pack("{s:s,s:s}", "status", "error", "message", "Please send valid bearer token");
        status_code = MHD_HTTP_FORBIDDEN;
        *valid = 0;
    }
    else {
        char *token = strchr(auth, ' ');
        token = token +1;
        if (0 != verify_token(token)) {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "Unauthorized access");
            status_code = MHD_HTTP_FORBIDDEN;
            *valid = 0;
        }
    }

    if (0 == *valid) {
        // make response
        s = json_dumps(j , 0);
        json_decref(j);
        ret = buffer_queue_response(connection, s, JSON_CONTENT_TYPE, status_code);
        free(s);
    }
    else logger(INFO, "token verification ok");
    return ret;
}
