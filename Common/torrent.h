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
   torrent.h version 0.9 by me@bramp.net
   Class to store info on a torrent file
*/

#pragma once

#include "becode.h"

#define MAXTORRENTFILESIZE 1048576 /* 1MB Max size of a torrent in bytes */

class Torrent {
   private:
      bee::Dictionary *beeData;
      char infoHash[21];
      char niceHash[41];
      
   public:
      class TorrentNotFoundException : public Exception {};
      class InvalidTorrentException : public Exception {};
         
      Torrent(char *fileName); /* Open a torrent from file */
      ~Torrent(void);
      bee::Dictionary *getObject() { return beeData; };
      const char *getInfoHash() { return infoHash; };
      const char *getNiceHash() { return niceHash; };
      const char *getTracker();
      const char *setTracker(char *newTracker);
      const unsigned char *getInfoString(char *key);
      INT64 getInfoInteger(char *key);
      bool getFileName(int idx, const char **path, int *size);
      
      int saveToFile(char *filename);
};
