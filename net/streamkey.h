#ifndef _STREAMKEY_H_
#define _STREAMKEY_H_

#include <stdint.h>

typedef struct stream4_key_t
{
   uint16_t       lport;
   uint16_t       rport;
   uint8_t        ipproto;
   uint8_t        vflags;		// IPV6
   uint16_t       pad;			// align addrs on 32-bit

   uint8_t        addrs[8];
}
stream4_key_t;

#define VFLAG_IPV6 (1<<0)

typedef struct stream_key_t
{
   stream4_key_t  v4;
   uint8_t        addrs2[24];
}
stream_key_t;


extern int net_stream_is_v6(stream_key_t *key);

extern int net_stream_is_tcp(stream_key_t *key);

extern int net_stream4_is_tcp(stream4_key_t *key);

extern uint32_t net_stream4_left(stream_key_t *key);

extern uint32_t net_stream4_right(stream_key_t *key);

/*---------------------------------------------------
 * initialize key with v4 packet details
 * returns 1 if key flipped (left<->right) from packet direction (src<->dst)
 * return 0 otherwise
 *---------------------------------------------------*/
extern int
net_stream_init_v4(stream_key_t *key, int8_t ipproto,
                   uint32_t srcip, uint32_t dstip, uint16_t sport, uint16_t dport);

/*---------------------------------------------------
 * return a computed hash on the key
 *---------------------------------------------------*/
uint32_t hash_stream_key(const void *_key);

/*---------------------------------------------------
 * key compare function for hash table
 * returns 0 on equal, non-zero otherwise
 *---------------------------------------------------*/
int hcmp_stream_key(const void *_k1, const void *_k2);

char* streamkey_to_s(char *dest, stream_key_t *key);

#endif // _STREAMKEY_H_
