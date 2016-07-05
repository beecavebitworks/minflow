#include <net/streamkey.h>
#include <net/ip.h>
#include <string.h>	// memset
#include <net/iputils.h>
#include <stdio.h>

int net_stream_is_v6(stream_key_t *key)
{
   return ((key->v4.vflags & VFLAG_IPV6) == VFLAG_IPV6);
}

int net_stream_is_tcp(stream_key_t *key)
{
   return (key->v4.ipproto == IPPROTO_TCP);
}

int net_stream4_is_tcp(stream4_key_t *key)
{
   return (key->ipproto == IPPROTO_TCP);
}

#define SET_LEFT(_key, _ipaddr) *(uint32_t *)((_key)->v4.addrs) = (_ipaddr);
#define SET_RIGHT(_key, _ipaddr) *(uint32_t *)((_key)->v4.addrs+4) = (_ipaddr);

#define GET_LEFT(_key) (*(uint32_t *)((_key)->v4.addrs))
#define GET_RIGHT(_key) (*(uint32_t *)((_key)->v4.addrs+4))

//------------------------------------------------
// srcip
//------------------------------------------------
uint32_t net_stream4_left(stream_key_t *key)
{
  return *(uint32_t *)(key->v4.addrs);
}

uint32_t net_stream4_right(stream_key_t *key)
{
  return *(uint32_t *)(key->v4.addrs+4);
}

//------------------------------------------------
// net_stream_init_v4
// keep key consistent, irregardless of which direction packet has
// sport > dport || (sport==dport && srcip < dstip)
//------------------------------------------------
int net_stream_init_v4(stream_key_t *key, int8_t ipproto, uint32_t srcip, uint32_t dstip, uint16_t sport, uint16_t dport)
{
  memset(key, 0, sizeof(*key));
  key->v4.ipproto = ipproto;
  
  if (sport > dport || (sport == dport && srcip < dstip))
  {
    key->v4.lport = sport;
    key->v4.rport = dport;
    SET_LEFT(key, srcip);
    SET_RIGHT(key, dstip);

    return 0; // not flipped
  }
  else
  {
    key->v4.lport = dport;
    key->v4.rport = sport;
    SET_LEFT(key, dstip);
    SET_RIGHT(key, srcip);

    return 1; // flipped
  }
}

//--------------------------------------------
// compare stream keys (for HashTable)
// returns 0 if equal, non-zero otherwise
//--------------------------------------------
int hcmp_stream_key(const void *_k1, const void *_k2)
{
  int val = memcmp(_k1,_k2,sizeof(stream_key_t));
/*
  stream_key_t *a = (stream_key_t *)_k1;
  stream_key_t *b = (stream_key_t *)_k2;

  char pbuf1[80];
  char pbuf2[80];

  streamkey_to_s(pbuf1, a);
  streamkey_to_s(pbuf2, b);

  printf("HCMP_key %s %s %s\n", (val == 0 ? "SAME" : "NEQ"), pbuf1, pbuf2);
*/
  return val;
}

//--------------------------------------------
// compute simple hash on stream key (for HashTable)
//--------------------------------------------
uint32_t
hash_stream_key(const void *_key)
{
  stream_key_t *key = (stream_key_t *)_key;

  uint32_t val = *(uint32_t *)key;
  val = GET_LEFT(key) ^ GET_RIGHT(key);
  val *= key->v4.rport;
  val = val ^ key->v4.lport;

  //char pbuf[80];
  //printf ("HASH: %s %u\n", streamkey_to_s(pbuf, key), val);

  if (!net_stream_is_v6(key)) return val;

  val = val ^ *(uint32_t *)key->addrs2;
  val = val ^ *(uint32_t*)(key->addrs2+4);
  val = val ^ *(uint32_t*)(key->addrs2+8);
  val = val ^ *(uint32_t*)(key->addrs2+12);
  val = val ^ *(uint32_t*)(key->addrs2+16);

  return val;
}

extern char* get_proto_name(uint8_t ipproto);	// net/pktinfo.c

//-----------------------------------------------
// to_s
//-----------------------------------------------
char* streamkey_to_s(char *dest, stream_key_t *key)
{
  uint16_t sport, dport;

  if (0) //pkt_is_v6(pkt))
  {
    sprintf(dest, "IPV6 STREAM HERE");
    //sprintf(dest, "%s_%s_%d_%s_%d", get_proto_name(pkt->ip6.ip_p),"?:?:?",sport, "?:?:?", dport);
    return dest;
  }
  else
  {
    char leftip[16], rightip[16];
    ip2string(leftip, GET_LEFT(key));
    ip2string(rightip, GET_RIGHT(key));

    sprintf(dest, "%s_%s_%d_%s_%d", get_proto_name(key->v4.ipproto), leftip, key->v4.lport, rightip, key->v4.rport);
    return dest;
  }

}
