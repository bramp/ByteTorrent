#include "stdafx.h"
#include "common.h"

/* Creats a random string of chars or length len
 [in] chars - Set of valid chars
 [in] len - Length of string
[out] str - The string to get some random chars
*/
void createRandomString(char *str, char *chars, int len) {
   
   int charsLen = (int)strlen(chars);
   
   /* Seed the random-number generator */
   srand( (unsigned)time( NULL ) );

   /* Loop len times */
   for (int i=0; i<len; i++) {
      *str = chars[rand() % charsLen];
      str++;
   }
}