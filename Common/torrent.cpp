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
   torrent.cpp version 0.9 by me@bramp.net
   Class to store info on a torrent file
*/

#include "stdafx.h"
#include "torrent.h"
#include "becode.h"
#include "sha1.h"

/* Create a new torrent object from file */
Torrent::Torrent(char *fileName) {

   HANDLE inFile;

   beeData = NULL;

   /* Open the torrent file */
   inFile = CreateFile(fileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
   
   if (inFile != INVALID_HANDLE_VALUE) {
      DWORD bytesRead = 0;
      int readResult = 0;
      char *buffer;
      char *bufferPtr;
      int bufferSize;
      int bytes = 0;
      CSHA1 *sha1;
      
      bufferSize = GetFileSize(inFile, NULL);
      
      /* Check if the torrent is small enough to load */
      if (bufferSize > MAXTORRENTFILESIZE) {
         throw InvalidTorrentException();
      }
      
      buffer = (char *)malloc(bufferSize);
      bufferPtr = buffer;
      
      do { /* Main copy loops */
         readResult = ReadFile(inFile, bufferPtr, MIN(bufferSize, 1024), &bytesRead, NULL);
         if (readResult == 0)
            printf("%d", GetLastError());
         bufferPtr+=bytesRead;
         bufferSize -= 1024;
      } while (readResult != 0 && bufferSize > 0);
      
      /* Now close file */
      CloseHandle(inFile);
      
      try {
         /* Now decode the torrent file into a dict */
         beeData = (bee::Dictionary *)bee::decode(buffer, &bytes);
      } catch (bee::InvalidBeeDataException) {
         free(buffer);
         throw InvalidTorrentException();
      }

      /* Clean up buffer */            
      free(buffer);
      
      /* If it didn't turn out correct throw error */
      if (beeData->getType()!=bee::DictionaryType) {
         throw InvalidTorrentException();
      }
      
      /* Now work out SHA1 hash of the file */
      sha1 = new CSHA1();
      
      /* Get the encoded data for the info to make SHA1 */
      buffer = beeData->get("info")->encode();
      bufferSize = beeData->get("info")->mEncodeLen;
      
      /* Inilise some vars */
      infoHash[0] = '\0';
      niceHash[0] = '\0';
      
      /* Do some hashing and save it */
      sha1->Update((unsigned char *)buffer, bufferSize);
      sha1->Final();
      sha1->GetHash((unsigned char *)&infoHash[0]);
      sha1->ReportHash(&niceHash[0]);
      
      /* Add a NULL for good measures */
      infoHash[20] = '\0';
      niceHash[40] = '\0';
      
      delete sha1;
      
   } else { //If file doesn't exist or access error...
		throw TorrentNotFoundException();
	}
}

Torrent::~Torrent(void) {
   if (beeData != NULL)
      delete beeData;
}

const char *Torrent::getTracker() {
   bee::String *tracker;
   tracker = (bee::String *)beeData->get("announce");
   if (tracker != NULL)
      return tracker->get(); 
   else
      return NULL;
}

const char *Torrent::setTracker(char *newTracker) {
   bee::String *tracker;
   tracker = (bee::String *)beeData->get("announce");
   if (tracker != NULL) {
      tracker->set(newTracker);
      return tracker->get(); 
   } else {
      return NULL;
   }
}

/* Gives you a field from the info section of the torrent
 [in] key - Key to look up, ie pieces or name...
[ret] value - The value with the key, or NULL
*/
const unsigned char *Torrent::getInfoString(char *key) {
   bee::Dictionary *info;
   bee::String *value;
   
   info = (bee::Dictionary *)(beeData->get("info"));
   if (info == NULL) {
      return NULL;
   }
   
   value = (bee::String *)(info->get(key));
   if (value == NULL) {
      return NULL;
   }
   
   return (const unsigned char *)(value->get());
}

/* Gives you a field from the info section of the torrent
 [in] key - Key to look up, ie pieces or name...
[ret] value - The value with the key, or NULL
*/
INT64 Torrent::getInfoInteger(char *key) {
   bee::Dictionary *info;
   bee::Integer *value;
   
   info = (bee::Dictionary *)(beeData->get("info"));
   if (info == NULL) {
      return NULL;
   }
   
   value = (bee::Integer *)(info->get(key));
   if (value == NULL) {
      return NULL;
   }
   
   return (value->get());
}

/* Gives you filename/length of files in torrent
 [in] idx - Which number file you refering to (0, 1, 2...)
[out] path - The full path & filename of the file
[out] size - The size of the file
[ret] found - If we are returning a file or not
*/
bool Torrent::getFileName(int idx, const char **path, int *size) {
   
   int len;
   
   len = (int)getInfoInteger("length");
   
   /* Check if we have more than one file name */
   if (len!=0) {
      /* One filename */
      if (idx > 0) {
         return false;
      }
      
      *path = (const char *)getInfoString("name");
      *size = len;
      
   } else {
      /* Many filenames */
   }
   
   return true;
}

int Torrent::saveToFile(char *filename) {

   HANDLE hFile;
   DWORD bytesWritten;
   int ret;

   /* Open the torrent file */
   hFile = CreateFile(filename,GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
   
   if (hFile != INVALID_HANDLE_VALUE) {
      char *data;
      int dataLen;
      
      data = this->beeData->encode();
      dataLen = this->beeData->mEncodeLen;
      
      ret = WriteFile(hFile, data, dataLen, &bytesWritten, NULL);
      
      CloseHandle(hFile);
      free(data);
      
      if ((ret==0) || (bytesWritten==0)) {
         return (int)GetLastError();
      }
      
   } else {
      return (int)GetLastError();
   }
   
   return 0;
}