#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <bmp280_defs.h>
#include <bmp280i2c.h>

#include "resttools.h"
#include "util.h"
#include "data_stream.h"

/********************** Static function declarations ************************/
static ssize_t 
data_reader_cb(void *cls, uint64_t pos, char *buf , size_t size_max);
static void
data_reader_completed_cb(void *cls);

/****************** User Function  declarations *******************************/
/* 
* Initialize response from callback function
* @param args
* @param sensor
* @param timespan
*/
int
init_data_stream(struct func_args_t *args, const char *sensor, int timespan) {

    struct stream_info_t *info = malloc(sizeof(struct stream_info_t));
    args->con_info->stream_info = info;
    info->connection = args->connection;
    info->status = INIT;
    info->buffer_index = 0;
    info->value_size = 0;
    info->time_span = timespan;
    clock_gettime(CLOCK_MONOTONIC, &info->time_start);
  
    #ifndef NO_SENSOR
        if (strcmp(sensor, "temperature")==0)
            info->sensor_func = &readTemperature;
        else if (strcmp(sensor, "pressure")==0)
            info->sensor_func = &readPressure;
        else {
            free(info);
            return report_error(args->connection, "sensor not found", MHD_HTTP_BAD_REQUEST); 
        }        
    #else
        info->sensor_func = NULL;
    #endif

    // make response from callback
    int ret = 1;
    struct MHD_Response * response;
    response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 1024, &data_reader_cb , info, &data_reader_completed_cb);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, JSON_CONTENT_TYPE);
    ret &= MHD_queue_response(args->connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

/*
* Loop until timespan is reached. Writes the sensor value to a char buffer.
* Signals MHD to process buffered data (MHD_resume_connection)
* @param info
*/
void stream_handler(struct stream_info_t *info) {
    
    // wait until the buffer is processed
    const union MHD_ConnectionInfo *con_info = MHD_get_connection_info(info->connection, MHD_CONNECTION_INFO_CONNECTION_SUSPENDED);
    if (con_info->suspended == MHD_NO) return;

    // Read Sensor value
    double value = 999.9999;
    #ifndef NO_SENSOR
        if (info->sensor_func != NULL)
            value = info->sensor_func();
    #endif 

    // TODO add more values to the buffer 
    memset(info->buffer, 0, MAX_VALUE_DIGITS);
    sprintf(info->buffer, "%.4f,", value);
    info->value_size = strlen(info->buffer);
    
    // stream ends after timespan is reached
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    if (time.tv_sec - info->time_start.tv_sec > info->time_span)
        info->status = END;
    
    MHD_resume_connection(info->connection);
}


/****************** Static Function Definitions *******************************/

/*
* Response callback 
*/
static ssize_t 
data_reader_cb(void *cls, uint64_t pos, char *buf , size_t size_max) { 
    struct stream_info_t *info = cls;

    // initialize state and wait stream handler is resuming
    if (info->status == INIT){
        info->status = RUN;
        MHD_suspend_connection(info->connection);
        return 0;
    }

    if (info->status == END)
        return MHD_CONTENT_READER_END_OF_STREAM; 

    // returning buffer
    if (info->buffer_index < info->value_size) {
        *buf = info->buffer[info->buffer_index++]; 
        return 1;
    }

    // reset buffer index and wait for next value
    info->buffer_index = 0;
    MHD_suspend_connection(info->connection);
    return 0;
}

/*
* Response callback completed
*/
static void
data_reader_completed_cb(void *cls) {
    struct data_reader_info *info = cls;
    if (cls != NULL){
        free(info);
        info = NULL;
    }
}
