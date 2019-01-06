#include <microhttpd.h>

/*
* test function token generation
*
* @return Bearer token
*/
const char *
generate_token();

/*
* Generic authentication 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
user_auth(struct MHD_Connection *connection, int *valid);

/*
* Basic authentication 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
basic_auth(struct MHD_Connection *connection, int *valid);

/*
* Digest authentication 
*
* @param connection
* @param valid
* @return #MHD_NO on error
*/
int
digest_auth(struct MHD_Connection *connection, int *valid);

/*
* Bearer Token validation 
*
* @param connection
* @param status resutl of verifing
* @return #MHD_NO on error
*/
int
bearer_auth(struct MHD_Connection *connection, int *status);




