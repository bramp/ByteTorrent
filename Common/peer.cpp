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
   peer.cpp version 0.9 by me@bramp.net
   Class to connect to peers
*/

#include "stdafx.h"
#include "transfer.h"

/******************************************************************************
PeerList Object Starts Here
******************************************************************************/
Transfer::PeerList::PeerList(char *infoHash, char *peerID) {
   this->infoHash = infoHash;
   this->peerID = peerID;
   
   InitializeCriticalSection (&peersLock);

   peerCount = 0;
}

void Transfer::PeerList::add(void* peer) {
   /* Make sure we aren't interrupted */
   EnterCriticalSection(&peersLock);

   /* Allow others to play with peers */
   LeaveCriticalSection(&peersLock);
}

Transfer::PeerList::~PeerList() {
   /* Clean up the CriticalSection */
   DeleteCriticalSection(&peersLock);
}


/******************************************************************************
PeerListener Object Starts Here
******************************************************************************/

DWORD WINAPI Transfer::PeerListener::peerListenerThread(LPVOID lpParameter) {
   
   PeerListener *me = (PeerListener *)lpParameter;
   Peer *incomingPeer;
   SOCKET sock;
   struct sockaddr addr;
   int addrLen;

   //me->peers
   do {
      addrLen = sizeof(struct sockaddr);
      
      /* Block until a connection */
      sock = accept(me->conn, (struct sockaddr*)&addr, &addrLen);
      
      if (sock != INVALID_SOCKET) {
         try {
            /* Create a new peer */
            incomingPeer = new Peer(me->peers, sock);

            /* A catch all.. We don't really care about Peer errors (well at the moment( */
         } catch (...) {}
      } else {
         /* Sleep here to stop fast spinning error loops */
         Sleep(5000);
      }

   } while (!(me->closing));
   
   return true;
}

/* Starts listening on this port */
Transfer::PeerListener::PeerListener(PeerList *peers, int port) {
	
   WSADATA wsaData;
	int err;
	struct sockaddr_in addr;

   WSAStartup(MAKEWORD( 2, 2 ), &wsaData);

   /* Creates the socket */
	conn = socket(AF_INET,SOCK_STREAM,0);
   
   if (conn == INVALID_SOCKET) {
      throw new SocketErrorException(WSAGetLastError());
   }

   /* Choses settings for the socket to bind to */
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(port);	

   err = bind(conn, (struct sockaddr*)&addr, sizeof(addr));

   /* Check for errors and throw the correct type */
   if (err == SOCKET_ERROR) {
      closesocket(conn);
      err = WSAGetLastError();
      switch (err) {
         case WSAEADDRINUSE:
            throw new PortInUseException();
            break;
         default:
            throw new SocketErrorException(err);
      }
   }

   /* Now actually make the socket listen) */
   err = listen(conn, 10);

   if (err == SOCKET_ERROR) {
      closesocket(conn);
      err = WSAGetLastError();
      switch (err) {
         case WSAEADDRINUSE:
            throw new PortInUseException();
            break;
         default:
            throw new SocketErrorException(err);
      }
   }

   this->peers = peers;
   this->closing = false;

   /* We have got this far so everything is fine, now we just spawn the listener thread */
   DWORD threadID;

   CreateThread(NULL, 0, &peerListenerThread, this, 0, &threadID);

}

/******************************************************************************
Peer Object Starts Here
******************************************************************************/

/* Listens on this port */
Transfer::Peer::Peer(PeerList *peerList, SOCKET incoming) {
   int addrLen = sizeof(this->addr);

   /* Set up local vars */
   this->sock = incoming;
   getsockname(incoming, (sockaddr *)&(this->addr), &addrLen);
   this->state = peerState::connected;
   this->incoming = true;

   this->peerList = peerList;

   /* Now do the handShake, 
      If we fail a exception is thrown from inside */
   handShake();

   /* By now we know we are good so Lets fire off a new peer thread */
   DWORD threadID;
   CreateThread(NULL, 0, &peerThread, this, 0, &threadID);
   
}

void Transfer::Peer::handShake() {
   int len;
   int ret;

   len = sizeof(PROTOCOL);

   /* Lets receive some data :) */
   ret = recv(this->sock, data, len, 0);

   /* If no data or a error just give up */
   if (ret < len) {
      closesocket(this->sock);
      throw new PeerConnectException();
   }

   /* Send out the protocol line */
   len--;
   send(this->sock, (char *)&len, 1, 0);
   send(this->sock, PROTOCOL, (sizeof(PROTOCOL) - 1), 0);

   /* If this isn't the protocol line, panic :) */
   if ((len != (sizeof(PROTOCOL) - 1)) || (memcmp(&data[1], PROTOCOL, len) != 0)) {
      /* Close the socket, throw */
      closesocket(this->sock);
      throw new PeerConnectException();
   }

   /* Now send the 8 empty bytes */
   len = 0;
   send(this->sock, (char *)&len, 4, 0);
   send(this->sock, (char *)&len, 4, 0);

   /* Recv the blank 8 bytes */
   ret = recv(this->sock, data, 8, 0);

   /* If no data or a error just give up */
   if (ret < 8) {
      closesocket(this->sock);
      throw new PeerConnectException();
   }

   /* Now send the torrent SHA1 hash */
   send(this->sock, this->peerList->infoHash, 20, 0);

   /* Recv the torrent hash */
   ret = recv(this->sock, data, 20, 0);

   /* If no data or a error just give up or the torrent hash doesn't match */
   if (ret < 20 || (memcmp(data, this->peerList->infoHash, 20) != 0)) {
      closesocket(this->sock);
      throw new PeerConnectException();
   }

   /* Now send our peer ID */
   send(this->sock, this->peerList->peerID, 20, 0);

   /* Recv the peer ID */
   ret = recv(this->sock, data, 20, 0);

   /* If no data or a error just give up or the torrent hash doesn't match */
   if (ret < 20) {
      closesocket(this->sock);
      throw new PeerConnectException();
   }

   /* If we have a blank ID lets create a new one */
   if (this->peerID != NULL) {
      if (memcmp(data, this->peerID->get(), 20) != 0) {
         closesocket(this->sock);
         throw new PeerConnectException();
      }
   } else {
      this->peerID = new bee::String(data, 20);
   }

   this->doneHandShake = true;

   /* Now add ourself to the peer list if this is a incoming connection */
   if (this->incoming)
      this->peerList->add(this);
}

/* Onces connected we spawn a new worker thread */
DWORD WINAPI Transfer::Peer::peerThread(LPVOID lpParameter) {
   Peer *me = (Peer *)lpParameter;

   /* Check if we have done the handshake, if not lets begin */
   if (!me->doneHandShake) {
      try {
         int err;
         WSADATA wsaData;

         /* Connect out and do handshake */
         
         /* Set up winsock */
         WSAStartup(MAKEWORD( 2, 2 ), &wsaData);

         /* Creates the socket */
	      me->sock = socket(AF_INET, SOCK_STREAM, 0);
         
         if (me->sock == INVALID_SOCKET) {
            me->state = peerState::closed;
            return false;
         }

         err = connect(me->sock, (struct sockaddr*)&me->addr, sizeof(me->addr));

         /* Check for errors and throw the correct type */
         if (err == SOCKET_ERROR) {
            closesocket(me->sock);
            me->state = peerState::closed;
            return false;
         }

         me->handShake();
      } catch (...) {
         me->state = peerState::closed;
         return false;
      }
   }

   /* Now lets start the main loop */
   

   return true;
}

/* Connects out to this peer */
Transfer::Peer::Peer(PeerList *peerList, bee::String *peerID, char *ip, int port) {

   this->doneHandShake = false;

   memset(&(this->addr), 0, sizeof(this->addr));
   this->addr.sin_family = AF_INET;
   this->addr.sin_addr.S_un.S_addr = inet_addr(ip);
   this->addr.sin_port = htons(port);
   this->peerID = new bee::String(peerID->get(), 20);
   this->incoming = false;
   this->sock = INVALID_SOCKET;
   this->peerList = peerList;

   /* Add ourselfs to the list */
   this->peerList->add(this);

   /* By now we know we are good so Lets fire off a new peer thread */
   DWORD threadID;
   CreateThread(NULL, 0, &peerThread, this, 0, &threadID);
}

Transfer::Peer::~Peer() {
   /* Clean up some things */
   if (this->sock != INVALID_SOCKET)
      closesocket(this->sock);

   if (this->peerID != NULL)
      delete this->peerID;
   
   WSACleanup();
}

/* Close the connection */
void Transfer::Peer::Close() {
   this->state = peerState::closed;
   closesocket(this->sock);
   this->sock = INVALID_SOCKET;
}