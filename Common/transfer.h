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
