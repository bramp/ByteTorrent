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
   web.cpp version 0.9 by me@bramp.net
   Simple HTTP Class built for ByteTorrent
*/

#include "stdafx.h"
#include "web.h"

/******************************************************************************
URL Object Starts Here
******************************************************************************/
Web::URL::URL(char *pUrl) {

   char *ptr;
   int len;
   int tmpLen;
   char *token;

   host = NULL;
   port = 80;
   path = NULL;
   
   /* Check if the URL begins with http:// */
   if (strnicmp(pUrl, "http://", 7) != 0) {
      throw InvalidURLException();
   }
   
   /* Create space and copy the URL */
   len = (int)strlen(pUrl);
   url = (char *)malloc( len+ 1);
   strcpy(url, pUrl);
   
   /* Skip past the http:// */
   ptr = url + 7;
   
   /* Find where the host name ends */
   len = (int) strlen(ptr);
   tmpLen = (int)(strchr(ptr, '/') - ptr);
   
   if ((tmpLen > 0) && (tmpLen < len))
      len = tmpLen;
   
   tmpLen = (int)(strchr(ptr, ':') - ptr);
   
   if ((tmpLen > 0) && (tmpLen < len))
      len = tmpLen;
   
   host = (char *)malloc(len + 1);
   
   /* Now copy that hostname */
   strncpy(host, ptr, len);
   host[len] = '\0';
   
   /* Move past the host */
   ptr += len;
   
   /* If there is a colon get the port */
   if (*ptr == ':') {
      char *portPtr;
      char *tempPtr;
      ptr++;
      
      /* Find the / and turn it into a null */
      portPtr = ptr + strlen(ptr);
      tempPtr = strchr(ptr, '/');
      
      if ((tempPtr > 0) && (tempPtr < portPtr))
         portPtr = tempPtr;
      
      *portPtr = '\0';

      /* Convert to a i */
      port = atoi(ptr);
      
      /* Change back from null */
      *portPtr = '/';
      
      ptr = portPtr;
   }

   /* Now copy the path part into the path var */
   len = (int)strlen(ptr);
   tmpLen = (int)(strchr(ptr, '?') - ptr);
   
   /* Check who is smaller */
   if ((tmpLen > 0) && (tmpLen < len))
      len = tmpLen;
   
   path = (char *)malloc(len + 1);
   
   strncpy(path, ptr, len);
   path[len] = '\0';
   
   /* Move past path part */
   ptr +=len;
   
   /* Now break down the query string */
   token = strtok( ptr, "?&" );
   
   /* While there are tokens left */
   while( token != NULL ) {
      
      /* Breaks down like this ?param=value */
      std::string param; 
      std::string value;
      char *valueStart;
      
      /* Find the value, and copy it in */
      valueStart = strchr(token, '=');
      if (valueStart == NULL) {
         free(url);
         throw InvalidURLException();
      }
      
      valueStart++;
      value = valueStart;

      /* Get the begining param */      
      param.assign(token, strlen(token) - (valueStart - token));
      
      if (param == "=") {
         free(url);
         throw InvalidURLException();
      }
      
      /* Add to map */
      query[param] = value;
      
      /* Anymore tokens? */
      token = strtok( NULL, "?&" );
   }

   /* Clean up our copy */
   free(url);
   url = NULL;
   
}

/* This function returns a full URL */
const char *Web::URL::getURL() {

   std::string sUrl;
   char tmpPort[6];
   std::map<std::string, std::string>::iterator queryIter;
   char queryChar = '?';
   
   /* Create the URL with the basic parts */
   sUrl = "http://";
   sUrl += host;
   
   if (port != 80) {
      itoa(port, tmpPort, 10);
      sUrl += ":";
      sUrl += tmpPort;
   }
   
   sUrl += path;

   /* Now do the query string */
   /* Correct a iterator to get all elements from map */
   queryIter = query.begin();
   while ( queryIter != query.end() ) {
      std::string key;
      std::string value;

      sUrl += queryChar;

      /* Retreive the key and the value */      
      key = queryIter->first;
      value = queryIter->second;

      sUrl += key;
      
      if (value.length() > 0) {
         sUrl += "=";
         sUrl += value;
      }

      /* Move along one */
      queryIter++;

      /* Make sure we us a & instead of ? on next loop */
      queryChar='&';

   }

   /* Delete old Memory and Malloc some more for this URL */
   if (url!=NULL)
      free(url);
         
   url = (char *)malloc(sUrl.length() + 1);
   strcpy(url, sUrl.data());

   return url;

}

/* Adds data onto the query string */
void Web::URL::addQuery(char *pParam, char *pValue) {
   /* We should encode the data */
   char *param;
   char *value;
   
   param = Web::URL::encode((unsigned char *)pParam);
   value = Web::URL::encode((unsigned char *)pValue);
   
   query[param] = value;
   
   free(param);
   free(value);
}

/* Encodes some data in the HTTP Escape format
   Please free my char * after usage :)        */
char *Web::URL::encode(unsigned char *data) {
   
   char *retChar;
   std::string ret = "";
   
   /* For each char try to escape */
   while (*data != NULL) {
      
      char tmpEscape[4];
      
      /* Check if we need escaping */
      /* if ( (*data>=0x00 && *data<=0x1F) 
        || (*data==0x75) || (*data==0x20) || (*data=='<') || (*data=='>')
        || (*data=='#') || (*data=='%') || (*data=='"') || (*data=='{') 
        || (*data=='}') || (*data=='|') || (*data=='\\') || (*data=='^')
        || (*data=='[') || (*data==']') || (*data=='`') ) {
      */ /* This is the offical set according to rfc2396, but lets be paranoid */
      if ( (*data<'0') || ((*data>'9') && (*data<'A')) || 
                          ((*data>'Z') && (*data<'a')) || (*data>'z') ) {
         sprintf(tmpEscape, "%%%.2X", *data);
         ret += tmpEscape;
      } else {
         ret += *data;
      }
      
      data++;
   }
   
   retChar = (char *)malloc(ret.length() + 1);
   strcpy(retChar, ret.data());
   
   return retChar;
   
}

/* Destory the object and return any memory */
Web::URL::~URL(void) {

   if (url!=NULL)
      free(url);
               
   if (host!=NULL)
      free(host);
   
   if (path!=NULL)
      free(path);

}

/******************************************************************************
Request Object Starts Here
******************************************************************************/
Web::Request::Request(Web::URL *pUrl) {

   WSADATA wsaData;

   /* Start up WSA */
   WSAStartup(MAKEWORD( 2, 2 ), &wsaData);

   /* Make a reference to the URL inside me */
   url = pUrl;

}

void Web::Request::Connect() {
   
	SOCKET httpSocket;
	int ret;
	struct sockaddr_in httpAddr;
	struct hostent *hostinfo;
	char buffer[REQUESTBUFFERSIZE + 1];
	int bufferSize;
   int timeout = REQUESTTIMEOUT;

   /* Choses settings for the socket to connect to */
   httpAddr.sin_family = AF_INET;

   /* Set the port */
   //httpAddr.sin_port = htons(url->getPort());
   httpAddr.sin_port = htons(80);

   /* Do a hostname lookup on the host */
   //hostinfo = gethostbyname (url->getHost());
   hostinfo = gethostbyname ("localhost");
   if (hostinfo == NULL) {
      throw HostNotFoundException();
   }
   
   /* Fill the httpAddr with IP data */
   httpAddr.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    
	/* Creates the socket */
	httpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
	
	/* Check for errors */
	if (httpSocket == INVALID_SOCKET)
	    throw SocketException();

   /* Add a timeout on all communiction*/
   ret = setsockopt(httpSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

   /* Check for errors */
   if (ret == SOCKET_ERROR) {
      closesocket(httpSocket);
      throw SocketException();
   }

   /* Connect to the server */
   ret = connect(httpSocket,(LPSOCKADDR)&httpAddr, sizeof(struct sockaddr));

   /* Check for errors */
   if (ret == SOCKET_ERROR) {
      closesocket(httpSocket);
      throw ConnectionFailedException(WSAGetLastError());
   }

   /* Create a request */
   bufferSize = _snprintf(buffer, REQUESTBUFFERSIZE, "GET %s HTTP/1.0\r\n\r\n", url->getURL());

   if (bufferSize <= 0) {
      closesocket(httpSocket);
      throw Exception("Request Buffer Too Small");
   }
      
   /* Send a HTTP request */
   ret = send(httpSocket, buffer, bufferSize, 0);

   if (ret < bufferSize) {
      closesocket(httpSocket);
      throw TransferException();
   }
   
   reply.assign("");
   ret = REQUESTBUFFERSIZE;
   
   /* Now wait for the return and loop while its coming */
   while (ret > 0) {
      ret = recv(httpSocket, buffer, REQUESTBUFFERSIZE, 0);
      
      /* Append data to the buffer */
      if (ret > 0)
         reply.append(buffer, ret);
   }

   /* Clean up the socket */
   closesocket(httpSocket);

   /* Now if we didn't get any data throw a error */
   if (reply.length()==0) {
      throw TransferException();
   }

   /* Got some data let parse it */
   ret = (int)reply.find("\r\n\r\n");

   if (ret==-1) {
      throw InvalidDataException();
   }
   
   header = reply.substr(0, ret);
   ret += 4;
   body = reply.substr(ret, reply.length() - ret);  

}

Web::Request::~Request(void) {
   WSACleanup ();
}