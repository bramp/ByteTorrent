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
   transfer.h version 0.9 by me@bramp.net
   Class to actually download a file
*/

#pragma once

#include "torrent.h"

class transfer {
   private:
   
      /* This class allows accept to files in a random and thread-safe way */
      class file { 
         private:
            HANDLE hFile;
            char fileName[MAX_PATH];
            int fileLength;
            CRITICAL_SECTION fileLock;
            
         public:
            class FileErrorException : protected Exception {
               public:
                  DWORD Error;
                  FileErrorException(DWORD pError) { Error = pError; };
            };
                     
            file(char *pFileName, int length);
            int getPart(char *buffer, int start, int len);
            int setPart(char *buffer, int start, int len);
            ~file();
      };

      /* This gets populated so I can map peices to files */
      class fileIndex {
         public:
            file *data;
            INT64 lastIdx;
            fileIndex *nextIndex;
      };   
      
      class connection { };
      
      class piece {
         private:
            int pieceIndex;
         
         public:  
            piece(int index, int len);
            int getIndex() { return pieceIndex; };
            ~piece();
         
      };
      
      torrent *myTorrent;
      const char *torrentName;
      char peerID[21];
      int pieceLen;
      unsigned const char *pieces;
      fileIndex *files;
      
   public:
      transfer(char *torrentFile);
      void start();
      ~transfer(void);
};
