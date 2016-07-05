#ifndef _PCAP32_H_
#define _PCAP32_H_

#include <common/tval.h>

typedef struct pcap_pkthdr32
{
  tval_t             ts;
  unsigned int       caplen;
  unsigned int       len;
} pcap_pkthdr32_t;

#endif
