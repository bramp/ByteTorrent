/* 
   torrent.cpp version 0.9 by me@bramp.net
   Class to store info on a torrent file
*/

#include "stdafx.h"
#include "torrent.h"
#include "becode.h"
#include "sha1.h"

/* Create a new torrent object from file */
torrent::torrent(char *fileName) {

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
         readResult = ReadFile(inFile, bufferPtr, 1024, &bytesRead, NULL);
         bufferPtr+=bytesRead;
      } while (bytesRead != 0 && readResult != 0);
      
      /* Now close file */
      CloseHandle(inFile);
      
      try {
         /* Now decode the torrent file into a dict */
         beeData = (bee::dictionary *)bee::decode(buffer, &bytes);
      } catch (bee::InvalidBeeDataException) {
         free(buffer);
         throw InvalidTorrentException();
      }

      /* Clean up buffer */            
      free(buffer);
      
      /* If it didn't turn out correct throw error */
      if (beeData->getType()!=bee::dictionaryType) {
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

torrent::~torrent(void) {
   if (beeData != NULL)
      delete beeData;
}

const char *torrent::getTracker() {
   bee::string *tracker;
   tracker = (bee::string *)beeData->get("announce");
   if (tracker != NULL)
      return tracker->getString(); 
   else
      return NULL;
}

const char *torrent::setTracker(char *newTracker) {
   bee::string *tracker;
   tracker = (bee::string *)beeData->get("announce");
   if (tracker != NULL) {
      tracker->setString(newTracker);
      return tracker->getString(); 
   } else {
      return NULL;
   }
}

/* Gives you a field from the info section of the torrent
 [in] key - Key to look up, ie pieces or name...
[ret] value - The value with the key, or NULL
*/
const unsigned char *torrent::getInfoString(char *key) {
   bee::dictionary *info;
   bee::string *value;
   
   info = (bee::dictionary *)(beeData->get("info"));
   if (info == NULL) {
      return NULL;
   }
   
   value = (bee::string *)(info->get(key));
   if (value == NULL) {
      return NULL;
   }
   
   return (const unsigned char *)(value->getString());
}

/* Gives you a field from the info section of the torrent
 [in] key - Key to look up, ie pieces or name...
[ret] value - The value with the key, or NULL
*/
INT64 torrent::getInfoInteger(char *key) {
   bee::dictionary *info;
   bee::integer *value;
   
   info = (bee::dictionary *)(beeData->get("info"));
   if (info == NULL) {
      return NULL;
   }
   
   value = (bee::integer *)(info->get(key));
   if (value == NULL) {
      return NULL;
   }
   
   return (value->getInt());
}

/* Gives you filename/length of files in torrent
 [in] idx - Which number file you refering to (0, 1, 2...)
[out] path - The full path & filename of the file
[out] size - The size of the file
[ret] found - If we are returning a file or not
*/
bool torrent::getFileName(int idx, const char **path, int *size) {
   
   int len;
   
   len = (int)getInfoInteger("length");
   
   /* Check if we have more than one file name */
   if (len!=NULL) {
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

int torrent::saveToFile(char *filename) {

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