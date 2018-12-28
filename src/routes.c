#include <jansson.h>
#include <string.h>
#include "urltools.h"
#include "auth.h"
#include "util.h"
//#ifndef NO_SENSOR
    #include <bmp280_defs.h>
    #include <bmp280i2c.h>
//#endif

/*
* Responses an error message
*
* @param args 
* @param message 
* @param status_code
* @return #MHD_NO on error
*/
int
report_error(struct func_args_t args, char *message, int status_code) {   
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
verify_token(struct func_args_t args) {
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
* @param params
* @return #MHD_NO on error
*/
int
user_basic_auth(struct func_args_t args, char **params){
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
* @param params
* @return #MHD_NO on error
*/
int
get_status(struct func_args_t args, char **params){
    if (0 == verify_token(args)) return 1;

    json_t *j = json_pack("{s:s,s:s}", "status", "success", "message", "validation accepted");
    char *s = json_dumps(j , 0);
    if (NULL == s) return report_error(args, "could not create json", MHD_HTTP_OK); 

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
* route: /<chip>/data
* @param args
* @param params
* @return #MHD_NO on error
*/
int
get_sensor_data(struct func_args_t args, char **params){
    if (0 == verify_token(args)) return 1;

    // read id and validate
    char *chip = params[0]; 
    if (NULL== chip || 0 == match(chip, "[0-9a-zA-Z]+")) {
        return report_error(args, "invalid chip id", MHD_HTTP_BAD_REQUEST);
    }

    double temp=0, pres=0;
#ifndef NO_SENSOR
    temp = readTemperature();
    pres = readPressure();
#endif

    json_t *jd = json_pack("{s:f,s:f}", "temperature", temp, "pressure", pres);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "returning data", "chip", chip, "data", jd);
    char *s = json_dumps(j , 0);
    if (NULL == s) return report_error(args, "could not create json", MHD_HTTP_OK); 

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
* route: /<chip>/<sensor>/data
* @param args
* @param kvtable
* @param params
* @return #MHD_NO on error
*/
int
get_data_by_sensor_id(struct func_args_t args, char **params) {
    if (0 == verify_token(args)) return 1;

    // read id and validate
    char *chip = params[0]; 
    char *sensor = params[1]; 
    if (NULL== chip || 0 == match(chip, "[0-9a-zA-Z]+")) 
        return report_error(args, "invalid chip id", MHD_HTTP_BAD_REQUEST);
    if (NULL== sensor || 0 == match(sensor, "[0-9a-zA-Z]+"))
        return report_error(args, "invalid sensor id", MHD_HTTP_BAD_REQUEST);
    if (strcmp(chip, "bmp280") !=0)
        return report_error(args, "invalid chip, currently only bmp280 is available", MHD_HTTP_OK); 

    double value = 0;
#ifndef NO_SENSOR
    if (strcmp(sensor, "temperature")==0)
        value = readTemperature();
    else if (strcmp(sensor, "pressure")==0)
        value = readPressure();
    else
        return report_error(args, "invalid sensor", MHD_HTTP_OK); 
#endif

    json_t *jd = json_pack("{s:f}", sensor, value);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "returning data", "chip", chip, "data", jd);
    char *s = json_dumps(j , 0);
    if (NULL == s) return report_error(args, "could not create json", MHD_HTTP_OK); 

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
* This function sets the configuration of a specified chip. The mode ist transfered by an post body.
* route: /<chip>/config
* @param args
* @param kvtable
* @param params
* @return #MHD_NO on error
*/
int
post_chip_config(struct func_args_t args, char **params){
    if (0 == verify_token(args)) return 1;

    // read the id, value and validate
    const char *chip = params[0];
    if (NULL== chip || 0 == match(chip, "[0-9a-zA-Z]+"))
        return report_error(args, "invalid id", MHD_HTTP_BAD_REQUEST);
    if (strcmp(chip, "bmp280") !=0)
        return report_error(args, "invalid chip, currently only bmp280 is available", MHD_HTTP_OK); 


    // Process uploaded data
    const char *data = args.upload_data;
    // parse string
    json_t *k = json_loads(data , 0, NULL);
    json_t *jv = json_object_get(k, "config");
    //if (1 != json_is_real(jv))
    //    return report_error(args, "double value required", MHD_HTTP_BAD_REQUEST);
    
    char* value1 = json_string_value(json_object_get(jv, "value1"));
    char* value2 = json_string_value(json_object_get(jv, "value2"));
    char* value3 = json_string_value(json_object_get(jv, "value3"));

#ifndef NO_SENSOR
    // load configuration, modify and save
    struct bmp280_config conf;
    getConfiguration(&conf);
    /*conf.dummy = 1;
    conf.dummy = 2;
    conf.dummy = 3;*/
    setConfiguration(&conf);
    if (setConfiguration(&conf))
        return report_error(args, "configuration not changed", MHD_HTTP_OK); 
#endif

    json_t *j = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "config saved", "chip", chip);
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
* This function sets the mode of a specified chip. The mode ist transfered by an url parameter.
* route: /<chip>/mode?modeid=someid
* @param args
* @param kvtable
* @return #MHD_NO on error
*/
int
post_chip_mode(struct func_args_t args, char **params){
    if (0 == verify_token(args)) return 1;

    // read the id, value and validate
    const char *chip = params[0];
    const char *mode = MHD_lookup_connection_value(args.connection , MHD_GET_ARGUMENT_KIND, "power");

    if (NULL== chip || 0 == match(chip, "[0-9a-zA-Z]+"))
        return report_error(args, "invalid chip id", MHD_HTTP_BAD_REQUEST);
    if (NULL== mode || 0 == match(mode, "[0-9a-zA-Z]+"))
        return report_error(args, "invalid mode", MHD_HTTP_BAD_REQUEST);
    if (strcmp(chip, "bmp280") !=0)
        return report_error(args, "invalid chip, currently only bmp280 is available", MHD_HTTP_OK); 

#ifndef NO_SENSOR
    if (setPowerMode(mode)!=0) 
        return report_error(args, "mode selection failed", MHD_HTTP_OK); 
#endif

    json_t *jd = json_pack("{s:s}", "mode", mode);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "mode changed", "chip", chip, "data", jd);
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
struct routes_map_t rtable[MAX_ROUTES] = {
    { &get_status,             "/",                       "GET"},
    { &user_basic_auth,        "/user/auth",              "GET"},
    { &get_sensor_data,        "/<chip>/data",            "GET"},
    { &get_data_by_sensor_id,  "/<chip>/<sensor>/data",   "GET"},
    { &post_chip_config,       "/<chip>/config",          "POST"},
    { &post_chip_mode,         "/<chip>/mode",            "POST"},
};
