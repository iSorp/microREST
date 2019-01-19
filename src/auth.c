#include <string.h>
#include <stdio.h>
#include <jansson.h>
#include <microhttpd.h>

#include "util.h"
#include "auth.h"
#include "resttools.h"

// need to be generated for each request
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
        return 0;
    if (0==strcmp(USER, user) && 0==strcmp(PASSWORD, pass))
        return 1;
  return 0;
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
    
    if (1 != verify_user_password(username, pass)) {
        logger(WARNING, "basic authentication failed");
        json_t *j = json_pack("{s:s,s:s}", "status", "error", "message", "basic authentication failed");
        message = json_dumps(j , 0);
        json_decref(j);

        // make response
        response = MHD_create_response_from_buffer(strlen(message) ,message ,MHD_RESPMEM_MUST_COPY);
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

        // make response
        response = MHD_create_response_from_buffer(strlen(message), message , MHD_RESPMEM_MUST_COPY);
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

        // make response
        response = MHD_create_response_from_buffer(strlen(message) , message , MHD_RESPMEM_MUST_COPY);
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
    int ret = 1;
    json_t *j_object;
    *valid = 1;

    // check whether bearer token authentication header is found
    const char *auth = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
    if (auth == NULL || strstr(auth, "Bearer")==NULL) {
        j_object = json_pack("{s:s,s:s}", "status", "error", "message", "Please send valid bearer token");
        *valid = 0;
    }
    else {
        // Verify Token
        char *token = strchr(auth, ' ');
        token = token +1;
        if (0 != verify_token(token)) {
            j_object = json_pack("{s:s,s:s}", "status", "error", "message", "Unauthorized access");
            *valid = 0;
        }
    }

    // generate response with WWW-Authenticate header
    if (0 == *valid) {
        char *message = json_dumps(j_object , 0);
        json_decref(j_object);
        struct MHD_Response * response;
        response = MHD_create_response_from_buffer(strlen(message) ,message, MHD_RESPMEM_MUST_COPY);
        ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, JSON_CONTENT_TYPE);
        ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_WWW_AUTHENTICATE, "Bearer");
        ret &= MHD_queue_response(connection , MHD_HTTP_UNAUTHORIZED, response);
        MHD_destroy_response(response);
        free(message);
    }
    else 
        logger(INFO, "token verification ok");
        
    return ret;
}
