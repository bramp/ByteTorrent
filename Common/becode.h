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

