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
   log.cpp version 0.9 by me@bramp.net
   Simple logging class
*/

#include "StdAfx.h"
#include "log.h"

HANDLE Log::logFile;
CRITICAL_SECTION Log::logMutex;

void Log::getTime(char *buffer) {
   SYSTEMTIME sTime;
   
   //Lets get system time
   GetSystemTime(&sTime);
   
   sprintf(buffer, "[%02i/%02i/%i %02i:%02i:%02i UTC] " , sTime.wDay, sTime.wMonth, sTime.wYear, sTime.wHour, sTime.wMinute, sTime.wSecond);
      
}

void Log::writeFile(char *buffer) {
   DWORD bytesWritten;   
   
   EnterCriticalSection(&logMutex);
   WriteFile(logFile, buffer, (DWORD)strlen(buffer), &bytesWritten, NULL);
   WriteFile(logFile, "\r\n", 2, &bytesWritten, NULL);
   FlushFileBuffers(logFile);
   LeaveCriticalSection(&logMutex);
}

void Log::OpenLog(char *filename) {
   logFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   
   if (logFile==NULL)
      return;
      
   SetFilePointer (logFile,0,0,FILE_END);
   InitializeCriticalSection(&logMutex);
}

void Log::CloseLog() {
   DeleteCriticalSection (&logMutex);
   CloseHandle(logFile);
}

void Log::AddMsg(char *message, void *arg1, void *arg2) {
   char buffer[1024];

   sprintf(buffer, message, arg1, arg2);
   writeFile(buffer);
}

void Log::AddErrMsg(char *message, DWORD err, void *arg1, void *arg2) {
   char buffer[1024];
   char *bufferPtr;
   char error[1024];

   sprintf(buffer, message, arg1, arg2);
   bufferPtr = buffer + strlen(buffer);

   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, err, 0, (LPTSTR)error, 0, NULL);
   
   sprintf(bufferPtr, " [%d] %s ", err, error);
   
   LocalFree(error);

   writeFile(buffer);
}

void Log::AddWsaMsg(char *message, int err, void *arg1, void *arg2) {
   char buffer[1024];
   char *bufferPtr;

   sprintf(buffer, message, arg1, arg2);
   bufferPtr = buffer + strlen(buffer);
   
   sprintf(bufferPtr, " [WSA %d] ", err);

   writeFile(buffer);
}