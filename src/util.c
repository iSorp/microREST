#include <stdlib.h>
#include <string.h>
#include <regex.h>

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
        return(0);     
    }
    status = regexec(&re, string, (size_t) 0, NULL, 0);
    regfree(&re);
    if (status != 0) {
        return(0);   
    }
    return(1);
}
