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
