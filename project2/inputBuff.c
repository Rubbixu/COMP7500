#include <stdio.h>
#include <string.h>

int inputBuff(){
  char *p, s[100];
  int BUFFER_SIZE;

  printf("PLZ input Buffer Size: ");
  while (fgets(s, sizeof(s), stdin)) {
    BUFFER_SIZE = strtol(s, &p, 10);
    if (p == s || *p != '\n') {
        printf("Invalid input, PLZ input a number greater than 0!\n");
        printf("PLZ input Buffer Size: ");
    } else break;
  }
  return BUFFER_SIZE;
}
