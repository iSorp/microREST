#include <stdlib.h>
#include <stdio.h> 
#include <string.h>

#include "urltools.h"
#include "util.h"

/**
* Splitts a URL by '/' 
*
* @param parts 
* @param path 
* @param max_parts 
* @return actual number of values, on error -1
*/
int
split_url(char **parts, char *path, int *trailing_slash)
{
    char *delim = "/";
    int index = 0;
    char *token;
    *trailing_slash = 0;

    int length = strlen(path);
    if (path[length-1] == '/')
        *trailing_slash = 1;

    token = strtok(path, delim);
    if (token != NULL) {
        memset(&parts[0], 0, sizeof(parts));
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
* Finds values in a url defined by the url pattern
*
* @param values 
* @param pattern 
* @param url 
* @param max_values 
* @return actual number of values, on error -1
*/
int
find_values(char **values, const char *pattern, char* url, int max_values){
   
    int i, index =0;
    while (*pattern != '\0') {
        if (index >= max_values) return -1; // on error

        if (*pattern == '<'){
            char *value = url;
            while (*pattern != '>' && *pattern != '\0') pattern++;
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
* Creates rexgex patterns for all routes. The routes are allocated in new memory locations.
* This Function has to be called only ONCE.
*/
void
regexify_routes(){
    int i, x;
    for(i=0;i<MAX_ROUTES; i++){ 
     
        char *parts[100];
        char url[MAX_URL_SIZE];
        memset(&url[0], 0, sizeof(url));
        snprintf(url, sizeof(url), "%s%s", url, rtable[i].url);

        int trailing_slash;
        int count = split_url(parts, url, &trailing_slash);
   
        for (x=0; x<count;x++){
            if (NULL != parts[x] && strstr(parts[x], "<")!=NULL){
                parts[x] = "[0-9a-zA-Z]*";
            }
        }

        // Create memory allocation for the new Regex URL Pattern
        char *regexified_url = (char*)malloc(255);
        strncat(regexified_url, "^", 1);
        for (x=0; x<count;x++){
            strncat(regexified_url, "/", 1);
            strncat(regexified_url, parts[x], 255);
        }  
        if (trailing_slash == 1)
            strncat(regexified_url, "/", 1);
        strncat(regexified_url, "$", 1);
        rtable[i].regexurl = regexified_url;
    }
}

/**
 * Calling a function depending on the given URL
 *
 * @param args 
 * @return #MHD_NO on error
 */
int
route_func(struct func_args_t args){
    int i;
    for(i=0;i<MAX_ROUTES; i++){
        if (NULL != rtable[i].url && 1 == match(args.url, rtable[i].regexurl)) {
            if (0 == strcmp(rtable[i].method, args.method)) {
                char url_a[MAX_URL_SIZE];
                char *values[MAX_URL_PARAM];
                
                // Copy url to char array for modification
                memset(&url_a[0], 0, sizeof(url_a));
                snprintf(url_a, sizeof(url_a), "%s%s", url_a, args.url);
                find_values(values, rtable[i].url, url_a, MAX_URL_PARAM);

                return rtable[i].fctptr(args, values);
            }
            else
                return report_error(args, "Method not allowed", MHD_HTTP_METHOD_NOT_ALLOWED);
        }
    }
    return report_error(args, "Route not found", MHD_HTTP_NOT_FOUND);
}