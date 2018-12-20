#include <jansson.h>
#include <string.h>
#include <regex.h>
#include "routes.h"
#include "auth.h"


/*
* Regex matching function
*
* @param string 
* @param pattern 
* @return 1 if string matches
*/
int
match(const char *string, char *pattern) {
    int status;
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB|REG_ICASE) != 0) {
        return(0);     
    }
    status = regexec(&re, string, (size_t) 0, NULL, 0);
    regfree(&re);
    if (status != 0) {
        return(0);   
    }
    return(1);
}

/*
* Responses an error message
*
* @param args 
* @param message 
* @param status_code
* @return #MHD_NO on error
*/
int
report_error(struct func_args args, char *message, int status_code) {   
    json_t *j = json_pack("{s:s,s:s}", "status", "error", "message", message);
    char *s = json_dumps(j , 0);
    
    // make response
    int ret = 1;  
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret &= MHD_queue_response(args.connection , status_code, response);
    MHD_destroy_response(response);
    return ret;
}

/*
* Token validation 
*
* @param args
* @param ret resutl of verifing
* @return 1 on valid token
*/
int
verify_token(struct func_args args) {
    int status_code, valid = 1;
    json_t *j;
    char *s;
    const char *auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check wheather Bearer token authentication header is found
    if (auth == NULL || strstr(auth, "Bearer")==NULL) {
        j = json_pack("{s:s,s:s}", "status", "error", "message", "Please send valid bearer token");
        s = json_dumps(j , 0);
        status_code = MHD_HTTP_UNAUTHORIZED;
        valid = 0;
    }
    else {
        char *token = strchr(auth, ' ');
        token = token +1;
        if (0 != validate_token(token)) {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "Unauthorized access");
            s = json_dumps(j , 0);
            status_code = MHD_HTTP_UNAUTHORIZED;
            valid = 0;
        }
    }

    if (0 == valid) {
        // make response
        int ret = 1;
        struct MHD_Response *response;
        response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
        ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
        ret &= MHD_queue_response(args.connection , status_code, response);
        MHD_destroy_response(response);
    }
    return valid;
}

/*
* User basic authentication, returns a token.
* route: /user/auth
* @param args
* @return #MHD_NO on error
*/
int
user_basic_auth(struct func_args args){
    int status_code;
    json_t *j;
    const char *auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check wheather Bearer token authentication header is found
    if (auth == NULL || strstr(auth, "Basic")==NULL) {
        j = json_pack("{s:s,s:s}", "status", "error", "message", "basic authentication required");
        status_code = MHD_HTTP_UNAUTHORIZED;
    } else {
        char *pass = NULL; 
        char *user = MHD_basic_auth_get_username_password (args.connection , &pass);
        if (0 == validate_user(user, pass)) {
            j = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "user authenticated, returning token", "token", generate_token());
            status_code = MHD_HTTP_OK;
        }
        else {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "wrong password");
            status_code = MHD_HTTP_UNAUTHORIZED;
        }
    }

    char *s = json_dumps(j , 0);

    // make response
    int ret = 1;
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret &= MHD_queue_response(args.connection , status_code, response);
    MHD_destroy_response(response);
    return ret;
}

/*
* At / (root) the client can verify if the server responds to requests.
* route: / (root)
* @param args
* @return #MHD_NO on error
*/
int
get_status(struct func_args args){
    if (0 == verify_token(args)) return 1;

    json_t *j = json_pack("{s:s,s:s}", "status", "success", "message", "validation accepted");
    char *s = json_dumps(j , 0);

    // make response
    int ret = 1;
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret &= MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

/*
* This function returns all sensors data as a json string.
* route: /sensor/data
* @param args
* @return #MHD_NO on error
*/
int
get_all_data(struct func_args args){
    if (0 == verify_token(args)) return 1;

    /*---> TODO here read the data <---*/
    char *value1 = "1.123";
    char *value2 = "1234";

    json_t *jd = json_pack("{s:s,s:s}", "sensor 1", value1, "sensor 2", value2);
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "returning data", "data", jd);
    char *s = json_dumps(j , 0);

    // make response
    int ret = 1;
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret &= MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

/*
* This function returns the data of a specified sensor as a json string.
* route: /sensor/data/?id=123abc
* @param args
* @param kvtable
* @return #MHD_NO on error
*/
int
get_data_by_id(struct func_args args) {
    if (0 == verify_token(args)) return 1;

    // read id and validate
    const char *id = MHD_lookup_connection_value(args.connection, MHD_GET_ARGUMENT_KIND, "id");  
    if (NULL== id || 0 == match(id, "[0-9a-zA-Z]+")) {
        return report_error(args, "invalid id", MHD_HTTP_BAD_REQUEST);
    }

    /*---> TODO here read the data <---*/
    char *value = "1.123";

    char buf[256];
    snprintf(buf, sizeof buf, "%s%s", "sensor ", id);

    json_t *jd = json_pack("{s:s}", buf, value);
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "returning data", "data", jd);
    char *s = json_dumps(j , 0);

    // make response
    int ret = 1;
    struct MHD_Response *response;
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret &= MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

/*
* This function sets the threshold of a specified sensor.
* route: /sensor/threshold/?id=123abc&value=123,123
* @param args
* @param kvtable
* @return #MHD_NO on error
*/
int
post_threshold_by_id(struct func_args args){
    if (0 == verify_token(args)) return 1;

    // read the id, value and validate
    const char *id     = MHD_lookup_connection_value(args.connection, MHD_GET_ARGUMENT_KIND, "id");
    const char *value  = MHD_lookup_connection_value(args.connection, MHD_GET_ARGUMENT_KIND, "value");
    if (NULL== id || 0 == match(id, "[0-9a-zA-Z]+")) {
        return report_error(args, "invalid id", MHD_HTTP_BAD_REQUEST);
    }
    if (NULL== value || 0 == match(value, "^[0-9]+([.,][0-9]{1,9})?$")) {
        return report_error(args, "decimal value required (#.#{0,9})", MHD_HTTP_BAD_REQUEST);
    }

    /*---> TODO here save the therhold <---*/

    char buf[256];
    snprintf(buf, sizeof buf, "%s%s", "sensor ", id);

    json_t *jd = json_pack("{s:s}", buf, value);
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "data stored", "data", jd);
    char *s = json_dumps(j , 0);

    // make response
    int ret = 1;
    struct MHD_Response * response;
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret &= MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

/*
*   Route - Function map
*/
struct routes_map rtable[MAX_ROUTES] = {
    { &get_status,             "/",                     "GET"},
    { &user_basic_auth,        "/user/auth/",           "GET"},
    { &get_all_data,           "/sensor/data/",         "GET"},
    { &get_data_by_id,         "/sensor/data",          "GET"},
    { &post_threshold_by_id,   "/sensor/threshold",     "POST"}
};
