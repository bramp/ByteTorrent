#pragma once

#include <string>
#include <map>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#define UNNAMEDTORRENT "Unnamed"

class Exception {
   public:
      char *mError;
   public:
      Exception() { mError = ""; };
      Exception(char *error) { mError = error; };
      char *getError() { return mError; };
};

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

/* Creats a random string of chars or length len
 [in] chars - Set of valid chars
 [in] len - Length of string
[out] str - The string to get some random chars
*/
void createRandomString(char *str, char *chars, int len);
