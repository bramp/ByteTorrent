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
   Transfer.h version 0.9 by me@bramp.net
   Class to actually download a file
*/

#pragma once

#include "torrent.h"
#include "file.h"

#define STARTPORT 1000
#define ENDPORT 1100
#define BUFFERSIZE 1024 //Must be no smaller than 20
#define PROTOCOL "BitTorrent protocol"

class Transfer {
   private:

      /* This gets populated so I can map Pieces to files */
      class Files {
      
         private:
            class FileIndex {
               public:
                  File *data;
                  int len;
                  FileIndex *nextIndex;
            };

            FileIndex *startIndex;
            FileIndex *lastIndex;
            
            /* Returns a file, start and len to write. Also returns true if we are to write to
               another file also */
            //bool findPart(INT64 begin, const file *file, const int *start, const int *len);
            File *findPart(INT64 *start);
            
         public:
            Files();
            ~Files();
            /* Finds which file(s) and sets a part in it */
            int getPart(char *buffer, INT64 start, int len);
            int setPart(char *buffer, INT64 start, int len);
            
            void addFile(const char *fileName, int size);
            
            INT64 length();
      };   

      class PieceMap {
         private:
            unsigned const char *PieceHash;
            int pieces;
            int pieceSize;
            unsigned char *map; /* Used for double bit data */
            unsigned char *bitMap; /* Used for single bit data */

            CRITICAL_SECTION pieceLock;

            /* Checks one piece */
            bool checkPiece(char *fileBuffer, unsigned const char *pieceHash);

            Files *Files; /* List of files for this PieceMap */
            CSHA1 sha; /* Temp SHA1 to be used */

         public:
            PieceMap(unsigned const char *pPieceHash, int pPieces, int pPieceSize, Files *pFiles);
            ~PieceMap();

            enum bitState {
               NotChecked = 0,  // 00
               Downloading = 1, // 01
               InValid = 2,     // 10
               Valid = 3,       // 11
            };
            
            /* Checks the entire map */
            int checkAll();
            
            /* Only checks specific index */
            bool check(int idx);

            /* Get info about specific bit */
            Transfer::PieceMap::bitState getBit(int idx);
            
            /* Set a bit */
            void setBit(int idx, Transfer::PieceMap::bitState value);

            /* Get the single digit bit map */
            const unsigned char *getBitMap();

            int getPieceSize() { return pieceSize; };
            int getPieces() { return pieces; };
      };

      /* Keep track of all the peers, and what file they are downloading */
      class PeerList {
         private:
            /* Allows mutal exclusive access to the peers list */
            CRITICAL_SECTION peersLock;

            /* List of all the peers sent to us by the tracker keyed on their peerID*/
            std::map<bee::String*, void*> peers;

            int peerCount;

         public:
            PeerList(char *infoHash, char *peerID);
            ~PeerList();
            
            /* Adds new peer */
            void add(void* peer);

            /* Returns peer with this ID, returns NULL if none found */
            void* get(bee::String* peerID);

            /* Remove old peers (ie ones that we can't connect to) */
            int removeDeadPeers();

            /* Info hash of the torrent */
            char *infoHash;

            /* Our peer ID */
            char *peerID;

      };

      class Peer {
         public:

            class PeerConnectException : public Exception {
               public: PeerConnectException() {};
            };

            enum peerState {
               waiting,
               connected,
               closed
            };

         private:
            /* ID of the peer we are connected to */
            bee::String *peerID;
            peerState state;
            
            struct sockaddr_in addr;
            SOCKET sock;

            /* A buffer to hold the recieved TCP data */
            char data[BUFFERSIZE];
            
            /* Two pointers to manage the processing of the data */
            //char *dataStart;
            //char *dataEnd;

            /* Onces connected we spawn a new worker thread */
            static DWORD WINAPI peerThread(LPVOID lpParameter);

            /* Carrys out the handshake */
            void handShake();
            bool doneHandShake;

            /* Tells us if it was a incoming or outgoing connection */
            bool incoming;

            /* The peerList that we belong to */
            PeerList *peerList;

         public:
            /* Accepts a incoming connection */
            Peer(class Transfer::PeerList *peerList, SOCKET incoming);

            /* Connects out to this peer */
            Peer(class Transfer::PeerList *peerList, bee::String *peerID, char *ip, int port);
            ~Peer();

            /* Connects to the peer */
            void Connect();
            
            /* Close the connection */
            void Close();
      
            peerState getState() { return state; };
      };

     
      class PeerListener {
         private:
            /* Member called to listen out */
            static DWORD WINAPI peerListenerThread(LPVOID lpParameter);

            /* Used to hold all the peers */
            PeerList *peers;

            SOCKET conn;
            int port;
            bool closing;
            bee::String *infoHash;

         public:
            class SocketErrorException : public Exception {
               public: SocketErrorException(int errorCode) { mErrorCode = errorCode; };
            };

            class PortInUseException : public Exception {};

            PeerListener(PeerList *peers, int port) throw(...);

            /* Stops Listening */
            void Close() { closing = true; };
      };

      /* Torrent file associated with this */
      Torrent *myTorrent;
      const char *torrentName;

      /* This peer's unquie (random) transfer ID */
      char peerID[21];

      /* Map of peices in this transfer */
      PieceMap *pieces;

      /* List of files in this transfer */
      Files *myFiles;

      PeerList *peers;

      /* Where the files are being saved */
      char savePath[MAX_PATH];

      int remaining;
      int listeningPort;

      /* Time to wait between each connection to the tracker */
      int interval;

   public:
      /* Class that handles events */
      class Event {
         enum eventType {
            started,
            pieceDone,
            fileDone,
            torrentDone,
            peerConnect,
            errorTrackerConnection,
            errorTrackerBadTorrent,
            errorTrackerBad,
            errorBadHash
         };
      
      };

      /* This records our current state */
      enum transferState {
         unknown,
         initialised, /*All varibles are set up */
         running, /* We are transfering up and down */
         finished, /* Finished transfering down, but still going up */
         quiting, /* Closing all connections */
         quit, /* Cleaned up, now lets die */
      };

   private:
      /* Points to the function that receives events */
      int (*eventCallback)(Transfer*, Event*);

      transferState state;

      /* This loops around connecting to the tracker and dishing out connections */
      static DWORD WINAPI trackerThread(LPVOID lpParameter);

      /* This sends data to the tracker and parses its return */
      void Transfer::sendTrackerData(char *event);

   public:
      /* Create a transfer object pointing to this torrent */
      Transfer(char *torrentFile);
      ~Transfer(void);

      /* Specify where the files will be saved */
      void setSaveLocation(char *pSavePath);
      const char *getSaveLocation() { return savePath; };
      
      /* Scans the torrent and sets all the transfer details */
      void setup();
      
      /* Spawns a new thread and then connects to the tracker then peers and does the download */
      void start();

      /* What a EventCallback function looks like */
      //int (*EventCallback)(*Transfer transfer, *Event event)

      void setEventHandler( int (*EventCallback)(Transfer *, Event *) ) { eventCallback = EventCallback; };

      /* Remaining pieces to get */
      int getPiecesLeft() { return remaining; };
      
      /* TCP Port we are listening on */
      int getPort() { return listeningPort; };

      /* Get State */
      transferState getState() { return state; };
};
