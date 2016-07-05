#ifndef _PKTINFO_H_
#define _PKTINFO_H_

// direction with respect to stream_key left,right
typedef enum PktDir {LEFT_TO_RIGHT=0, RIGHT_TO_LEFT=1,DIR_NONE=-1} PktDir;

#include <stdint.h>

#include <net/ether.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>

#include <net/streamkey.h>

typedef struct pktinfo_t
{
	ether_hdr_t *     ether;
	union {
	  ip_hdr_t *      ip4;
	  ip6_hdr_t *     ip6;
	};
	union {
	  tcp_hdr_t *     tcp;
	  udp_hdr_t *     udp;
	};
	int               payload_position;
	int               payload_len;

        uint8_t           isv6;
        uint8_t           istcp;
        uint8_t           tcpflags;
        uint8_t           isfin;
}
pktinfo_t;

extern int pkt_is_v6(pktinfo_t *pktinfo);

extern uint8_t pkt_tcp_flags(pktinfo_t *pktinfo);

extern void pkt_init(pktinfo_t *pktinfo);

extern int16_t pkt_get_vlan(pktinfo_t *pktinfo);	// -1 for none, vlan value otherwise

extern int16_t pkt_get_vlan2(pktinfo_t *pktinfo);	// -1 for none, vlan2 value otherwise

extern int pkt_init_key(pktinfo_t *pkt, stream_key_t *key);

extern char* pkt_to_s(char *dest, pktinfo_t *pkt);

extern char *pkt_src_mac(pktinfo_t *pktinfo);

extern char *pkt_dst_mac(pktinfo_t *pktinfo);

#endif // _PKTINFO_H_
