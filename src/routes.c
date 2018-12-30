#include <string.h>
#include <jansson.h>
#ifndef NO_SENSOR
    #include <bmp280_defs.h>
    #include <bmp280i2c.h>
#endif

#include "auth.h"
#include "util.h"

clock_t clock_start, clock_diff;

/********************** Static function declarations ************************/
static int get_status(struct func_args_t args);
static int user_basic_auth(struct func_args_t args);
static int get_sensor_data(struct func_args_t args);
static int get_data_by_sensor_id(struct func_args_t args); 
static int update_board_config(struct func_args_t args);
static int set_board_action(struct func_args_t args);
static int set_board_mode(struct func_args_t args);


/********************** Route - Function map ************************/
// Parameter are defined by using {}

struct routes_map_t rtable[MAX_ROUTES] = {
    { &get_status,              "/",                                "GET"},
    { &user_basic_auth,         "/user/auth",                       "GET"},
    { &get_sensor_data,         "/board/{id}/sensor/data",          "GET"},
    { &get_data_by_sensor_id,   "/board/{id}/sensor/{sensor}/data", "GET"},
    { &update_board_config,     "/board/{id}/config",               "PATCH"},
    { &set_board_action,        "/board/{id}/action",               "PUT"},
    { &set_board_mode,          "/board/{id}/mode",                 "PUT"},
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
get_status(struct func_args_t args) {
    if (0 == verify_token(args.connection)) return 1;

    json_t *j = json_pack("{s:s,s:s}", "status", "success", "message", "validation accepted");
    char *s = json_dumps(j , 0);
    if (NULL == s) return report_error(args.connection, "could not create json", MHD_HTTP_OK); 

#ifndef NO_SENSOR

#endif

    // make response
    return buffer_queue_response_ok(args.connection, s, JSON_CONTENT_TYPE);
}


/*
* User basic authentication, returns a token.
* route: /user/auth
* @param args
* @return #MHD_NO on error
*/
static int
user_basic_auth(struct func_args_t args) {
    int status_code;
    json_t *j;
    const char *auth = MHD_lookup_connection_value(args.connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_AUTHORIZATION);

    // check wheather Bearer token authentication header is found
    if (auth == NULL || strstr(auth, "Basic")==NULL) {
        j = json_pack("{s:s,s:s}", "status", "error", "message", "basic authentication required");
        status_code = MHD_HTTP_UNAUTHORIZED;
        logger(WARNING, "basic authentication required");
    } else {
        char *pass = NULL; 
        char *user = MHD_basic_auth_get_username_password (args.connection , &pass);
        if (0 == validate_user(user, pass)) {
            j = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "user authenticated, returning token", "token", generate_token());
            status_code = MHD_HTTP_OK;
            logger(INFO, "basic auth ok");
        }
        else {
            j = json_pack("{s:s,s:s}", "status", "error", "message", "wrong password");
            status_code = MHD_HTTP_UNAUTHORIZED;
            logger(WARNING, "wrong password");
        }
    }

    char *s = json_dumps(j , 0);

    // make response
    return buffer_queue_response(args.connection, s, JSON_CONTENT_TYPE, status_code);
}

/*
* This function returns all sensors data as a json string.
* route: /board/{id}/sensor/data
* @param args
* @return #MHD_NO on error
*/
static int
get_sensor_data(struct func_args_t args) {
    if (0 == verify_token(args.connection)) return 1;

    // read id and validate
    const char *board = args.route_values[0]; 
    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+")) {
        return report_error(args.connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    }

    double temp=0, pres=0;
#ifndef NO_SENSOR
    temp = readTemperature();
    pres = readPressure();
#endif

    json_t *jd = json_pack("{s:f,s:f}", "temperature", temp, "pressure", pres);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "returning data", "board", board, "data", jd);
    char *s = json_dumps(j , 0);
    if (NULL == s) return report_error(args.connection, "could not create json", MHD_HTTP_OK); 

    // make response
    return buffer_queue_response_ok(args.connection, s, JSON_CONTENT_TYPE);
}


/*static ssize_t data_reader(void *cls, uint64_t pos, char *buf , size_t size_max) { 
    
    clock_diff = clock() - clock_start;
    int msec = clock_diff * 1000 / CLOCKS_PER_SEC;

    if (msec > 2000)
        return MHD_CONTENT_READER_END_OF_STREAM; 

    if (msec % 100 > 90)
        return 0;


    char *b = 'b';
    *buf = 'b';//readTemperature();

    return 1;
}*/

/*
* This function returns the data of a specified sensor as a json string.
* route: /board/{id}/sensor/{sensor}/data
* @param args
* @return #MHD_NO on error
*/
static int
get_data_by_sensor_id(struct func_args_t args) {
    if (0 == verify_token(args.connection)) return 1;

    const char *timespan = MHD_lookup_connection_value(args.connection , MHD_GET_ARGUMENT_KIND, "timespan");

    // read id and validate
    const char *board = args.route_values[0]; 
    const char *sensor = args.route_values[1]; 
    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+")) 
        return report_error(args.connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (NULL== sensor || 0 == match(sensor, "[0-9a-zA-Z]+"))
        return report_error(args.connection, "invalid sensor id", MHD_HTTP_BAD_REQUEST);
    if (strcmp(board, "bmp280") !=0)
        return report_error(args.connection, "invalid board, currently only bmp280 is available", MHD_HTTP_OK); 

    double value = 0;
#ifndef NO_SENSOR
    if (strcmp(sensor, "temperature")==0)
        value = readTemperature();
    else if (strcmp(sensor, "pressure")==0)
        value = readPressure();
    else
        return report_error(args.connection, "invalid sensor", MHD_HTTP_OK); 
#endif

    json_t *jd = json_pack("{s:f}", sensor, value);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "returning data", "board", board, "data", jd);
    char *s = json_dumps(j , 0);
    if (NULL == s) return report_error(args.connection, "could not create json", MHD_HTTP_OK); 

    //clock_start = clock();

    // make response  
    /*int ret = 1;
    struct MHD_Response * response;
    response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 1024, &data_reader , NULL, NULL);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, JSON_CONTENT_TYPE);
    ret &= MHD_queue_response(args.connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);*/
    return buffer_queue_response_ok(args.connection, s, JSON_CONTENT_TYPE);
}

/*
* This function sets the configuration of a specified board. The mode ist transfered by an post body.
* route: /board/{id}/config
* @param args
* @return #MHD_NO on error
*/
static int
update_board_config(struct func_args_t args) {
    if (0 == verify_token(args.connection)) return 1;

    // read the id, value and validate
    const char *board = args.route_values[0];
    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+"))
        return report_error(args.connection, "invalid id", MHD_HTTP_BAD_REQUEST);
    if (strcmp(board, "bmp280") !=0)
        return report_error(args.connection, "invalid board, currently only bmp280 is available", MHD_HTTP_OK); 
    if (args.upload_data == NULL)
        return report_error(args.connection, "invalid payoad", MHD_HTTP_OK); 

    // Process uploaded data
    const char *data = args.upload_data;
    // parse string
    json_t *k = json_loads(data , 0, NULL);
    int8_t os_temp = *json_string_value(json_object_get(k, "os_temp"));
    int8_t os_pres = *json_string_value(json_object_get(k, "os_pres"));
    int8_t odr = *json_string_value(json_object_get(k, "odr"));
    int8_t filter = *json_string_value(json_object_get(k, "filter"));
    int altitude = *json_string_value(json_object_get(k, "altitude"));

#ifndef NO_SENSOR
    // load configuration, modify and save
    struct bmp280_config conf;
    getConfiguration(&conf);

	conf.filter = filter;
	conf.os_pres = os_pres;
	conf.os_temp = os_temp; 
	conf.odr = odr;

    setConfiguration(&conf);
    if (setConfiguration(&conf))
        return report_error(args.connection, "configuration not changed", MHD_HTTP_OK); 
#endif

    json_t *j = json_pack("{s:s,s:s,s:s}", "status", "success", "message", "config saved", "board", board);
    char *s = json_dumps(j , 0);

    // make response
    return buffer_queue_response_ok(args.connection, s, JSON_CONTENT_TYPE);
}

/*
* This function fires an action to a specified board. The action ist transfered by an url parameter.
* route: /board/{id}/action?action=value
* @param args
* @return #MHD_NO on error
*/
static int
set_board_action(struct func_args_t args) {
    if (0 == verify_token(args.connection)) return 1;

    // read the id, value and validate
    const char *board = args.route_values[0];
    const char *action = MHD_lookup_connection_value(args.connection , MHD_GET_ARGUMENT_KIND, "action");

    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+"))
        return report_error(args.connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (NULL== action || 0 == match(action, "[0-9a-zA-Z]+"))
        return report_error(args.connection, "invalid action", MHD_HTTP_BAD_REQUEST);

#ifndef NO_SENSOR
    if (0==strcmp(action, "reset")){
        if (softReset()!=0) 
            return report_error(args.connection, "reset action failed", MHD_HTTP_OK); 
    }
    else {
        return report_error(args.connection, "action not found", MHD_HTTP_OK); 
    }
#endif

    json_t *jd = json_pack("{s:s}", "action", action);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "action set", "board", board, "data", jd);
    char *s = json_dumps(j , 0);

    // make response
    return buffer_queue_response_ok(args.connection, s, JSON_CONTENT_TYPE);
}

/*
* This function fires an mode to a specified board. The action ist transfered by an url parameter.
* route: /board/{id}/mode?mode=value
* @param args
* @return #MHD_NO on error
*/
static int
set_board_mode(struct func_args_t args) {
    if (0 == verify_token(args.connection)) return 1;

    // read the id, value and validate
    const char *board = args.route_values[0];
    const char *mode = MHD_lookup_connection_value(args.connection , MHD_GET_ARGUMENT_KIND, "mode");

    if (NULL== board || 0 == match(board, "[0-9a-zA-Z]+"))
        return report_error(args.connection, "invalid board id", MHD_HTTP_BAD_REQUEST);
    if (NULL== mode || 0 == match(mode, "[0-9]+"))
        return report_error(args.connection, "invalid mode", MHD_HTTP_BAD_REQUEST);

    int imode = atoi(mode);

#ifndef NO_SENSOR
    if (setPowerMode(imode)!=0) 
        return report_error(args.connection, "mode action failed", MHD_HTTP_OK); 
#endif

    json_t *jd = json_pack("{s:s}", "mode", mode);
    json_t *j = json_pack("{s:s,s:s,s:s,s:o}", "status", "success", "message", "mode set", "board", board, "data", jd);
    char *s = json_dumps(j , 0);

    // make response
    return buffer_queue_response_ok(args.connection, s, JSON_CONTENT_TYPE);
}