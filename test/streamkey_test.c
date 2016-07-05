#define DEBUG 1

#include<assert.h>
#include <net/streamkey.h>
#include <string.h>	// memset
#include <stdio.h>

#define IPPROTO_TCP 6

int main(int argc, char *argv[])
{

  stream_key_t key, key2, key3;

  uint32_t srcip1 = 0x03030A0C, dstip1=0x0B0B0103;
  uint16_t sport1 = 3333; uint16_t dport1 = 80;

  memset(&key, 0, sizeof(key));
  memset(&key2, 0, sizeof(key));
  memset(&key3, 0, sizeof(key));

  // key2 is based on reverse direction of key, should yield same values
 
  net_stream_init_v4(&key, IPPROTO_TCP, srcip1, dstip1, sport1, dport1);
  net_stream_init_v4(&key2, IPPROTO_TCP, dstip1, srcip1,dport1, sport1);

  assert(net_stream4_left(&key) == srcip1);
  assert(net_stream4_right(&key) == dstip1);
  assert(key.v4.sport == sport1);
  assert(key.v4.dport == dport1);

  assert(net_stream_is_tcp(&key));
  assert(!net_stream_is_v6(&key));

  assert(net_stream4_left(&key2) == srcip1);
  assert(net_stream4_right(&key2) == dstip1);
  assert(key2.v4.sport == sport1);
  assert(key2.v4.dport == dport1);

  assert(net_stream_is_tcp(&key2));
  assert(!net_stream_is_v6(&key2));

  assert(hash_stream_key(&key) == hash_stream_key(&key2));

  // create a key for different connection

  uint32_t srcip3 = 0x01020B0C, dstip3=0x12345678;
  uint16_t sport3 = 54321; uint16_t dport3 = 135;

  net_stream_init_v4(&key3, IPPROTO_TCP, dstip3, srcip3, dport3, sport3);

  // make sure hash is different than 1

  assert(hash_stream_key(&key) != hash_stream_key(&key3));

  assert(hcmp_stream_key(&key, &key2) == 0);	// must be same
  assert(hcmp_stream_key(&key2, &key3) != 0);	// must be diff

  printf("stream_key_t tests DONE\n");
}

