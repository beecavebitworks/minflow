#ifndef _READCAP_H_
#define _READCAP_H_

#include <fcntl.h>
#include <sys/types.h>
typedef unsigned short ushort;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned char u_char;
#include <pcap/pcap.h>

#include <common/tval.h>
#include <capture/pcap32.h>

typedef int (*callback_t)(pcap_pkthdr32_t *hdr, char *pkt, int pktnum);

int open_pcap(char *filename);
int read_pcap_loop(int fd, callback_t cb);

#endif // _READCAP_H_
