// ByteTorrent.cpp : Defines the entry point for the console application.
// http://bitconjurer.org/BitTorrent/protocol.html

#include "stdafx.h"
#include "common/transfer.h"

int _tmain(int argc, _TCHAR* argv[]) {
   
   transfer *download;
   
   char *filename = "C:\\downloads\\scrubs.s02e21.my.drama.queen-ftv.torrent";

   download = new transfer(filename);

   download->start();

   delete download;
   
	return 0;
}

