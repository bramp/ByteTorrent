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
   file.cpp version 0.9 by me@bramp.net
   This class allows accept to files in a random and thread-safe way
*/

#include "stdafx.h"
#include "file.h"

File::File(char *pFileName, int length) {

   DWORD ret = 0;

   /* Make a copy of the filename */
   strncpy(fileName, pFileName, MAX_PATH);

   hFile = INVALID_HANDLE_VALUE;

   /* Open the file - Note its opened write, shared read, and RANDOM_ACCESS since 
      we are going to be jumping around alot */

   hFile = CreateFile(fileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);

   if (hFile == INVALID_HANDLE_VALUE) {
      ret = GetLastError();
      throwError(fileName, ret);
   }

   /* Now make sure the file is the correct length */
   ret = SetFilePointer(hFile, (DWORD) length, NULL, FILE_BEGIN);
   
   if (ret == INVALID_SET_FILE_POINTER) {
      ret = GetLastError();
      CloseHandle(hFile);
      hFile = NULL;
      throwError(fileName, ret);
   }

   /* Now save the file as this length */
   ret = SetEndOfFile(hFile);

   if (ret==0) {
      ret = GetLastError();   
      CloseHandle(hFile);
      hFile = NULL;
      throwError(fileName, ret);
   }
   
   /* Initialize the critical section */
   InitializeCriticalSection (&fileLock);
}

/* This function reads a specific part of the file */
int File::getPart(char *buffer, int start, int len) {
   DWORD bytesRead;
   int ret;
   
   /* Make sure no one else enters */
   EnterCriticalSection(&fileLock); 

   /* Move to correct position in file */   
   ret = SetFilePointer(hFile, start, NULL, FILE_BEGIN);

   /* And fill the buffer */
   ret = ReadFile(hFile, buffer, len, &bytesRead, NULL);
   
   LeaveCriticalSection(&fileLock);
   
   /* Check for errors */
   if (ret==0) {
      ret = GetLastError();
      throwError(fileName, ret);
      return 0;
   } else {
      return bytesRead;
   }
      
}

/* This function writes a specific part of the file */
int File::setPart(char *buffer, int start, int len) {
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

File::~File() {

   /* Enter critical section so we have exclusive rights to close the file */
   EnterCriticalSection(&fileLock); 

   if (hFile != INVALID_HANDLE_VALUE)
      CloseHandle (hFile);

   LeaveCriticalSection(&fileLock); 

   /* Clean up the CriticalSection */
   DeleteCriticalSection(&fileLock);
}

void File::throwError(char *pFilename, DWORD pError) {
  
   /* If we know this type of error throw a special case */
   switch (pError) {
      case ERROR_DISK_FULL:
         throw DiskFullException(pFilename);
      case ERROR_ACCESS_DENIED:
         throw AccessDeniedException(pFilename);
      default:
          throw ErrorException(pFilename, pError);
   }
}