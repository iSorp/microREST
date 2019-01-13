#include <string.h>
#include <jansson.h>
#include <bmp280_defs.h>
#include <bmp280i2c.h>

#include "auth.h"
#include "util.h"
#include "resttools.h"
#include "data_stream.h"

/********************** Static function declarations ************************/
static int get_status(struct func_args_t *args);
static int get_user_auth(struct func_args_t *args);
//static int add_user(struct func_args_t *args);
//static int update_user_by_id(struct func_args_t *args);
//static int delete_user_by_id(struct func_args_t *args);
//static int get_users(struct func_args_t *args);
//static int get_user_by_id(struct func_args_t *args);
//static int get_boards(struct func_args_t *args);
//static int get_sensors(struct func_args_t *args);
static int get_sensor_data(struct func_args_t *args);
static int get_data_by_sensor_id(struct func_args_t *args); 
static int set_board_config(struct func_args_t *args);
static int set_board_action(struct func_args_t *args);
static int set_board_mode(struct func_args_t *args);


/********************** Route - Function map ************************/
// Parameter are defined by {id}, {param} usw..

struct routes_map_t rtable[MAX_ROUTES] = {
    { &get_status,              NO_AUTH,    "GET",      "/"},
    { &get_user_auth,           NO_AUTH,    "GET",      "/user/auth"},
    //{ &add_user,                AUTH,       "POST",     "/user/"},
    //{ &update_user_by_id,       AUTH,       "PATCH",    "/user/{id}"},
    //{ &delete_user_by_id,       AUTH,       "DELETE",   "/user/{id}"},
    //{ &get_users,               AUTH,       "GET",      "/user/"},
    //{ &get_user_by_id,          AUTH,       "GET",      "/user/{id}"},
    //{ &get_boards,              NO_AUTH,    "GET",      "/board/"},
    //{ &get_sensors,             NO_AUTH,    "GET",      "/board/{id}/sensor/"},
    { &get_sensor_data,         NO_AUTH,    "GET",      "/board/{id}/sensor/data"},
    { &get_data_by_sensor_id,   NO_AUTH,    "GET",      "/board/{id}/sensor/{sensor}/data"},
    { &set_board_config,        AUTH,       "PUT",      "/board/{id}/config"},
    { &set_board_action,        AUTH,       "PUT",      "/board/{id}/action"},
    { &set_board_mode,          AUTH,       "PUT",      "/board/{id}/mode"},
};

/****************** Static Function Definitions *******************************/

/*
* At / (root) the client can verify if the server responds to requests.
* route: / (root)
* @param args
* @param params
* @return #MHD_NO on error
*/
static int
get_status(struct func_args_t *args) {
    int ret = 1;
 
    // create resource list
    json_t *resources = json_array();
    int i =0;
    while (rtable[i].route_func != NULL){
        json_array_append_new(resources, json_string(rtable[i].url_pattern));  
        ++i;
    }
    
    json_t *j_object = json_pack("{s:s,s:o}", "status", "success", "resources", resources);
    char *message = json_dumps(j_object , 0);
    json_decref(j_object);
    
    if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 
  
    // make response
    ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);
    free(message);
    return ret;
}

/*
* User authentication, returns a token.
* route: /user/auth
* @param args
* @return #MHD_NO on error
*/
static int
get_user_auth(struct func_args_t *args) {
    // user authentication
    int valid = 1, res = 0;
    #ifdef DIGEST_AUTH
        res = digest_auth(args->connection, &valid);
    #endif 
    #ifdef BASIC_AUTH
        res = basic_auth(args->connection, &valid);
    #endif  
    if (valid == 0) {
        return res;
    }
    
    const char *token = generate_token();
    json_t *j_object = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "user authenticated, returning token", "token", token);
    char *message = json_dumps(j_object , 0);
    json_decref(j_object);
    if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 

    // make response
    int ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);
    free(message);
    return ret;
}

/*
* This function returns all sensors data as a json string.
* route: /board/{id}/sensor/data
* @param args
* @return #MHD_NO on error
*/
static int
get_sensor_data(struct func_args_t *args) {

    // read id and validate
    const char *board = args->route_values[0]; 
    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+")) {
        return report_error(args->connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    }
   
    double temp=0, pres=0;
#ifndef NO_SENSOR
    temp = readTemperature();
    pres = readPressure();
#endif

    json_t *jd = json_pack("{s:f,s:f}", "temperature", temp, "pressure", pres);
    json_t *j_object = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "returning data", "board", board, "data", jd);
    char *message = json_dumps(j_object , 0);
    json_decref(j_object);
    if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 

    // make response
    int ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);
    free(message);
    return ret;
}

/*
* This function returns the data of a specified sensor as a json string.
* route: /board/{id}/sensor/{sensor}/data
* @param args
* @return #MHD_NO on error
*/
static int
get_data_by_sensor_id(struct func_args_t *args) {

    const char *board = args->route_values[0]; 
    const char *sensor = args->route_values[1]; 
    const char *timespan = MHD_lookup_connection_value(args->connection , MHD_GET_ARGUMENT_KIND, "timespan");

    // validate parameters
    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+")) 
        return report_error(args->connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (NULL== sensor || 0 == match(sensor, "[0-9a-zA-Z]+"))
        return report_error(args->connection, "invalid sensor id", MHD_HTTP_BAD_REQUEST);
    if (strcmp(board, "bmp280") !=0)
        return report_error(args->connection, "invalid board, currently only bmp280 is available", MHD_HTTP_BAD_REQUEST); 

    // returning data series
    if (timespan != NULL) {
        if (0!=strcmp(args->version, MHD_HTTP_VERSION_1_1))
            return report_error(args->connection, "timespan is not supported by HTTP/1.0", MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED); 
        if (0 == match(timespan, "[0-9]+"))
            return report_error(args->connection, "timespan must be integer", MHD_HTTP_BAD_REQUEST);

        return init_data_stream(args, sensor, atoi(timespan));
    } 
    else
    {
        double value = 0;
        #ifndef NO_SENSOR
            if (strcmp(sensor, "temperature")==0)
                value = readTemperature();
            else if (strcmp(sensor, "pressure")==0)
                value = readPressure();
            else
                return report_error(args->connection, "invalid sensor", MHD_HTTP_BAD_REQUEST); 
        #endif

        json_t *jd = json_pack("{s:f}", sensor, value);
        json_t *j_object = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "returning data", "board", board, "data", jd);
        char *message = json_dumps(j_object , 0);
        json_decref(j_object);
        if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 

        int ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);
        free(message);
        return ret;
    }
}

/*
* This function sets the configuration of a specified board. The mode ist transfered by an post body.
* route: /board/{id}/config
* @param args
* @return #MHD_NO on error
*/
static int
set_board_config(struct func_args_t *args) {
    
    // validate board id
    const char *board = args->route_values[0];
    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+"))
        return report_error(args->connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (strcmp(board, "bmp280") !=0)
        return report_error(args->connection, "invalid board id, currently only bmp280 is available", MHD_HTTP_BAD_REQUEST); 

    // check content type
    const char *content_type = MHD_lookup_connection_value(args->connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
    if (args->upload_data == NULL || strcmp(content_type, JSON_CONTENT_TYPE) != 0)
        return report_error(args->connection, "invalid content! Content-Type: application/json required", MHD_HTTP_BAD_REQUEST); 

    // parse string
    json_t *k;
    int j1, j2, j3, j4, j5;
    k = json_loads(args->upload_data , 0, NULL);  
    int jerror = json_unpack(k, "{s:i,s:i,s:i,s:i,s:i}",
                "os_temp", &j1,
                "os_pres", &j2,
                "odr", &j3,
                "filter", &j4,
                "altitude", &j5      
                );

    // Validate json
    if (0 != jerror) {
        json_decref(k);
        return report_error(args->connection, "Data not valid", MHD_HTTP_BAD_REQUEST);
    } 
    // drop the object reference
    json_decref(k);

    #ifndef NO_SENSOR
        // load configuration, modify and save
        struct bmp280_config conf;
        getConfiguration(&conf);
        conf.os_temp = j1; 
        conf.os_pres = j2;
        conf.odr     = j3;
        conf.filter  = j4;
        //conf.altitude= j5;

        setConfiguration(&conf);
        if (setConfiguration(&conf))
            return report_error(args->connection, "configuration not changed", MHD_HTTP_INTERNAL_SERVER_ERROR); 
    #endif

    json_t *j_object = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "config saved", "board", board);
    char *message = json_dumps(j_object , 0);
    json_decref(j_object);
    if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 

    // make response
    int ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);
    free(message);
    return ret;
}

/*
* This function fires an action to a specified board. The action ist transfered by an url parameter.
* route: /board/{id}/action?action=value
* @param args
* @return #MHD_NO on error
*/
static int
set_board_action(struct func_args_t *args) {

    // read the id, value and validate
    const char *board = args->route_values[0];
    const char *action = MHD_lookup_connection_value(args->connection , MHD_GET_ARGUMENT_KIND, "action");

    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+"))
        return report_error(args->connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (NULL== action || 0 == match(action, "[0-9a-zA-Z]+"))
        return report_error(args->connection, "invalid action", MHD_HTTP_BAD_REQUEST);

#ifndef NO_SENSOR
    if (0==strcmp(action, "reset")){
        if (softReset()!=0) 
            return report_error(args->connection, "reset action failed", MHD_HTTP_INTERNAL_SERVER_ERROR); 
    }
    else {
        return report_error(args->connection, "action not found", MHD_HTTP_BAD_REQUEST); 
    }
#endif

    json_t *jd = json_pack("{s:s}", "action", action);
    json_t *j_object = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "action set", "board", board, "data", jd);
    char *message = json_dumps(j_object , 0);
    if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 
    json_decref(j_object);

    // make response
    int ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);
    free(message);
    return ret;
}

/*
* This function fires a mode to a specified board. The action ist transfered by an url parameter.
* route: /board/{id}/mode?mode=value
* @param args
* @return #MHD_NO on error
*/
static int
set_board_mode(struct func_args_t *args) {

    // read the id, value and validate
    const char *board = args->route_values[0];
    const char *mode = MHD_lookup_connection_value(args->connection , MHD_GET_ARGUMENT_KIND, "mode");

    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+"))
        return report_error(args->connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (NULL== mode || 0 == match(mode, "[0-9]+"))
        return report_error(args->connection, "invalid mode", MHD_HTTP_BAD_REQUEST);

    int imode = atoi(mode);

#ifndef NO_SENSOR
    if (setPowerMode(imode)!=0) 
        return report_error(args->connection, "mode action failed", MHD_HTTP_INTERNAL_SERVER_ERROR); 
#endif

    json_t *jd = json_pack("{s:s}", "mode", mode);
    json_t *j_object = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "mode set", "board", board, "data", jd);
    char *message = json_dumps(j_object , 0);
    if (NULL == message) return report_error(args->connection, "could not create json object", MHD_HTTP_INTERNAL_SERVER_ERROR); 
    json_decref(j_object);

    // make response
    int ret = buffer_queue_response_ok(args->connection, message, JSON_CONTENT_TYPE);;
    free(message);
    return ret;
}