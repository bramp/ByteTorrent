#pragma once

class web {
   public:
      class URL {
         private:
            char *url;
            char *host;
            int port;
            char *path;
            std::map <std::string, std::string> query;
            
         public:
            class InvalidURLException : protected Exception {};
            
            URL(char *pUrl);
            ~URL(void);
            
            const char *getURL();
            const char *getHost() { return host; };
            const char *getPath() { return path; };
            int getPort() { return port; };
            
            void addQuery(char *param, char *value);
            
            static char *encode(unsigned char *data);
            
      };

      class Request {
         
         #define REQUESTBUFFERSIZE 512
         #define REQUESTTIMEOUT 5000
         
         private:
            web::URL *url;
            std::string reply;
            std::string header;
            std::string body;
            
         public:
            class SocketException : protected Exception {};
            class HostNotFoundException : protected Exception {};
            class HostTimeoutException : protected Exception {};
            class TransferException : protected Exception {};
            class InvalidDataException : protected Exception {};
            class ConnectionFailedException : protected Exception {
               public:
                  int WSAError;
                  ConnectionFailedException(int pWSAError) { WSAError = pWSAError; };
            };
            
            Request(web::URL *pUrl);
            void Connect();
            const char *getReply() { return reply.data(); };
            const char *getHeader() { return header.data(); };
            const char *getBody() { return body.data(); };
            ~Request(void);
            
      }; 

};
