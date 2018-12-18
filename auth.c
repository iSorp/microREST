#include <string.h>
#include <stdio.h>

const char *USER = "admin";
const char *PASSWORD = "1234";
const char *TOKEN = "4c078c9db4a85c59f1fd1544122586";


int 
validate_user (char *user , char *pass) {
  if (pass == NULL)
    return -1;
  return strcmp(PASSWORD, pass);
}

int 
validate_token(char *token) {
  if (token == NULL)
    return -1;
  return strcmp(TOKEN, token);
}

char *
generate_token(){
    return TOKEN;
}


