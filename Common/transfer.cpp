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
   transfer.cpp version 0.9 by me@bramp.net
   Class to actually download a file
*/

#include "stdafx.h"
#include "transfer.h"
#include "torrent.h"
#include "web.h"
#include "file.h"

/* Creates a transfer
 [in] data - The name of the torrent
*/
Transfer::Transfer(char *torrentFile) { //throws InvalidTorrentException, TorrentNotFoundException

   myTorrent = NULL;
   pieces = NULL;
   myFiles = NULL;
   
   /* Open the torrent file */
   myTorrent = new Torrent(torrentFile);

   /* Set this transfers peer ID */
   createRandomString(peerID, 20);
   peerID[20] = '\0';
   /* For testing reasons I'm setting this fixed */
   strcpy(peerID, "ABCDEFGHIJKLMNOPQRST");

   /* Get the name for this transfer */
   torrentName = (const char*)(myTorrent->getInfoString("name"));
   
   if (torrentName = NULL) {
      torrentName = UNNAMEDTORRENT;
   }

   this->remaining = 0;
   this->listeningPort = 1000;
   this->eventCallback = NULL;
   this->peers = NULL;
}

void Transfer::setSaveLocation(char *pSavePath) {
   strncpy(savePath, pSavePath, MAX_PATH);
}

void Transfer::setup() {

   const char *filename;
   char localFilename[MAX_PATH];
   int size;
   int i=0;
   
   myFiles = new Files();
   
   /* Create a list of files to work on */
   while (myTorrent->getFileName(i, &filename, &size)) {
      _snprintf(localFilename, MAX_PATH, "%s\\%s", savePath, filename);
      myFiles->addFile(localFilename, size);
      i++;
   }

   INT64 filesLength = myFiles->length();
   int pieceLen = (int)myTorrent->getInfoInteger("piece length");
   int piecesCount = (int) (filesLength / (INT64)pieceLen);
   
   /* Point the Pieces array at the correct thing */
   pieces = new PieceMap(myTorrent->getInfoString("pieces"), piecesCount, pieceLen, myFiles);
   
   this->remaining = pieces->checkAll();

   /* Set up our peer List */
   peers = new PeerList((char *) this->myTorrent->getInfoHash(), this->peerID);

   /* We set how many seconds between tracker connection, 0 to start with */
   this->interval = 0;
   this->state = transferState::initialised;
}

/* This sends data to the tracker and parses its return */
void Transfer::sendTrackerData(char *event) {

   /* Get the tracker URL */
   Web::URL *url;
   Web::Request *request;

   /* Bee Data */
   bee::Dictionary *trackerData;
   bee::Integer *beeInterval;
   bee::List *beePeers;
   bee::Dictionary *peer;
   bee::String *failure;

   int bytes;
   int i;
   
   /* Some char arrays for the numbers used */
   char port[6];
   INT64 left64;
   char left[21];

   url = new Web::URL((char *)this->myTorrent->getTracker());

   /* Add all the info we want */
   url->addQuery("info_hash", (char *)this->myTorrent->getInfoHash());
   url->addQuery("peer_id", this->peerID);
   //url->addQuery("ip', peerID);
   url->addQuery("port", itoa(this->listeningPort, port, 10));
   url->addQuery("uploaded", "0");
   url->addQuery("downloaded", "0");
   
   if (event == NULL)
      event = "";

   url->addQuery("event", event);

   left64 = (INT64)(this->remaining) * (INT64)this->pieces->getPieces();
   url->addQuery("left", itoa64(left64, left));

   /* Now make a connection to the tracker */
   request = new Web::Request(url);
   request->Connect();

   /* Beedecode the data from the tracker */
   trackerData = (bee::Dictionary *)bee::decode((char *)request->getBody(), &bytes);

   /* Print out data (debugging line) */
   //trackerData->printme();

   /*Check for failure */
   failure = (bee::String *)trackerData->get("failure reason");
   
   if (failure != NULL) {
      /* Something has gone wrong (we should log this) */
      Log::AddMsg ("Bad data from tracker %s", failure->get());
   }

   /* Update the Interval */
   beeInterval = (bee::Integer *)trackerData->get("interval");

   if (beeInterval != NULL)
      this->interval = (int)beeInterval->get();
      
   /* Now cycle the peers and add them */
   beePeers = (bee::List *)trackerData->get("peers");
      
   if (beePeers != NULL) {
      for (i = 0; i< beePeers->count(); i++) {
         peer = (bee::Dictionary *)beePeers->get(i);
         
         if (peer != NULL) {
            bee::String *peerID = (bee::String *)peer->get("peer id");
            bee::String *ip = (bee::String *)peer->get("ip");
            bee::Integer *port = (bee::Integer *)peer->get("port");
            Peer *newPeer;

            if (peerID != NULL && ip != NULL && port != NULL) {
               try {
                  newPeer = new Peer(this->peers, peerID, ip->get(), (int)port->get());
                  //peers.add(newPeer);
               } catch (...) { }
            }

         }
      }
   }

   delete trackerData;
   delete request;
   delete url;
}

/* This loops around connecting to the tracker and dishing out connections */
DWORD WINAPI Transfer::trackerThread(LPVOID lpParameter) {
  
   /* This is how long we sleep in each spin around the loop
      Longer values mean the app will quit slower
      Shorter values mean we will use more processor
   */
   Transfer *me = (Transfer *)lpParameter;
   const int SLEEPTIME = 5;
   int loopCount = 0;
   PeerListener *peerListener = NULL;

   me->listeningPort = STARTPORT;

   /* This loop spins trying to create a new peerListener.
      The reason it spins is to increment the port until it finds a valid one */
   while ((peerListener == NULL) && (me->listeningPort < ENDPORT)) {
      try {
         peerListener = new PeerListener(me->peers, me->listeningPort);
      } catch (PeerListener::PortInUseException) {
         Log::AddWsaMsg("Error setting up PeerListener port:%i", WSAGetLastError(), (void *)me->listeningPort);
         (me->listeningPort)++;
      } catch (PeerListener::SocketErrorException e) {
         Log::AddWsaMsg("Error setting up PeerListener port:%i", e.getErrorCode(), (void *)me->listeningPort);
      }
   }

   Log::AddWsaMsg("PeerListener Listening On Port:%i", WSAGetLastError(), (void *)me->listeningPort);

   while (me->state != transferState::quit) {
      
      /* If we are quiting remove the interval */
      if (me->state == transferState::quiting)
         me->interval = 0;
      
      if ((loopCount * SLEEPTIME) >= me->interval) {
         
         loopCount = 0;

         switch (me->state) {
            case (transferState::initialised): {
               
               /* Set this to 60 incase we crash out of the sendTrackerData */
               me->interval = 60;

               /* Try announcing ourselfs to the tracker */
               me->sendTrackerData("started");

               /* If we have gotten this far then All systems are go */
               me->state = transferState::running;
               break;

            /* Running and finished do the same thing */
            } case (transferState::running): {
            } case (transferState::finished): {
               me->sendTrackerData("");
               break;

            } case (transferState::quiting): {
               /* Start to close us down */
               me->sendTrackerData("stopped");
               /* Close all connections */
               me->state = transferState::quit;
               break;
            }

         } /* endswitch */
      } else {
         loopCount++;
      }

      Sleep(SLEEPTIME * 1000);
   }

   return true;
}

/* This spawns a new Tracker Thread */
void Transfer::start() {
   /* TODO add thread code */
   trackerThread(this);
};

Transfer::~Transfer(void) {
   if (myTorrent != NULL)
      delete myTorrent;
   
   /* Delete the files list */
   if (myFiles != NULL)
      delete myFiles;
      
   if (pieces != NULL)
      delete pieces;

   if (peers != NULL)
      delete peers;

}

/******************************************************************************
Files Object Starts Here
******************************************************************************/
Transfer::Files::Files() {
   startIndex = NULL;
   lastIndex = NULL;
}

Transfer::Files::~Files() {
   
   /* Delete the FileIndex list */
   while (startIndex != NULL) {
      FileIndex *tmp = startIndex;
      
      /* Cleanup the data */     
      delete tmp->data;
      startIndex = tmp->nextIndex;
      
      /* And now the FileIndex */
      delete tmp;
      
   }
}

void Transfer::Files::addFile(const char *fileName, int size) {
   
   FileIndex *tmpIndex;
   File *aFile;

   /* Try to open/create the file */
   aFile = new File((char *)fileName, size);

   /* Create new file index */
   tmpIndex = new FileIndex;

   tmpIndex->data = aFile;
   tmpIndex->len = size;
   tmpIndex->nextIndex = NULL;

   if (startIndex == NULL) {
      startIndex = tmpIndex;
   } else {
      lastIndex->nextIndex = tmpIndex;
   }

   lastIndex = tmpIndex;
}

INT64 Transfer::Files::length() {

   INT64 length = 0;
   FileIndex *idx;

   idx = startIndex;

   while (idx != NULL) {
      length += (INT64)idx->len;
      idx = idx->nextIndex;
   }
   
   return length;
}

File *Transfer::Files::findPart(INT64 *start) {

   FileIndex *idx = startIndex;
   File *lastFile;

   if (startIndex==NULL) {
      /* Some kind of error */
      printf("ERROR");
   }
    
   lastFile = idx->data;
   
   /* Loop while we aren't in the correct file */
   while (idx != NULL && *start > (INT64)idx->len) {
      *start -= (INT64)idx->len;
      lastFile = idx->data;
      idx = idx->nextIndex;
   }
   
   return lastFile;
}

int Transfer::Files::getPart(char *buffer, INT64 start, int len) {
   
   int bytesRead;
   int TotalBytesRead = 0;
   File *currentFile;
   INT64 currentPos = start;

   while (len > 0) {
   
      currentFile = findPart(&currentPos);
      
      bytesRead = currentFile->getPart(buffer, (int)currentPos, len);
      
      if (bytesRead < 0) {
         /* Some kind of error */
         printf("ERROR");
      }
      
      len -= bytesRead;
      TotalBytesRead += bytesRead;
      buffer+=bytesRead;
      
      currentPos = start + TotalBytesRead;
   }
   
   if (len > 0) {
      /* Some kind of error */
      printf("ERROR");
   }
   
   return TotalBytesRead;
}


/******************************************************************************
pieceMap Object Starts Here
******************************************************************************/
Transfer::PieceMap::PieceMap(unsigned const char *pPieceHash, int pPieces, int pPieceSize, Transfer::Files *pFiles) {
      
   /* Set all the member vars */
   PieceHash = pPieceHash;
   pieces = pPieces;
   pieceSize = pPieceSize;
   Files = pFiles;
   
   /* Malloc enough bits to count, 2 per pieces */
   map = (unsigned char*)malloc((pPieces/8)*2 + 1);
   bitMap = (unsigned char*)malloc((pPieces/8) + 1);
   
   //memset(map, 0, (pPieces/8)*2);
   //memset(bitMap, 0, (pPieces/8));
   
   /* Initialize the critical section */
   InitializeCriticalSection (&pieceLock);
}

Transfer::PieceMap::~PieceMap() {
   free(map);
   free(bitMap);
   
   /* Clean up the CriticalSection */
   DeleteCriticalSection(&pieceLock);
}

int Transfer::PieceMap::checkAll() {
   
   int i=0;
   INT64 filePos=0;
   INT64 len;
   unsigned char *mapPtr;
   unsigned char *bitMapPtr;
   unsigned char mapMask = 0;
   unsigned char bitMapMask = 0;
   char *buffer;
   unsigned const char *hash;
   int remaining = 0;
  
   /* Enter critical section so we have exclusive rights to the maps */
   EnterCriticalSection(&pieceLock); 

   bitMapPtr = bitMap;
   mapPtr = map;
   buffer = (char*) malloc(pieceSize);
   hash = PieceHash;
   
   len = (INT64)pieceSize * (INT64)pieces;

   mapMask = 0xC0;
   bitMapMask = 0x80;

   while (i<pieces) {

      Files->getPart(buffer, filePos, pieceSize);
      
      /* Map looks like
      00 00 00 ** 00 00 00 00   0xC0 0x30 0x0C 0x03
      00 - 0 Not Checked
      01 - 1 Downloading
      10 - 2 Checked InValid
      11 - 3 Checked Valid
      */
      
      /* Set affected area to zero */
      *mapPtr &= ~mapMask;
      *bitMapPtr &= ~bitMapMask;
      
      /* Check pieces and set correct bits */
      if (checkPiece(buffer, hash)) {
         *mapPtr |= mapMask;
         *bitMapPtr |= bitMapMask;
      } else {
         *mapPtr |= (mapMask & 0xAA);
         //*bitMapPtr |= (bitMapMask & 0x00);
         remaining++;
      }
      
      mapMask = mapMask >> 2;
      bitMapMask = bitMapMask >> 1;
      
      i++;
      filePos+=pieceSize;
      hash+=20;
      
      if (mapMask == 0) {
         mapMask = 0xC0;
         mapPtr++;
         if (bitMapMask==0) {
            bitMapMask = 0x80;
            bitMapPtr++;
         }
      }
   }
   
   free(buffer);

   LeaveCriticalSection(&pieceLock);

   return remaining;
}

bool Transfer::PieceMap::check(int idx) {

   //unsigned char* mapPtr;
   char *buffer;
   unsigned char *hash;
  
   buffer = (char*) malloc(pieceSize);
      
   hash = (unsigned char *)PieceHash + 20 * idx;
      
   Files->getPart(buffer, idx, pieceSize);
   
   if (checkPiece(buffer, (unsigned char *)hash)) {
      /* Good Piece */  
      free(buffer);
      setBit(idx, bitState::Valid);
      return true;
   } else {
      /* Bad Piece */
      free(buffer);
      setBit(idx, bitState::InValid);
      return false;
   }
}

/* Returns true for valid */
bool Transfer::PieceMap::checkPiece(char *fileBuffer, unsigned const char *pieceHash) {
   unsigned char hash[21];
   int ret;
   
   sha.Reset();
   sha.Update((unsigned char *)fileBuffer, pieceSize);
   sha.Final();
   
   sha.GetHash(&hash[0]);
   
   ret = strnicmp((char *)pieceHash, (char *)hash, 20);
   
   return (ret==0);
}

Transfer::PieceMap::bitState Transfer::PieceMap::getBit(int idx) {
   
   unsigned char *mapPtr;
   unsigned char mapMask;
   int bit;

   //Move to the correct place in the map
   mapPtr = map + (idx / 4);

   //Figure out which mask to use
   switch (idx % 4) {
      case 0: mapMask = 0xC0; break;
      case 1: mapMask = 0x30; break;
      case 2: mapMask = 0x0C; break;
      case 3: mapMask = 0x03; break;
   }

   //Mask out the correct values
   bit = (mapMask & *mapPtr);

   //Now shift down
   bit = bit >> (3 - (idx % 4)) * 2;

   return (bitState)bit;
}


void Transfer::PieceMap::setBit(int idx, Transfer::PieceMap::bitState value) {
   unsigned char *mapPtr;
   unsigned char *bitMapPtr;
   unsigned char mapMask = 0;
   unsigned char bitMapMask = 0;
   int shift;
   
   /* Enter critical section so we have exclusive rights to the maps */
   EnterCriticalSection(&pieceLock); 

   bitMapPtr = (idx / 8) + bitMap;
   mapPtr = (idx / 4) + map;
   
   /* Do the bitMap */
   if (value==bitState::Valid) {
      bitMapMask = 1 << (idx % 8);
   } else {
      bitMapMask = 0;
   }

   *bitMapPtr &= ~bitMapMask;
   *bitMapPtr |= bitMapMask;

   /* Do the normal map */
   shift = (3 - (idx % 4)) * 2;
   mapMask = 3 << shift ;
   value = (bitState) (value << shift);
   
   *mapPtr &= ~mapMask;
   *mapPtr |= value;

   LeaveCriticalSection(&pieceLock);
   
}


const unsigned char *Transfer::PieceMap::getBitMap() {
   return bitMap;
}
