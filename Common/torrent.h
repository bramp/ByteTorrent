/* 
   torrent.cpp version 0.9 by me@bramp.net
   Class to store info on a torrent file
*/

#pragma once

#include "becode.h"

#define MAXTORRENTFILESIZE 1048576 /* 1MB Max size of a torrent in bytes */

class torrent {
   private:
      bee::dictionary *beeData;
      char infoHash[21];
      char niceHash[41];
      
   public:
      class TorrentNotFoundException : protected Exception {};
      class InvalidTorrentException : protected Exception {};
         
      torrent(char *fileName); /* Open a torrent from file */
      ~torrent(void);
      bee::dictionary *getBeeObject() { return beeData; };
      const char *getInfoHash() { return infoHash; };
      const char *getNiceHash() { return niceHash; };
      const char *getTracker();
      const char *setTracker(char *newTracker);
      const unsigned char *getInfoString(char *key);
      INT64 getInfoInteger(char *key);
      bool getFileName(int idx, const char **path, int *size);
      
      int saveToFile(char *filename);
};
