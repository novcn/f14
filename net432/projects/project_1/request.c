#include <wchar.h>
#include <string.h>
#include "duckchat.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "raw.h"
void *mempcpy(void *dest, const void *src, size_t n);


int main(void){ 

  const char * TKN_DELIM = " \n";
  char * input = (char*)malloc(sizeof(char*) * SAY_MAX + 1);
  *((char*)mempcpy(input, "/join channel", strlen("/join channel"))) = '\0';

  char * one = strtok(input, TKN_DELIM);
  char * two = strtok(NULL, TKN_DELIM);

  printf("one: \'%s\'\n", one);
  printf("two: \'%s\'\n", two);

  return 0;

}

