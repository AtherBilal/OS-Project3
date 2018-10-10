#include <string.h>
#include <ctype.h>
#include <stdio.h>

int isNumber(char* str) {
  int counter = 0;
  int isDigitFlag = 0;
  int length = strlen(str);
  while (counter < length) {
    isDigitFlag = isdigit(str[counter]);
    if (isDigitFlag == 0) {
      // isdigit returns 0 if it checks a non-zero number
      return 0;
    }
    counter++;
  }
  return 1;
}
