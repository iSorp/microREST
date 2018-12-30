
/*
* Logging types
*/
enum tag_e {
    ERROR,
    WARNING,
    INFO
};

/*
* Simple logger for printing a message
*
* @param tag 
* @param message 
*/
void 
logger(enum tag_e tag, const char *message);

/*
* Regex matching function
*
* @param string 
* @param pattern 
* @return 1 if string matches
*/
int
match(const char *string, const char *pattern);
