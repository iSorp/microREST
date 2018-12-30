#include "resttools.h"

const char *
generate_token();

int 
validate_user (char *user , char *pass);

int
verify_token(struct MHD_Connection *connection);