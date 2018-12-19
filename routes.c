#include <jansson.h>
#include "routes.h"
#include "auth.h"

/*
* @param args
* @param ret resutl of verifing
* @return MHD_Response
*/
struct MHD_Response * 
verify_token(struct func_args args, int *ret) {
    struct MHD_Response * response;
    json_t *j;
    char *s;
    int status_code;
    *ret = 0;
    const char *auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check wheather Bearer token authentication header is found
    if (auth == NULL || strstr(auth, "Bearer")==NULL) {
        j = json_pack("{s:s,s:s}", "status", "error", "message", "Please send valid bearer token");
        status_code = MHD_HTTP_UNAUTHORIZED;
        *ret = 1;
    }
    else {
        char *token = strchr(auth, ' ');
        token = token +1;
        if (0 != validate_token(token)) {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "Unauthorized access");
            *ret = 1;
        }
    }

    if (0 != *ret) {
        status_code = MHD_HTTP_UNAUTHORIZED;
        s = json_dumps(j , 0);
        response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
        MHD_queue_response(args.connection , status_code, response);
    }
    return response;
}

/*
* User basic authentication, returns a token.
* route: /user/auth
* @param args
* @return MHD_Response
*/
struct MHD_Response *
user_basic_auth(struct func_args args){
    struct MHD_Response * response;
    json_t *j;
    int status_code;
    const char *basic_auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check wheather Bearer token authentication header is found
    if (basic_auth != NULL) {
        char *pass = NULL; 
        char *user = MHD_basic_auth_get_username_password (args.connection , &pass);
        if (0 == validate_user(user, pass)) {
            j = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "User authenticated, returning token", "token", generate_token());
            status_code = MHD_HTTP_OK;
        }
        else {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "Wrong password");
            status_code = MHD_HTTP_UNAUTHORIZED;
        }
    }
    else {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "Please send basic authentication");
            status_code = MHD_HTTP_UNAUTHORIZED;
    }
    char *s = json_dumps(j , 0);

    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    return response;
}

/*
* At / (root) the client can verify if the server responds to requests.
* route: / (root)
* @param args
* @return MHD_Response
*/
struct MHD_Response *
get_status(struct func_args args){
    struct MHD_Response * response;
    int ret = 0;
    response = verify_token(args, &ret);
    if (0 != ret) return response;

    json_t *j = json_pack("{s:s,s:s}", "status", "success", "message", "Validation accepted");
    char *s = json_dumps(j , 0);

    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    return response;
}

/*
* This function returns all sensors data as a json string.
* route: /sensor/data
* @param args
* @return MHD_Response
*/
struct MHD_Response *
get_all_data(struct func_args args){
    struct MHD_Response * response;
    int ret = 0;
    response = verify_token(args, &ret);
    if (0 != ret) return response;

    json_t *jd = json_pack("{s:s,s:s}", "sensor_1", "123", "sensor_2", "345");
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "returning data", "data", jd);
    char *s = json_dumps(j , 0);

    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    return response;
}

/*
* This function returns the data of a specified sensor as a json string.
* route: /sensor/data/?sensorid=id
* @param args
* @param kvtable
* @return MHD_Response
*/
struct MHD_Response *
get_data_by_id(struct func_args args, struct key_value_args *kvtable){
    struct MHD_Response * response;
    int ret = 0;
    response = verify_token(args, &ret);
    if (0 != ret) return response;

    json_t *jd = json_pack("{s:s,s:s}", "sensor_1", "123", "sensor_2", "345");
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "returning data", "data", jd);
    char *s = json_dumps(j , 0);

    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    return response;
}

/*
* This function sets the threshold of a specified sensor.
* route: /sensor/threshold/?sensorid=id&threshold=value
* @param args
* @param kvtable
* @return MHD_Response
*/
struct MHD_Response *
post_threshold_by_id(struct func_args args){
    struct MHD_Response * response;
    int ret = 0;
    response = verify_token(args, &ret);
    if (0 != ret) return response;

    json_t *jd = json_pack("{s:s,s:s}", "sensor_1", "123", "sensor_2", "345");
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "returning data", "data", jd);
    char *s = json_dumps(j , 0);

    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    return response;
}

/*
*   Route - Function map
*/
struct routes_map rtable[MAX_ROUTES] = {
    { &get_status,             "/" , "GET"},
    { &user_basic_auth,        "/user/auth/", "GET"},
    { &get_all_data,           "/sensor/data/", "GET"},
    { &get_data_by_id,         "/sensor/data", "GET"},
    { &post_threshold_by_id,   "/sensor/threshold", "POST"}
};
