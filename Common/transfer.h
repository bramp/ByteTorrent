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
#include "file.h"

class transfer {
   private:

      /* This gets populated so I can map Pieces to files */
      class files {
      
         private:
            class fileIndex {
               public:
                  file *data;
                  int len;
                  fileIndex *nextIndex;
            };

            fileIndex *startIndex;
            fileIndex *lastIndex;
            
            /* Returns a file, start and len to write. Also returns true if we are to write to
               another file also */
            //bool findPart(INT64 begin, const file *file, const int *start, const int *len);
            file *findPart(INT64 *start);
            
         public:
            files();
            ~files();
            /* Finds which file(s) and sets a part in it */
            int getPart(char *buffer, INT64 start, int len);
            int setPart(char *buffer, INT64 start, int len);
            
            void addFile(const char *fileName, int size);
            
            INT64 length();
      };   

      class pieceMap {
         private:
            unsigned const char *PieceHash;
            int pieces;
            int pieceSize;
            unsigned char *map;
            bool checkPiece(char *fileBuffer, unsigned const char *pieceHash);
            DWORD WINAPI checkAllLoader( LPVOID lpParam );
            files *Files;
            CSHA1 sha;

         public:
            pieceMap(unsigned const char *pPieceHash, int pPieces, int pPieceSize, files *pFiles);
            ~pieceMap();
            int checkAll();
            bool check(int idx);
      };
      
      class connection { };
      
      torrent *myTorrent;
      const char *torrentName;
      char peerID[21];
      pieceMap *pieces;
      files *myFiles;
      char savePath[MAX_PATH];
      
   public:
      transfer(char *torrentFile);
      void setSaveLocation(char *pSavePath);
      void setup();
      void start();
      ~transfer(void);
};
