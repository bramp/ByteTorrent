/*
Copyright (c) 2003, Andrew Brampton (me@bramp.net)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1) Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2) Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 
3) Neither the name of the ByteTorrent nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission. 
   
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* 
   becode.cpp version 0.9 by me@bramp.net
   becoding class allows to en/decode becoded data into a set of beeObjects
*/

#include "stdafx.h"
#include "becode.h"

/* These functions should never be called */
void bee::beeObject::printme() {}
char *bee::beeObject::encode() {return NULL;}

/******************************************************************************
Generic Encode/Decode Code Starts Here
******************************************************************************/

/* Creates the require beeObject depending on data 
 [in] data - The bee data ie "i100e"
[out] bytesRead - The number of bytes read from the data stream ie 5
*/
bee::beeObject *bee::decode(char *data, int *bytesRead) {
   bee::beeObject *newItem = NULL;
   
   /* Switch the type of beedata and create correct beeObject */
   switch (*data) {
      case 'i':
         newItem = new bee::integer(data, bytesRead);
         break;
      case '1': case '2':  case '3': case '4': case '5':
      case '6': case '7':  case '8': case '9':
         newItem = new bee::string(data, bytesRead);
         break;
      case 'l':
         newItem = new bee::list(data, bytesRead);
         break;
      case 'd':
         newItem = new bee::dictionary(data, bytesRead);
         break;
      default:
         throw InvalidBeeDataException("Invalid Becoded Type");
      }
   return newItem;
}

/******************************************************************************
Integer Object Starts Here
******************************************************************************/

/* Creates a integer object from bee data.
 [in] data - The bee data ie "i100e"
[out] bytesRead - The number of bytes read from the data stream ie 5
*/
bee::integer::integer(char *data, int *bytesRead) {
   
   char *beginData;
   char cInt[21];
   
   *bytesRead = 0;
   
   /* Check the data starts with a i */
   if (*data != 'i')
      throw InvalidBeeDataException("Invalid Integer Data");

   /* Start after the i */
   data++;
   beginData = data;
   (*bytesRead)++;
   
   /* Loop until we find a e or we have looped 11 times */
   /* I chose the limit of 11 since a int can't hold much more ;) */
   while ((*data != 'e') && (*bytesRead < sizeof(cInt))) {
      /* Check for valid chars */
      if (((*data < '0') || (*data > '9')) && (*data != '-'))
         throw InvalidBeeDataException("Invalid Integer Data");

      /* Increments the bytes read, and current string position */
      (*bytesRead)++;
      data++;
   }
   
   /* Copy the int into a char[] and then atoi it */
   memcpy(cInt, beginData, *bytesRead - 1);
   cInt[*bytesRead - 1] = '\0';
   mInt = _atoi64(cInt);
   
   /* Move pass the e */
   (*bytesRead)++;
}

/* Returns a int value of ourself */
INT64 bee::integer::getInt() {
   return mInt;
}

/* Print a int value of ourself */
void bee::integer::printme() {
   printf("%i", mInt);
}

/* Creates bee data from the object */
char *bee::integer::encode() {
   char *ret;
   
   ret = (char *)malloc(23); //Max size a int + 2 can be
   sprintf(ret, "i%I64ie", mInt);
   
   mEncodeLen = (int)strlen(ret);
   
   return ret;
}

/******************************************************************************
String Object Starts Here
******************************************************************************/

/* Creates a string object from bee data.
 [in] data - The bee data ie "4:spam"
[out] bytesRead - The number of bytes read from the data stream ie 6
*/
bee::string::string(char *data, int *bytesRead) {
   
   char *beginData = data;
   char cInt[11];
   
   /* Initalise member vars */
   mString = NULL;
   mLen = 0;
   *bytesRead = 0;
   
   /* Loops until we find a : or read a number too big for us */
   while ((*data!=':') && (*bytesRead < sizeof(cInt))) {
      if ((*data < '0') && (*data > '9'))
         throw InvalidBeeDataException("Invalid String Data");
         
      data++;
      (*bytesRead)++;
   }
   
   /* Check there is atleast one number */
   if (*bytesRead==0)
      throw InvalidBeeDataException("Invalid String Data");

   /* Read past the : */
   data++;

   /* Copy the len into a char[] so we can atoi it */
   memcpy(cInt, beginData, *bytesRead);
   cInt[*bytesRead] = '\0';
   
   mLen = atoi(cInt);

   /* Move one more */
   (*bytesRead)++;

   /* Malloc us some space */
   mString = (char *)malloc(mLen + 1);
   
   memcpy(mString, data, mLen);
   mString[mLen] = '\0';
   
   (*bytesRead)+=mLen;
   
}

bee::string::string(char *string, int len) {
   mLen = len;
   mString = (char *)malloc(mLen + 1);
   memcpy(mString, string, len+1);
}

/* Destory a string */
bee::string::~string() {
   if (mString != NULL)
      free(mString);
}

/* Returns a string value of ourself */
char *bee::string::getString() {
   return mString;
}

void bee::string::setString(char *newString, int len) {
   
   /* Free old string */
   if (mString != NULL)
      free(mString);   
      
   if (len==-1) {
      len = (int)strlen(newString);
   }
   
   mLen = len;
   mString = (char *)malloc(mLen + 1);
   strcpy(mString, newString);
}

/* Returns the length of ourself */
int bee::string::getLength() {
   return mLen;
}

/* Print a string value of ourself */
void bee::string::printme() {
   char *str;
   
   printf("'");
   str = mString;
   
   for (int i=0; i<mLen; i++) {
      printf("%c", *str);
      str++;
   }
   
   printf("'");
}

/* Creates bee data from the object */
char *bee::string::encode() {
   char *ret;
   char *beginRet;
   char len[12];
   
   itoa(mLen, len, 10);
   
   mEncodeLen = mLen + 1 + (int)strlen(len);
   
   ret = (char *)malloc(mEncodeLen + 1);
   beginRet = ret;
   
   sprintf(ret, "%s:", len, mString);
   
   ret += strlen(ret);
   memcpy(ret, mString, mLen);
   ret += mLen;
   *ret = '\0';
   
   return beginRet;
}
/******************************************************************************
List Object Starts Here
******************************************************************************/

/* Creates a list object from bee data.
 [in] data - The bee data ie "l4:spam4:eggse"
[out] bytesRead - The number of bytes read from the data stream ie 14
*/
bee::list::list(char *data, int *bytesRead) {
  
   int bytes;
   listItem *lastPtr = NULL;
   
   /* Initilise some varibles */
   startPtr = NULL;
   *bytesRead = 0;
   
   /* Check the data starts with a l */
   if (*data != 'l')
      throw InvalidBeeDataException("Invalid List Data");

   data++;
   (*bytesRead)++;
   
   /* Loop until we find a e */
   /* *NOTE POSSIBLE INFINITE LOOP* */
   while (*data!='e') {
      listItem *item;
      
      /* Create a new listItem */
      item = new listItem();
      
      /* Add element onto end of the list or begining of the list*/
      if (lastPtr == NULL) {
         startPtr = item;
      } else {
         lastPtr->nextPtr = item;
      }
      
      /* Populate the new list item */
      item->nextPtr = NULL;
      item->data = bee::decode(data, &bytes);
      
      /* Record this item */
      lastPtr = item;
      data += bytes;
      (*bytesRead) += bytes;
   }
   
   /* Count the e */
   (*bytesRead)++;
}

/* Destory a list */
bee::list::~list() {

   /* Iterate the list destorying it */
   while (startPtr!=NULL) {
      listItem *tmpItem = startPtr->nextPtr;
      delete startPtr->data;
      delete startPtr;
      startPtr = tmpItem;
   }
      
}

/* Print a list of ourself */
void bee::list::printme() {
   listItem *next;
   
   next = startPtr;
   printf("[");
   while (next!=NULL) {
      next->data->printme();
      next = next->nextPtr;
      if (next!=NULL)
         printf(", ");
   }
   printf("]");
}

/* Creates bee data from the object */
char *bee::list::encode() {
   
   listItem *nextItem;
   std::string data;
   char *ret;

   /* Start the data off */   
   data = "l";
   nextItem = startPtr;
   
   /* Iterate the list */
   while (nextItem!=NULL) {
      char *value;
      
      /* Get the encoded data for this object */
      value = nextItem->data->encode();
      
      /* Now append to our string */
      data.append(value, nextItem->data->mEncodeLen);
      
      /* Free the return char array */
      free(value);
      
      /* And move on */
      nextItem = nextItem->nextPtr;
   }
   
   data += "e";
   
   mEncodeLen = (int)data.length();
   
   /* Malloc enough memory to return and copy data in */
   ret = (char *)malloc(mEncodeLen + 1);
   memcpy(ret, data.data(), mEncodeLen + 1);
   
   return ret;
}

/******************************************************************************
Dictionary Object Starts Here
******************************************************************************/

/* Creates a dictionary object from bee data.
 [in] data - The bee data ie "d3:cow3:moo4:spam4:eggse"
[out] bytesRead - The number of bytes read from the data stream ie 24
*/
bee::dictionary::dictionary(char *data, int *bytesRead) {

   int bytes = 0;
   bee::string *key;
   bee::beeObject *value;

   *bytesRead = 0;

   /* Check the data starts with a d */
   if (*data != 'd')
      throw InvalidBeeDataException("Invalid Dictionary Data");
   
   data++;
   (*bytesRead)++;
   
   /* Loop until the end of the dictionary */
   while (*data != 'e') {
      
      /* Read the key which should be a string */
      key = (bee::string *) bee::decode(data, &bytes);
      
      if (key->getType()!=bee::stringType)
         throw InvalidBeeDataException("Invalid Key");
            
      /* Move past the string */
      data += bytes;
      (*bytesRead) += bytes;
      
      /* Now read the value of type beeObject */
      value = bee::decode(data, &bytes);
      
      /* Move past the object */
      data += bytes;
      (*bytesRead) += bytes;
      
      /* Place this info into the map */
      dict[key->getString()] = value;
      
      /* Clean up the key since we don't see it anymore */
      delete key;
   }
   
   (*bytesRead)++;
 
}

/* Destory a dictionary */
bee::dictionary::~dictionary() {
   bee::beeObject *value;
   std::map<std::string, bee::beeObject*>::iterator dictIter;
      
   for ( dictIter = dict.begin(); dictIter != dict.end(); dictIter++ ) {

      /* Retreive the value */      
      value = dictIter->second;
      
      /* and delete it */
      delete value;
   }
}

/* Gets the value associated with the key */
bee::beeObject *bee::dictionary::get(char *key) {
   return dict[key];
}

/* Print a dictionary of ourself */
void bee::dictionary::printme() {
   std::string key;
   bee::beeObject *value;
   std::map<std::string, bee::beeObject*>::iterator dictIter;
   
   printf("{\n");
   
   /* Correct a iterator to get all elements from map */
   dictIter = dict.begin();
   while ( dictIter != dict.end() ) {

      /* Retreive the key and the value */      
      key = dictIter->first;
      value = dictIter->second;
      
      /* Print out some stuff */
      printf("'%s':", key.data());
      value->printme();
      
      /* Move along one */
      dictIter++;
      
      /* Print a , if we aren't at the end */
      if (dictIter != dict.end())
         printf(",\n");
      else
         printf("\n");
   }
   
   printf("}");
}

/* Creates bee data from the object */
char *bee::dictionary::encode() {
   std::string key;
   bee::string *keyString;
   bee::beeObject *value;
   std::map<std::string, bee::beeObject*>::iterator dictIter;
   std::string data;
   char *ret;
   
   data = "d";
   
   /* Correct a iterator to get all elements from map */
   dictIter = dict.begin();
   while ( dictIter != dict.end() ) {

      char *tmpChar;

      /* Retreive the key and the value */      
      key = dictIter->first;
      value = dictIter->second;
      
      /* Make the key into a string */
      keyString = new bee::string((char*)key.data(), (int)key.length());
      
      /* Get, append and free returned key data */
      tmpChar = keyString->encode();
      data.append(tmpChar, keyString->mEncodeLen);
      free(tmpChar);
      
      /* Get, append and free returned value data */
      tmpChar = value->encode();

      data.append(tmpChar, value->mEncodeLen);
      
      free(tmpChar);
      
      delete keyString;
      
      dictIter++;
   }
   
   data += "e";
   
   /* Get the len of encoded data */
   mEncodeLen = (int)data.length();
   
   /* Create a char * long enough, copy data in, and append null */
   ret = (char *)malloc(mEncodeLen + 1);
   memcpy(ret, data.data(), mEncodeLen + 1);
   
   return ret;
}