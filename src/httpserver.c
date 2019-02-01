#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <bmp280i2c.h>

#include "resttools.h"
#include "data_stream.h"
#include "util.h"
#include "auth.h"


//#define STREAM_THREAD
#define LOCALHOST "127.0.0.1"
#define PORT 8888
#define MAX_POST_CLIENTS 1

static int exit_thread = 0;
static unsigned int nr_of_uploading_clients = 0;

// Connection list
TAILQ_HEAD(head_con_list_t, con_list_entry) head_con_list = TAILQ_HEAD_INITIALIZER(head_con_list);
struct head_con_list_t;               
struct con_list_entry {   
    struct con_info_t *con_info;
    TAILQ_ENTRY(con_list_entry) con_list_entries; 
};

static int 
report_error_max_upload(struct MHD_Connection *connection) {
    char mess[30];
    snprintf(mess, sizeof(mess), "%s%i","max payload in bytes: ", MAX_PAYLOAD);
    return report_error(connection, mess, MHD_HTTP_PAYLOAD_TOO_LARGE);
}

static void *
uri_logger_cb (void *cls, const char *uri) {
    logger(INFO, uri);
  return NULL;
}

static int
accept_policy_cb (void *cls, const struct sockaddr *addr,socklen_t addrlen) {
    #ifdef LOCAL_ONLY
        struct sockaddr_in *ad = (struct sockaddr_in*)addr;
        char *ip = inet_ntoa(ad->sin_addr);
        logger(INFO, "client request");
        logger(INFO, ip);

        if (ad->sin_family == AF_INET) {
            if (strcmp(ip, LOCALHOST)==0)
                return MHD_YES;
        }
        return MHD_NO;
    #else
        return MHD_YES;
    #endif
}

/*
 * Request-end function
 */
static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe)
{
    logger(INFO, "request completed");
    struct con_info_t *con_info = *con_cls;

    // remove list entry
    struct con_list_entry *item;
    TAILQ_FOREACH(item, &head_con_list, con_list_entries) {
        if (item->con_info == con_info) {
            TAILQ_REMOVE(&head_con_list, item, con_list_entries);
            free(item);
            break;
        }
    }

    // free connection info
    if (con_cls != NULL && con_info != NULL) {

        if (con_info->method == PUT)
            nr_of_uploading_clients--;
        if (con_info->data != NULL)
            free(con_info->data);       
        free(con_info);
        con_info->data = NULL;
        con_info = NULL;
    }
}

/*
 *  Assembling data chunks (because Data stays in memory, limit max upload size).  
 */
static int 
process_upload_data(struct con_info_t *con_info, const char *upload_data, int upload_data_size) {
    // prepare memory allocation
    if (con_info->data == NULL) {
        con_info->data = (char*)malloc(0);
    }

    // Check actual data size
    if (con_info->data_size + upload_data_size > MAX_PAYLOAD)
        return 0;
    
    // Copy data to the connection info object
    con_info->data = (char*)realloc(con_info->data, con_info->data_size + upload_data_size + 1);
    memset(con_info->data + con_info->data_size, '\0', upload_data_size + 1);
    memcpy(con_info->data + con_info->data_size, upload_data, upload_data_size);
    con_info->data_size += upload_data_size;
    return 1;
}

/*
 * Request entry function
 */
static int 
answer_to_connection(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{

    // Prepare connection info
    if (NULL == *con_cls) {

        // Initialize connection info
        struct con_info_t *con_info = (struct con_info_t*)malloc(sizeof(struct con_info_t));      
        *con_cls = (void *)con_info;
        con_info->stream_info = NULL;
        con_info->data = NULL;
        con_info->data_size = 0;
        con_info->method = get_enum_method(method);
        con_info->routes_map_index = route_table_index(url);

        // Add connection info to the connection list
        struct con_list_entry *entry = malloc(sizeof(struct con_list_entry));
        TAILQ_INSERT_HEAD(&head_con_list, entry, con_list_entries);
        entry->con_info = con_info;

        // Route not found
        if (con_info->routes_map_index < 0 )
            return report_error(connection, "Route not found", MHD_HTTP_NOT_FOUND);

        // Method not allowed 
        if (0 == (rtable[con_info->routes_map_index].method & con_info->method))
            return report_error(connection, "Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);

        // User authentication
        if (AUTH == rtable[con_info->routes_map_index].auth_type) {
            int valid = 1, res = 0;
            res = user_auth(connection, &valid);
            if (valid == 0) {
                return res;
            }
        }
       
        // Check upload
        const char *cl = MHD_lookup_connection_value(connection , MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_LENGTH);         
        int size = 0;
        if (cl != NULL) size = atoi(cl); 
        if (con_info->method == PUT) 
        {
            if (size > MAX_PAYLOAD)
                report_error_max_upload(connection);

            // Limit max uploading clients
            if (nr_of_uploading_clients >= MAX_POST_CLIENTS) 
                return report_error(connection, "Server is busy", MHD_HTTP_SERVICE_UNAVAILABLE);
            nr_of_uploading_clients++;
        }
        else if (size > 0 ) {
            return report_error(connection, "upload not allowed", MHD_HTTP_BAD_REQUEST);
        }
        
        return MHD_YES;
    }

    // restore connection info
    struct con_info_t *con_info = *con_cls;

    // Data upload
    if (*upload_data_size != 0) {   
        if (con_info->method == PUT) {
            int data_ret = process_upload_data(con_info, upload_data, *upload_data_size);
            *upload_data_size = 0;
            if (!data_ret) 
                return report_error_max_upload(connection);

            // Continue with upload
            return MHD_YES;
        }
        return report_error(connection, "upload not allowed", MHD_HTTP_BAD_REQUEST);
    }

    // Search for route parameters (http://localhost/test{id}/test2{id2})
    char *url_val_buf = strdup(url);
    const char *route_values[MAX_ROUTE_PARAM] = {0};
    if (0 > get_route_values(route_values, url_val_buf, rtable[con_info->routes_map_index].url_pattern)){
        free(url_val_buf);
        return report_error(connection, "too many route parameters", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
        
    // Route function call
    struct func_args_t args = { 
        connection, 
        con_info->method, 
        con_info->data, 
        route_values, 
        con_info};
    int ret = rtable[con_info->routes_map_index].route_func(&args);
    free(url_val_buf);
    return ret;
}

/*
*  Polling for stream events
*/ 
static void
stream_function_bw (void *arg) {
    int count = 0;
    struct con_list_entry *item;
    struct timespec time[] = {{0, 500000000L}};
    while (!exit_thread) {
        count = 0;item = NULL;
        TAILQ_FOREACH(item, &head_con_list, con_list_entries) {
            if (item->con_info->stream_info != NULL && item->con_info->stream_info->status == RUN)
                stream_handler(item->con_info->stream_info);         
            ++count;
        }

        if (count > 0)
            nanosleep(time, NULL); // sleep 500ms for next value
        else
            sleep(1); // sleep a bit longer if no connection is available 
    }
}

int
main (void)
{
    #ifdef SENSOR
        logger(INFO, "run with sensor");
        initI2C();
        initBmc280();
    #else
        logger(INFO, "run without sensor");
    #endif

    regexify_routes();
    TAILQ_INIT(&head_con_list);

    logger(INFO, "run server");
    struct MHD_Daemon *daemon;
  
    daemon = MHD_start_daemon (MHD_ALLOW_SUSPEND_RESUME | MHD_USE_POLL_INTERNAL_THREAD, PORT,
                                &accept_policy_cb, NULL, 
                                &answer_to_connection, NULL,
                                MHD_OPTION_THREAD_STACK_SIZE,  (size_t)PTHREAD_STACK_MIN,
                                MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 30,
                                MHD_OPTION_URI_LOG_CALLBACK, &uri_logger_cb, NULL,
                                MHD_OPTION_NOTIFY_COMPLETED, request_completed,
                                NULL, MHD_OPTION_END);

    if (NULL == daemon)
        return 1;
    
    // Polling for stream events
#ifndef STREAM_THREAD
    stream_function_bw(NULL);   
#else 
    // Polling for stream events in seperate thread
    //posix_memalign(&stack,PAGE_SIZE,STK_SIZE);
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, (size_t)PTHREAD_STACK_MIN);
    pthread_create(&tid, &attr, (void*)stream_function_bw, NULL);
    
    logger(INFO, "start data stream reader");
    (void) getchar ();

    exit_thread = 1;
    pthread_join(tid, NULL);
#endif

    // TODO wait for all connection to be resumed
    // TODO Free allocated regex pattern
    MHD_stop_daemon (daemon);
    logger(INFO, "byby");
    return 0;
}
