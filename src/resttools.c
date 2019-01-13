#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <jansson.h>

#include "util.h"
#include "resttools.h"

/*
* Creates a response from buffer and queue it to be transmitted to the client.
* @param connection
* @param message
* @param content_type
* @param status_code
* @return #MHD_NO on error
*/
int 
buffer_queue_response(struct MHD_Connection *connection, const char *message, char *content_type, int status_code) {
    // make response
    int ret = 1;
    struct MHD_Response * response;
    response = MHD_create_response_from_buffer(strlen(message) , (void *)message, MHD_RESPMEM_MUST_COPY);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, content_type);
    ret &= MHD_queue_response(connection , status_code, response);
    MHD_destroy_response(response);
    return ret;
}
int 
buffer_queue_response_ok(struct MHD_Connection *connection, const char *message, char *content_type) {
    return buffer_queue_response(connection, message, content_type, MHD_HTTP_OK);
}

/*
* Responses an error message
*
* @param connection 
* @param message 
* @param status_code
* @return #MHD_NO on error
*/
int
report_error(struct MHD_Connection *connection, const char *msg, int status_code) {   
    logger(ERROR, msg);
    json_t *j_object = json_pack("{s:s,s:s}", "status", "error", "message", msg);
    char *message = json_dumps(j_object , 0);
    json_decref(j_object);

    // make response
    int ret = 1;
    struct MHD_Response * response;
    response = MHD_create_response_from_buffer(strlen(message) , (void *)message, MHD_RESPMEM_MUST_COPY);
    ret &= MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, JSON_CONTENT_TYPE);
    ret &= MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    free(message);
    return ret;
}

/**
* Splitts a URL by '/' 
*
* @param parts 
* @param url 
* @param trailing_slash 
* @return actual number of values, on error -1
*/
int
split_url(char **parts, char *url, int *trailing_slash) {
    char *delim = "/";
    int index = 0;
    char *token;
    *trailing_slash = 0;

    int length = strlen(url);
    if (url[length-1] == '/')
        *trailing_slash = 1;

    token = strtok(url, delim);
    if (token != NULL) {
        parts[index++] = token;
        while (token != NULL) { 
            token = strtok(NULL, delim); 
            if (token != NULL)
                parts[index++] = token;
        } 
    }
	return index;
}

/**
* The function finds values in a url defined by the url pattern.
* For example /myroute/{my_param}/otherroute/
* it returns the value of {my_param}
*
* @param values 
* @param url 
* @param pattern 
* @return actual number of values, on error -1
*/
int
get_route_values(const char **values, char* url, const char *pattern) {
   
    int index =0;
    while (*pattern != '\0') {
        if (index >= MAX_ROUTE_PARAM) return -1; // on error

        if (*pattern == PARAM_PREFIX){
            char *value = url;
            // TODO error if no suffix is found
            // pattern: move pointer to the first / or to the end
            while (*pattern != '/' && *pattern != '\0') pattern++;
            while (*url != '/' && *url != '\0') url++;
            if (*pattern != *url)   
                return -1;

            *url = '\0';
            values[index++] = value;
        } else
        {
            ++pattern;
            ++url;
        }
    }
    return index;
}

/**
* Returns the index of the route table on which the url_pattern matches a given url.
* @param url 
* @return -1 on error
*/
int
route_table_index(const char *url) {
    int i;
    for(i=0;i<MAX_ROUTES; i++){
        if (NULL != rtable[i].url_pattern && 1 == match(url, rtable[i].url_regex)) {
            return i;
        }
    }
    return -1;
}

/**
* Creates regex patterns for all routes. The regex patterns are allocated in new memory allocations.
* This Function has to be called only ONCE.
*/
void
regexify_routes(){
    char * rpattern = "[0-9a-zA-Z]*";
    int i, x;
    for(i=0;i<MAX_ROUTES; i++){ 
     
        char *parts[MAX_ROUTE_PARTS];
        char url[MAX_URL_SIZE] = {0};
        snprintf(url, sizeof(url), "%s%s", url, rtable[i].url_pattern);

        // split url in parts escaped by "/"
        int trailing_slash; 
        int count = split_url(parts, url, &trailing_slash);
   
        // search parts containing parameters and replace parameters with regex patterns
        for (x=0; x<count;x++){
            const char prefix[2] = { PARAM_PREFIX, '\0' };
            if (NULL != parts[x] && strstr(parts[x], prefix)!=NULL){
                parts[x] = rpattern;
            }
        }

        // Create a new memory allocation for the Regex URL Pattern
        char *regexified_url = (char*)malloc(MAX_URL_SIZE);
        memset(regexified_url, 0, MAX_URL_SIZE);

        // reassamble route begining with '^' and ending with '$' 
        strncat(regexified_url, "^", 1);
        for (x=0; x<count;x++){
            strncat(regexified_url, "/", 1);
            strncat(regexified_url, parts[x], MAX_URL_SIZE);
        }  
        if (trailing_slash == 1)
            strncat(regexified_url, "/", 1);
        strncat(regexified_url, "$", 1);
        rtable[i].url_regex = regexified_url;
    }
}