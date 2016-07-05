#ifndef _UDP_H_
#define _UDP_H_

typedef struct udphdr {
    u_int16_t   uh_sport;       /* source port */
    u_int16_t   uh_dport;       /* destination port */
    u_int16_t   uh_ulen;        /* udp length */
    u_int16_t   uh_sum;         /* udp checksum */
} udp_hdr_t;

#endif // _UDP_H_
