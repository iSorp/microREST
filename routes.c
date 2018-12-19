#include <jansson.h>
#include "routes.h"
#include "auth.h"

const int MAX_PARAMS = 20;


struct key_value_struct {
    char *key;
    char *value;
}; 
struct key_value_struct key_value[MAX_PARAMS];

int param_counter = 0;


static int 
param_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *value) 
{
    key_value[param_counter].key = key;
    key_value[param_counter].value = value;
    ++param_counter;    
}

struct MHD_Response * 
verify_token(struct route_args args, int *ret) {
    struct MHD_Response * response = NULL;
    json_t *j;
    char *s;
    int status_code;
    *ret = 0;
    char *auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);


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
 * Route:  /user/auth
 * User basic authentication, returns a token.
 */
struct MHD_Response *
user_basic_auth(struct route_args args){
    struct MHD_Response * response;
    json_t *j;
    int status_code;

    const char *basic_auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);
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
 * Route:  / (root)
 * At / (root) the client can verify if the server responds to requests.
 */
struct MHD_Response *
get_status(struct route_args args){
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
 * Route:  /sensor/data
 * This function returns all sensors data as a json string.
 */
struct MHD_Response *
get_all_data(struct route_args args){
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
 * Route:  /sensor/data/<sensorid>
 * This function returns the data of a specified sensor as a json string.
 */
struct MHD_Response *
get_data_by_id(struct route_args args){
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
 * Route:  /sensor/threshold/<sensorid>/<threshold>
 * This function sets the threshold of a specified sensor.
 */
struct MHD_Response *
post_threshold_by_id(struct route_args args){
    struct MHD_Response * response;
    int ret = 0;
    response = verify_token(args, &ret);
    if (0 != ret) return response;

    MHD_get_connection_values(args.connection, MHD_GET_ARGUMENT_KIND, &param_iterator, args.con_cls);

    json_t *jd = json_pack("{s:s,s:s}", "sensor_1", "123", "sensor_2", "345");
    json_t *j = json_pack("{s:s,s:s,s:o}", "status", "success", "message", "returning data", "data", jd);
    char *s = json_dumps(j , 0);

    response = MHD_create_response_from_buffer(strlen(s) , (void *)s, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    MHD_queue_response(args.connection , MHD_HTTP_OK, response);
    return response;
}

struct routes_map rtable[MAX_ROUTES] = {
    { &get_status,             "/" , "GET"},
    { &user_basic_auth,        "/user/auth/", "GET"},
    { &get_all_data,           "/sensor/data/", "GET"},
    { &get_data_by_id,         "/sensor/data/<id>", "GET"},
    { &post_threshold_by_id,   "/sensor/threshold", "POST"}
};
