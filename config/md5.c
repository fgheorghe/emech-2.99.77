#include <stdio.h>
#include <stdlib.h>  
#include <string.h>
#include <unistd.h>
char *crypt(char *, char *);
int main(void)
{
  char *enc;
  enc = crypt("password","\$1\$XX");
  if (enc && !strcmp(enc,"\$1\$XX\$HxaXRcnpWZWDaXxMy1Rfn0")) 
    printf("yes\n");
  exit(0);
}

