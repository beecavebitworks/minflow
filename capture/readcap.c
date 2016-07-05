#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "readcap.h"

#define MAX_PKT_BYTES 1700

//-------------------------------------------------------------
// open_pcap - return file descriptor
//-------------------------------------------------------------
int open_pcap(char *filename)
{
   int fd = open(filename, O_RDONLY);
   if (fd <= 0) return fd;

   // skip over file header for now.  TODO: validate

   lseek(fd, sizeof(struct pcap_file_header), SEEK_SET);

   return fd;   
}


//-------------------------------------------------------------
// read_pcap_loop
//-------------------------------------------------------------
int read_pcap_loop(int fd, callback_t cb)
{
  int pktnum=0;
  int num;
  char tmpbuf[MAX_PKT_BYTES];

  while (1)
  {
    struct pcap_pkthdr32 hdr;
    num = read(fd, &hdr, sizeof(hdr));

    if (num < sizeof(hdr)) return -1;

    num = read(fd, tmpbuf, hdr.caplen);

    if (num < hdr.caplen) return -2;

    pktnum++;

    if (cb(&hdr, tmpbuf, pktnum) != 0) return -3;

  }

  return 0;
}

