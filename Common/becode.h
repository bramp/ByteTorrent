/*
Copyright (c) 2003, Andrew Brampton (me@bramp.net)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1) Redistributions of source code must retain the above copyright notice, this
   List of conditions and the following disclaimer. 
2) Redistributions in binary form must reproduce the above copyright notice,
   this List of conditions and the following disclaimer in the documentation
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
   becoding class allows to decode becoded data into a set of Objects
*/

#pragma once

class bee {
   public:
      /* Different types of Objects */
      enum beeType {
         objectType,
         DictionaryType,
         ListType,
         IntegerType,
         StringType
      };
   
      /* Thrown when invalid encoded data is found */
      class InvalidBeeDataException : public Exception {
         public: InvalidBeeDataException(char *error) { mError = error; };
      };
   
      /* Base Class Of beeCode Objects */
      class Object {
         public:
            /* Returns the type of object */
            virtual bee::beeType getType() { return bee::objectType; }; 
            
            /* printf the data */
            virtual void printme(); 
            
            /* Converts the object into a beedata String. Please free me afterwards */
            virtual char *encode(); 
            
            /* Length of the data in encoded form, must be used after encode() */
            int mEncodeLen;
      };
   
      /* Static function to return a Object from a beedataString */
      static Object *decode(char *data, int *bytesRead);
   
      /* Dictionary */
      class Dictionary : public Object {
         private:
            std::map<std::string, bee::Object*> dict;
         public:
            Dictionary(char *data, int *bytesRead);
            bee::beeType getType() { return bee::DictionaryType; };
            void printme();
            char *encode();
            ~Dictionary();
            
            /* Gets a Object matched with the key. If not found returns null */
            bee::Object *get(char *key);
            
            /* Sets key and Object pair */
            void set(char *key, bee::Object *value);
            
            /* Removes a Object from the List, and returns it. Null if not found */
            bee::Object *remove(char *key);
      };

      /* List */      
      class List : public Object  {
         private:
            
            class ListItem {
               public:
                  ListItem *nextPtr;
                  Object *data;
            };

            ListItem *startPtr;
            int listCount;

         public:
            List(char *data, int *bytesRead);
            bee::beeType getType() { return bee::ListType; };
            void printme();
            char *encode();
            ~List();
            
            /* Gets the Object at specified index */
            bee::Object *get(int idx);
            
            /* Adds a Object to the List at idx */
            void add(bee::Object *value, int idx=0);
            
            /* Sets the Object at the specified index */
            void set(int idx, bee::Object *value);
            
            /* Removes Object at idx and returns it */
            bee::Object *remove(int idx);
            
            /* Returns the number of elements in the List */
            int count() { return listCount; };

      };

      /* Integer (64 Bits) */      
      class Integer : public Object  {
         private:
            INT64 mInt;
            
         public:
            Integer(char *data, int *bytesRead);
            bee::beeType getType() { return bee::IntegerType; };
            void printme();
            char *encode();
            
            /* Returns the int */
            INT64 get();
            
            /* Sets the int */
            void set(INT64 value);
      };

      /* String */      
      class String : public Object  {
         private:
            char *mString;
            int mLen;

         public:
            String(char *data, int *bytesRead);
            String(char *String, int len = -1);
            bee::beeType getType() { return bee::StringType; };
            void printme();
            char *encode();
            ~String();
            
            /* Returns the String */
            char *get();
            
            /* Sets the String */
            void set(char *newString, int len=-1);
            
            /* Returns the length of the String */
            int getLength();
            
            bool operator < ( const String );
      };
};

