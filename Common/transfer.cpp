/* 
   transfer.cpp version 0.9 by me@bramp.net
   Class to actually download a file
*/

#include "stdafx.h"
#include "transfer.h"
#include "torrent.h"
#include "web.h"

/* Creates a transfer
 [in] data - The name of the torrent
*/
transfer::transfer(char *torrentFile) { //throws InvalidTorrentException, TorrentNotFoundException

   const char *filename;
   int size;
   int i=0;
   INT64 total = 0;
   fileIndex *lastIndex = new fileIndex;

   files = NULL;
   myTorrent = NULL;   
   
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
   
   /* Point the peices array at the correct thing */
   pieces = myTorrent->getInfoString("pieces");
   
   lastIndex = NULL;
   
   /* Create a list of files to work on */
   while (myTorrent->getFileName(i, &filename, &size)) {
      
      fileIndex *tmpIndex;
      file *aFile;

      /* Try to open/create the file */
      try {
         aFile = new file((char *)filename, size);
      } catch (file::FileErrorException e) {
         printf("Error: %i", e.Error);
         aFile=NULL;
      }

      if (aFile!=NULL) {

         /* Create new file index */
         tmpIndex = new fileIndex;

         total += (INT64)size;

         tmpIndex->data = aFile;
         tmpIndex->lastIdx = total;
         tmpIndex->nextIndex = NULL;
         
         if (lastIndex == NULL) {
            files = tmpIndex;
         } else {
            lastIndex->nextIndex = tmpIndex;
         }
         
         lastIndex = tmpIndex;
      
      }
      
      i++;
   }

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
   
   /* Delete the fileIndex list */
   while (files != NULL) {
      fileIndex *tmp = files;
      
      /* Cleanup the data */     
      delete files->data;
      files = files->nextIndex;
      
      /* And now the fileIndex */
      delete tmp;
      
   }

}


/******************************************************************************
Piece Object Starts Here
This represents a piece of the file either being downloaded or has been
******************************************************************************/

transfer::piece::piece(int pieceIndex, int len) {
   
   //char tempPath[MAX_PATH];
   
   /* Generate a random file name */
   //GetTempPath(MAX_PATH, tempPath);
   
   //GetTempFileName(tempPath, "bt0", 0, file);
}

transfer::piece::~piece() {

}

/******************************************************************************
File Object Starts Here
******************************************************************************/

transfer::file::file(char *pFileName, int length) {

   DWORD ret = 0;

   /* Make a copy of the filename */
   strncpy(fileName, pFileName, MAX_PATH);

   hFile = INVALID_HANDLE_VALUE;

   /* Open the file - Note its opened write, shared read, and RANDOM_ACCESS since 
      we are going to be jumping around alot */

   hFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);

   if (hFile == INVALID_HANDLE_VALUE) {
      throw FileErrorException(GetLastError());
   }
   
   length = 10;

   /* Now make sure the file is the correct length */
   ret = SetFilePointer(hFile, (DWORD) length, NULL, FILE_BEGIN);
   
   if (ret == INVALID_SET_FILE_POINTER) {
      int lastError;
      lastError = GetLastError();
      CloseHandle(hFile);
      throw FileErrorException(lastError);
   }

   /* Now save the file as this length */
   ret = SetEndOfFile(hFile);

   if (ret==0) {
      int lastError;
      lastError = GetLastError();   
      CloseHandle(hFile);
      throw FileErrorException(lastError);
   }
   
   /* Initialize the critical section */
   InitializeCriticalSection (&fileLock);
}

/* This function reads a specific part of the file */
int transfer::file::getPart(char *buffer, int start, int len) {
   DWORD bytesRead;
   int ret;
   
   /* Make sure no one else enters */
   EnterCriticalSection(&fileLock); 

   /* Move to correct position in file */   
   SetFilePointer(hFile, start, NULL, FILE_BEGIN);

   /* And fill the buffer */
   ret = ReadFile(hFile, buffer, len, &bytesRead, NULL);
   
   LeaveCriticalSection(&fileLock);
   
   /* Check for errors */
   if (ret==0) {
      return 0;
   } else {
      return bytesRead;
   }
      
}

/* This function writes a specific part of the file */
int transfer::file::setPart(char *buffer, int start, int len) {
   DWORD bytesWritten;
   int ret;

   /* Make sure no one else enters */
   EnterCriticalSection(&fileLock); 

   /* Move to correct position in file */ 
   SetFilePointer(hFile, start, NULL, FILE_BEGIN);
   
   /* Do the write */
   ret = WriteFile(hFile, buffer, len, &bytesWritten, NULL);
   
   LeaveCriticalSection(&fileLock);
   
   /* Check for errors */
   if (ret==0) {
      return 0;
   } else {
      return bytesWritten;
   }
}

transfer::file::~file() {

   /* Enter critical section so we have exclusive rights to close the file */
   EnterCriticalSection(&fileLock); 

   if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle (hFile);

   LeaveCriticalSection(&fileLock); 

   /* Clean up the CriticalSection */
   DeleteCriticalSection(&fileLock);
}