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
   becode.h version 0.9 by me@bramp.net
   becoding class allows to decode becoded data into a set of beeObjects
*/

#pragma once

class bee {
   public:
      /* Different types of beeObjects */
      enum beeType {
         objectType,
         dictionaryType,
         listType,
         integerType,
         stringType
      };
   
      /* Thrown when invalid encoded data is found */
      class InvalidBeeDataException : public Exception {
         public: InvalidBeeDataException(char *error) { mError = error; };
      };
   
      /* Base Class Of beeCode Objects */
      class beeObject {
         public:
            /* Returns the type of object */
            virtual bee::beeType getType() { return bee::objectType; }; 
            
            /* printf the data */
            virtual void printme(); 
            
            /* Converts the object into a beedata string. Please free me afterwards */
            virtual char *encode(); 
            
            /* Length of the data in encoded form, must be used after encode() */
            int mEncodeLen;
      };
   
      /* Static function to return a beeObject from a beedatastring */
      static beeObject *decode(char *data, int *bytesRead);
   
      /* Dictionary */
      class dictionary : public beeObject {
         private:
            std::map<std::string, bee::beeObject*> dict;
         public:
            dictionary(char *data, int *bytesRead);
            bee::beeType getType() { return bee::dictionaryType; };
            void printme();
            char *encode();
            ~dictionary();
            
            /* Gets a beeObject matched with the key. If not found returns null */
            bee::beeObject *get(char *key);
            
            /* Sets key and beeObject pair */
            void set(char *key, bee::beeObject *value);
            
            /* Removes a beeObject from the list, and returns it. Null if not found */
            bee::beeObject *remove(char *key);
      };

      /* List */      
      class list : public beeObject  {
         private:
            
            class listItem {
               public:
                  listItem *nextPtr;
                  beeObject *data;
            };

            listItem *startPtr;

         public:
            list(char *data, int *bytesRead);
            bee::beeType getType() { return bee::listType; };
            void printme();
            char *encode();
            ~list();
            
            /* Gets the beeObject at specified index */
            bee::beeObject *get(int idx);
            
            /* Adds a beeObject to the list at idx */
            void add(bee::beeObject *value, int idx=0);
            
            /* Sets the beeObject at the specified index */
            void set(int idx, bee::beeObject *value);
            
            /* Removes beeObject at idx and returns it */
            bee::beeObject *remove(int idx);
            
            /* Returns the number of elements in the list */
            int count();

      };

      /* Integer (64 Bits) */      
      class integer : public beeObject  {
         private:
            INT64 mInt;
            
         public:
            integer(char *data, int *bytesRead);
            bee::beeType getType() { return bee::integerType; };
            void printme();
            char *encode();
            
            /* Returns the int */
            INT64 get();
            
            /* Sets the int */
            void set(INT64 value);
      };

      /* String */      
      class string : public beeObject  {
         private:
            char *mString;
            int mLen;

         public:
            string(char *data, int *bytesRead);
            string(char *string, int len);
            bee::beeType getType() { return bee::stringType; };
            void printme();
            char *encode();
            ~string();
            
            /* Returns the string */
            char *get();
            
            /* Sets the string */
            void set(char *newString, int len=-1);
            
            /* Returns the length of the string */
            int getLength();
            
      };
};

