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
Nice little Torrent File Editing Console App
Allows you to change the name of the trackers for a bunch of files

CHANGELOG
0.95 Fixed a directory problem
0.94 Fixed a memory leak... opps
     Added better directory support ie for btchange c:\blah
0.93 Stopped it from thinking directories were files
     Put a limit on the size of torrents, 1mb currently
0.92 Fixed the last parameter
0.91 Fixed 2 parameters
0.90 Released to friend
*/

#include "stdafx.h"
#include "common/torrent.h"

#define VERSION "0.95"

/* Different error messages in the app */
#define FINDERROR "Please specify word to find\n"
#define FINDSTARERROR "Finds expression can't have wildcard characters (* or ?)\n"
#define REPLACEERROR "Please specify tracker to replace with\n"
#define NOMATCHERROR "Sorry no matchs\n"

/* Shows the default program usage */
void showUsage(_TCHAR* argv[]) {
   printf("BTChange %s - Easily changes trackers in many torrents (/? for more)\n\n", VERSION, argv[0]);
   printf("Usage: %s [-f find] [-r replace] [/?] [files]\n", argv[0]);
}

int _tmain(int argc, _TCHAR* argv[]) {
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind;
   char *targetFiles = "*.torrent";
   char *findTracker = NULL;
   char *replaceTracker = NULL;
  
   /* If no arguments show usage */
   if (argc==1) {
      showUsage (argv);
      return 1;
   }
   
   /* Check if all parameters */
   for (int i=1; i<argc; i++) {
      /* Check for help */
      if (!stricmp(argv[i], "/?") || !stricmp(argv[i], "/h") || !stricmp(argv[i], "-h")) {
         showUsage(argv);
         printf("      -f find     Finds all torrents using this tracker\n");
         printf("      -r replace  Replaces all matched trackers with this\n");
         printf("\nExample:  %s -f roop -r http://newurl:6969/announce\n", argv[0]);
         printf("This will find all trackers with the word roop in and replace\n");
         printf("the entire tracker string with the new url in all *.torrents\n");
         printf("\nExample:  %s A*.torrent\n", argv[0]);
         printf("Shows info for all torrents begining with A\n");
         printf("\nPlease use with care. You could accidently ruin all your torrents\n");
         printf("                                                 (bytetorrent.sf.net)\n"); 
         return 0;
      
      /* Check for find */
      } else if (!stricmp(argv[i], "-f")) {
         /* Check if there is another param, which doesn't start with - */
         if (i<(argc-1)) {
            
            if (*argv[i + 1] != '-') {
               i++;
               /* Check for wild cards */
               if (strchr(argv[i], '*')!=NULL || strchr(argv[i], '?')!=NULL) {
                  printf(FINDSTARERROR);
                  return 1;
               }
               findTracker = argv[i];
            } else {
               printf(FINDERROR);
               return 1;
            }
            
         } else {
            printf(FINDERROR);
            return 1;
         }
      } else if (!stricmp(argv[i], "-r")) {
         /* Check if there is another param, which doesn't start with - */
         if (i<(argc-1)) {            
            if (*argv[i + 1] != '-') {
               i++;
               replaceTracker = argv[i];
            } else {
               printf(REPLACEERROR);
               return 1;
            }
         } else {
            printf(REPLACEERROR);
            return 1;
         }
      } else if (i == argc-1) {
         targetFiles = argv[i];
      }
   }
   
   /* Start the find file */
   hFind = FindFirstFile(targetFiles, &FindFileData);

   /* Check for errors */
   if (hFind == INVALID_HANDLE_VALUE) {
      DWORD error = GetLastError();
      if (error == 2) {
         printf (NOMATCHERROR);
      } else {
         printf ("Error %d Occured\n", error);
      }
   } else {
      
      int matchs = 0;
      torrent *t;
      char currentFile[MAX_PATH];
      char searchDirectory[MAX_PATH];
      int searchDirectoryLen;
      
      /* Figure out the directory these files are in */
      searchDirectoryLen = (int)(strrchr(targetFiles, '\\') - targetFiles);
      if (searchDirectoryLen > 0) {
         strncpy(searchDirectory, targetFiles, searchDirectoryLen);
         searchDirectory[searchDirectoryLen] = '\0';
      } else {
         strcpy(searchDirectory, "");
      }
      
      /* Loop for each matched file */
      do {

         /* Ready to catch exceptions */
         try {
            /* Check the file isn't a directory */
            if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
               
               /* Find its full path */
               if (searchDirectory[0] != '\0')
                  _snprintf(currentFile, MAX_PATH, "%s\\%s", searchDirectory, FindFileData.cFileName);
               else
                  strncpy(currentFile, FindFileData.cFileName, MAX_PATH);
               
               /* Open the torrent */
               t = new torrent(currentFile);
               
               /* Check if this torrent matchs */
               if ( (findTracker==NULL) || (strstr(t->getTracker(), findTracker))!=NULL ) {
                  
                  matchs++;
                  printf ("%s\n", FindFileData.cFileName);

                  /* Check if we need to replace */
                  if (replaceTracker!=NULL) {
                     int ret;
                     
                     t->setTracker(replaceTracker);
                     ret = t->saveToFile(currentFile);
                     
                     if (ret!=0) {
                        printf("Error %i altering torrent file\n", ret);
                     }
                  }

                  printf ("Tracker: %s\nHash: %s\n\n", t->getTracker(), t->getNiceHash());

               } /* if ( (findTracker==NULL) || (strstr(t->getTracker(), findTracker))!=NULL ) */
               
               delete t;
            } /* if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) */
         
         } catch (torrent::TorrentNotFoundException) {
            printf ("%s\n", currentFile);
            printf ("Not Found\n\n");
         } catch (torrent::InvalidTorrentException) {
            printf ("%s\n", currentFile);
            printf ("Invalid Torrent File\n\n");
         }

      } while (FindNextFile(hFind, &FindFileData));
      
      FindClose(hFind);
      
      if (matchs==0) {
         printf (NOMATCHERROR);
      }
   }

	return 0;
}

