/*
Nice little Torrent File Editing Console App

CHANGELOG
0.93 Stopped it from thinking directories were files
     Put a limit on the size of torrents, 1mb currently
0.92 Fixed the last parameter
0.91 Fixed 2 parameters
0.90 Released to friend
*/

#include "stdafx.h"
#include "common/torrent.h"

#define FINDERROR "Please specify word to find\n"
#define FINDSTARERROR "Finds expression can't have wildcard characters (* or ?)\n"
#define REPLACEERROR "Please specify tracker to replace with\n"
#define NOMATCHERROR "Sorry no matchs\n"


void showUsage(_TCHAR* argv[]) {
   printf("BTChange 0.93 - Easily changes trackers in many torrents (/? for more)\n\n", argv[0]);
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
         printf("                                                  by me@bramp.net\n"); 
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
      
      /* Loop for each matched file */
      do {

         /* Ready to catch exceptions */
         try {
            /* Check the file isn't a directory */
            if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
               /* Open the torrent */
               t = new torrent(FindFileData.cFileName);
               
               /* Check if this torrent matchs */
               if ( (findTracker==NULL) || (strstr(t->getTracker(), findTracker))!=NULL ) {
                  
                  matchs++;
                  printf ("%s\n", FindFileData.cFileName);

                  /* Check if we need to replace */
                  if (replaceTracker!=NULL) {
                     int ret;
                     
                     t->setTracker(replaceTracker);
                     ret = t->saveToFile(FindFileData.cFileName);
                     
                     if (ret!=0) {
                        printf("Error %i altering torrent file\n", ret);
                     }
                  }

                  printf ("Tracker: %s\nHash: %s\n\n", t->getTracker(), t->getNiceHash());

               } /* if ( (findTracker==NULL) || (strstr(t->getTracker(), findTracker))!=NULL ) */

            } /* if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) */
         
         } catch (torrent::TorrentNotFoundException) {
            printf ("%s\n", FindFileData.cFileName);
            printf ("Not Found\n\n");
         } catch (torrent::InvalidTorrentException) {
            printf ("%s\n", FindFileData.cFileName);
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

