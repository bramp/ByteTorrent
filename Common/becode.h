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
      enum beeType {
         objectType,
         dictionaryType,
         listType,
         integerType,
         stringType
      };
   
      class InvalidBeeDataException : protected Exception {
         public: InvalidBeeDataException(char *error) { mError = error; };
      };
   
      class beeObject {
         public:
            virtual void printme(); /* printf the data */
            virtual bee::beeType getType() { return bee::objectType; }; /* Returns the type of object */
            virtual char *encode(); /* Converts the object into bee data */
            int mEncodeLen;
      };
   
      static beeObject *decode(char *data, int *bytesRead);
   
      /* Dictionary */
      class dictionary : public beeObject {
         private:
            std::map<std::string, bee::beeObject*> dict;
         public:
            dictionary(char *data, int *bytesRead);
            ~dictionary();
            bee::beeObject *get(char *key);
            void printme();
            bee::beeType getType() { return bee::dictionaryType; };
            char *encode();
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
            ~list();
            void printme();
            bee::beeType getType() { return bee::listType; };
            char *encode();
      };

      /* Integer */      
      class integer : public beeObject  {
         private:
            INT64 mInt;
            
         public:
            integer(char *data, int *bytesRead);
            INT64 getInt();
            void printme();
            bee::beeType getType() { return bee::integerType; };
            char *encode();
      };

      /* String */      
      class string : public beeObject  {
         private:
            char *mString;
            int mLen;

         public:
            string(char *data, int *bytesRead);
            string(char *string, int len);
            ~string();
            char *getString();
            void setString(char *newString, int len=-1);
            int getLength();
            void printme();
            bee::beeType getType() { return bee::stringType; };
            char *encode();
      };
};

