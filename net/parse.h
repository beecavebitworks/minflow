#ifndef _NET_PARSE_H_
#define _NET_PARSE_H_

#include <net/pktinfo.h>
#include <capture/pcap32.h>

#define PARSE_ERR_OK           0
#define PARSE_ERR_INCOMPLETE  -1
#define PARSE_ERR_UNSUPPORTED -2

extern int net_parse(pcap_pkthdr32_t *pcaphdr, char *buf, pktinfo_t *pktinfo);

#endif // _NET_PARSE_H_
