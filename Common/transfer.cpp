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
transfer::transfer(char *torrentFile) { //throws InvalidTorrentException, TorrentNotFoundException

   myTorrent = NULL;
   pieces = NULL;
   myFiles = NULL;
   
   /* Open the torrent file */
   myTorrent = new torrent(torrentFile);

   /* Set this transfers peer ID */
   createRandomString(peerID, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890", 20);
   peerID[20] = '\0';
   
   /* Get the name for this transfer */
   torrentName = (const char*)(myTorrent->getInfoString("name"));
   
   if (torrentName = NULL) {
      torrentName = UNNAMEDTORRENT;
   }

}

void transfer::setSaveLocation(char *pSavePath) {
   strncpy(savePath, pSavePath, MAX_PATH);
}

void transfer::setup() {

   const char *filename;
   char localFilename[MAX_PATH];
   int size;
   int i=0;
   
   myFiles = new files();
   
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
   pieces = new pieceMap(myTorrent->getInfoString("pieces"), piecesCount, pieceLen, myFiles);
   
   int remaining = pieces->checkAll();
}

void transfer::start() {

   web::URL *url;
   web::Request *request;
   //bee::dictionary *trackerData;
   //int bytes;

   /* Get the tracker URL */
   url = new web::URL((char *)myTorrent->getTracker());

   /* Add all the info we want */
   url->addQuery("info_hash", (char *)myTorrent->getInfoHash());
   url->addQuery("peer_id", peerID);
   //url->addQuery("ip', peerID);
   url->addQuery("port", "6882");
   url->addQuery("uploaded", "0");
   url->addQuery("downloaded", "0");
   url->addQuery("left", "1");
   url->addQuery("event", "started");

   /* Now make a connection to the tracker */
   request = new web::Request(url);
   //request->Connect();
   
   /* Beedecode the data from the tracker */
   //trackerData = (bee::dictionary *)bee::decode((char *)request->getBody(), &bytes);
   
   //trackerData->printme();
   
   //delete trackerData;
   
   delete request;
   delete url;
   
};

transfer::~transfer(void) {
   if (myTorrent != NULL)
      delete myTorrent;
   
   /* Delete the files list */
   if (myFiles != NULL)
      delete myFiles;
      
   if (pieces != NULL)
      delete pieces;

}

/******************************************************************************
Files Object Starts Here
******************************************************************************/
transfer::files::files() {
   startIndex = NULL;
   lastIndex = NULL;
}

transfer::files::~files() {
   
   /* Delete the fileIndex list */
   while (startIndex != NULL) {
      fileIndex *tmp = startIndex;
      
      /* Cleanup the data */     
      delete tmp->data;
      startIndex = tmp->nextIndex;
      
      /* And now the fileIndex */
      delete tmp;
      
   }
}

void transfer::files::addFile(const char *fileName, int size) {
   
   fileIndex *tmpIndex;
   file *aFile;

   /* Try to open/create the file */
   aFile = new file((char *)fileName, size);

   /* Create new file index */
   tmpIndex = new fileIndex;

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

INT64 transfer::files::length() {

   INT64 length = 0;
   fileIndex *idx;

   idx = startIndex;

   while (idx != NULL) {
      length += (INT64)idx->len;
      idx = idx->nextIndex;
   }
   
   return length;
}

file *transfer::files::findPart(INT64 *start) {

   fileIndex *idx = startIndex;
   file *lastFile;

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

int transfer::files::getPart(char *buffer, INT64 start, int len) {
   
   int bytesRead;
   int TotalBytesRead = 0;
   file *currentFile;
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
transfer::pieceMap::pieceMap(unsigned const char *pPieceHash, int pPieces, int pPieceSize, files *pFiles) {
      
   /* Set all the member vars */
   PieceHash = pPieceHash;
   pieces = pPieces;
   pieceSize = pPieceSize;
   Files = pFiles;
   
   /* Malloc enough bits to count, 2 per pieces */
   map = (unsigned char*)malloc((pPieces/8)*2 + 1);
   
   memset(map, 0, (pPieces/8)*2);
   
}

transfer::pieceMap::~pieceMap() {
   free(map);
}

int transfer::pieceMap::checkAll() {
   
   int i=0;
   INT64 filePos=0;
   INT64 len;
   unsigned char *mapPtr;
   unsigned char mapMask = 0;
   char *buffer;
   unsigned const char *hash;
   int remaining = 0;
  
   mapPtr = map;
   buffer = (char*) malloc(pieceSize);
   hash = PieceHash;
   
   len = (INT64)pieceSize * (INT64)pieces;

   mapMask = 0xC0;

   while (i<pieces) {

      Files->getPart(buffer, filePos, pieceSize);
      
      /* Map looks like
      00 00 00 ** 00 00 00 00   0xC0 0x30 0x0C 0x03
      00 - 0 Not Checked
      01 - 1 Downloading
      10 - 2 Checked InValid
      11 - 3 Checked Valid
      */
      
      if (checkPiece(buffer, hash)) {
         *mapPtr |= (mapMask & 0xFF);
      } else {
         *mapPtr |= (mapMask & 0xAA);
         remaining++;
      }
      
      mapMask = mapMask >> 2;
      
      i++;
      filePos+=pieceSize;
      hash+=20;
      
      if (mapMask == 0) {
         mapMask = 0xC0;
         mapPtr++;
      }
      
   }

   return remaining;
}

bool transfer::pieceMap::check(int idx) {
   unsigned char* mapPtr;
   
   mapPtr = map + ((idx / 8) * 2);
   
   //checkPiece
   
   return false;
}

/* Returns true for valid */
bool transfer::pieceMap::checkPiece(char *fileBuffer, unsigned const char *pieceHash) {
   unsigned char hash[21];
   int ret;
   
   sha.Reset();
   sha.Update((unsigned char *)fileBuffer, pieceSize);
   sha.Final();
   
   sha.GetHash(&hash[0]);
   
   ret = strnicmp((char *)pieceHash, (char *)hash, 20);
   
   return (ret==0);
}