#include <microhttpd.h>

const char *
generate_token();

int 
validate_user (char *user , char *pass);

/*
* Token validation 
*
* @param connection
* @param status resutl of verifing
* @return #MHD_NO on error
*/
int
verify_token(struct MHD_Connection *connection, int *status);