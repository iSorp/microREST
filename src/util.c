#include <stdio.h>
#include <string.h>
#include <time.h>
#include <regex.h>

#include "util.h"

/*
* Regex matching function
*
* @param string 
* @param pattern 
* @return 1 if string matches
*/
int
match(const char *string, const char *pattern) {
    int status;
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB|REG_ICASE) != 0) {
        return 0;     
    }
    status = regexec(&re, string, (size_t) 0, NULL, 0);
    regfree(&re);
    if (status != 0) {
        return 0;   
    }
    return 1;
}

/*
* Simple logger for printing a message
*
* @param tag 
* @param message 
*/
void 
logger(enum tag_e tag, const char *message) {
    char *tag_name;
     switch (tag){
         case ERROR:
            tag_name = "error";
            break;
        case WARNING:
            tag_name = "warning";
            break;
        case INFO:
            tag_name = "info";
            break;
     }

    time_t now;
    time(&now);
    char *time = ctime(&now);
    //time[strlen(ctime(&now))-1] = '\0';
    printf("%s [%s]: %s\n", time, tag_name, message);
}