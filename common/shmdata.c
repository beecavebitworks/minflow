#include <common/shmdata.h>
#include <net/streamkey.h>
#include <utils/hashtable.h>
#include <utils/mgarray.h>
#include <common/sharedif.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // calloc
#include <assert.h>
#include <net/iputils.h>
#include <net/ip.h>
#include <utils/ipc.h>
#include <capture/cleanup.h>

#define STREAM_EXPIRED_SEC 30
#define DBG 0

extern char *get_proto_name(uint8_t ipproto);	// net/pktinfo.c

static int g_num_int_addrs    = 0;
static int g_num_ext_addrs    = 0;
static int g_num_streams      = 0;

static sharedif_t *g_shm = NULL;
static int g_shm_handle=0;

static HashTable *ht_streams = NULL;
static HashTable *ht_v4addrs = NULL;

static mgarray_t g_lan_addrs;
static mgarray_t g_wan_addrs;
static mgarray_t g_streams;

static uint32_t gAddrCount=0;
static uint32_t gStreamCount=0;

typedef struct
{
  uint32_t a[4];
}
addr_key_t;

static addr_key_t zero_addr = {0,0,0,0};

void addr_key_init_v4(addr_key_t *dest, uint32_t addr);

// forward decls

entry_status_t stream_isempty_func(char *_entry, mgaop_t op);
entry_status_t addr_isempty_func(char *_entry, mgaop_t op);

uint32_t hash_v4addr_key(const void *_key);
int hcmp_v4addr_key(const void *_k1, const void *_k2);

//--------------------------------------------
// shm_stats_print
//--------------------------------------------
void shm_stats_print()
{
  printf("G pkt=%d bytes=%d streams=%d lan_addrs=%d wan_addrs=%d\n",
    g_shm->flow_stats[FLOW_TYPE_TOTAL].packets, g_shm->flow_stats[FLOW_TYPE_TOTAL].bytes,
    g_shm->stream_indexes.num_entries, g_shm->addr_lan_indexes.num_entries, g_shm->addr_wan_indexes.num_entries);
}

//--------------------------------------------
// shm_inc_global  increment global packet counters
//--------------------------------------------
void shm_inc_global(tval_t ts, int pktlen)
{
  g_shm->flow_stats[FLOW_TYPE_TOTAL].packets++;
  g_shm->flow_stats[FLOW_TYPE_TOTAL].bytes+= pktlen;

  //g_shm->packets++;
  //g_shm->bytes += pktlen;
  g_shm->tnow = ts;
  if (g_shm->tstart.tv_sec == 0) g_shm->tstart = ts;
}

//--------------------------------------------
// shm_data_init
//--------------------------------------------
HSHMDATA shm_data_init(int use_shm)
{
  if (use_shm)
  {
    g_shm_handle = shmem_create(SHMID_MINFLOWCAP, sizeof(sharedif_t));
    if (g_shm_handle < 0) {
      printf("Failed to create shared memory segment\n");
      return NULL;
    }

    g_shm = (sharedif_t*)shmem_open(SHMID_MINFLOWCAP, sizeof(sharedif_t));
    if (g_shm == NULL) {
      printf("ERROR: unable to get pointer to shared memory segment\n");
      return NULL;
    }

    // zero it out

    memset(g_shm, 0, sizeof(sharedif_t));
  }
  else
  {
    // we are in retrospective mode, just use regular memory

    g_shm = calloc(sizeof(sharedif_t), 1);
    if (g_shm == NULL)
    {
      printf("Failed to malloc sharedif_t %u bytes\n", (unsigned int)sizeof(sharedif_t));
      return NULL;
    }
  }

  // initialize members

  g_shm->magic = SHAREDIF_MAGIC;

  ht_streams = HashTableCreate(SI_NUM_STREAMS / 5);
  HashTableSetKeyComparisonFunction(ht_streams, hcmp_stream_key);
  // TODO: override key delete func, since not pointers

  // by default, value compare func checks pointers for equality
  HashTableSetHashFunction(ht_streams, hash_stream_key);

  ht_v4addrs = HashTableCreate(SI_NUM_ADDRS / 5);
  HashTableSetKeyComparisonFunction(ht_v4addrs, hcmp_v4addr_key);

  // by default, value compare func checks pointers for equality
  HashTableSetHashFunction(ht_v4addrs, hash_v4addr_key);

  mga_init(&g_lan_addrs, (char *)g_shm->addrs, &g_shm->addr_lan_indexes, sizeof(addr_info_t), addr_isempty_func, 0, 255);
  mga_init(&g_wan_addrs, (char *)g_shm->addrs, &g_shm->addr_wan_indexes, sizeof(addr_info_t), addr_isempty_func, 256, SI_NUM_ADDRS-1);
  mga_init(&g_streams, (char *)g_shm->streams, &g_shm->stream_indexes, sizeof(stream_info_t), stream_isempty_func, 0, SI_NUM_STREAMS-1);

  return g_shm;
}

void stream_init_key_from_stream(stream_key_t *key, stream_info_t *stream)
{
  addr_info_t *laddr = shm_addr_get(stream->idx_left);
  addr_info_t *raddr = shm_addr_get(stream->idx_right);

  if (laddr->isv6)
  {
    // TODO
    assert(0);
  }
  else
  {
    net_stream_init_v4(key, stream->ipproto, laddr->addr_offset, raddr->addr_offset, stream->lport, stream->rport);
  }
}

//--------------------------------------------
// lookup v4 addr in hashtable
//--------------------------------------------
addr_info_t *shm_addr_lookup_v4(uint32_t ipaddr, uint32_t *idx)
{
  addr_key_t key;
  addr_key_init_v4(&key, ipaddr);

  addr_info_t *ai;
  ai = (addr_info_t *) HashTableGet(ht_v4addrs, &key);

  if (ai == NULL) {*idx = 0; return ai; }

  // calc index from pointer

  *idx = (((char *)ai - (char *)&g_shm->addrs[0]) / sizeof(addr_info_t));

  return ai;
}

//--------------------------------------------
// delete stream key from lookup map
//--------------------------------------------
void shm_stream_lookup_free(stream_key_t *key)
{
  HashTableRemove(ht_streams, key);
}
//--------------------------------------------
// find free stream info entry
//--------------------------------------------
stream_info_t *shm_stream_alloc(stream_key_t *key)
{
  uint32_t idx;
  stream_info_t *val = (stream_info_t *)mga_alloc_entry(&g_streams, &idx);

  if (val == NULL) return NULL;

  HashTablePut(ht_streams, key, val);

  // initialize / clear out old values

  val->iid = gStreamCount++;
  val->flowflags = 0;
  val->tcpflags = 0;
  val->vlan = 0;
  val->packets = 0; val->bytes = 0;
  val->_packets = 0; val->_bytes = 0;

  return val;
}

//--------------------------------------------
// find stream info
//--------------------------------------------
stream_info_t *shm_stream_lookup(stream_key_t *key)
{
  stream_info_t *val = (stream_info_t *)HashTableGet(ht_streams, key);
  return val;
}

#define ETHER_MAC_LEN 6

//--------------------------------------------
// free addr_info
//--------------------------------------------
void shm_addr_free(uint32_t idx)
{
  if (idx >= g_lan_addrs.total_entries) {
    if (DBG) printf("FREE WAN %d\n",idx);
    mga_free_entry(&g_wan_addrs, idx);
  } else {
    if (DBG) printf("FREE LAN %d\n",idx);
    mga_free_entry(&g_lan_addrs, idx);
  }
}

//--------------------------------------------
// allocate (mark and return) addr_info entry
// NOTE: caller must initialize tstart upon return
//--------------------------------------------
addr_info_t *shm_addr_alloc_v4(uint32_t ipaddr, char *mac_addr, uint32_t *pidx)
{
  addr_info_t *addr;

  if (ipaddr_is_lan(ipaddr))
  {
    addr = (addr_info_t *)mga_alloc_entry(&g_lan_addrs, pidx);
    if (addr != NULL) addr->internal = 1;
  }
  else
  {
    addr = (addr_info_t *)mga_alloc_entry(&g_wan_addrs, pidx);
    if (addr != NULL) addr->internal = 0;
  }

  if (addr == NULL) {*pidx=0; return addr; }

  if (DBG) {
    char dbuf[80];
    printf("%s: pidx=%d %s\n", (addr->internal ? "LAN":"WAN"), *pidx, mga_to_s(dbuf,&g_lan_addrs));
  }

  // initialize

  addr->iid = gAddrCount++;
  addr->isv6 = 0;
  addr->resvd = 0; addr->resvd2 = 0;
  addr->addr_offset = ipaddr;
  addr->nstreams = 0; addr->nstreams_active = 0;
  addr->txbytes = 0; addr->txpackets = 0;
  addr->rxpackets = 0; addr->txbcastpkt = 0;

  memcpy(addr->mac, mac_addr, ETHER_MAC_LEN);

  addr_key_t *pkey = (addr_key_t *)malloc(sizeof(addr_key_t));
  addr_key_init_v4(pkey, ipaddr);

  HashTablePut(ht_v4addrs, (void *)pkey, addr);

  return addr;
}

uint32_t _check_addr_index(uint32_t idx)
{
  if (idx < SI_NUM_ADDRS) return idx;

  shm_stats_print();
  assert(0);

  return 0;
}

// This is going outside the mga_xx API, so be careful.  We
// Use two managed arrays in different areas of array memory.
// caller does not care which portion they are in, they just
// have an index.
#define GET_ADDR(index)  &g_shm->addrs[_check_addr_index(index)]

//--------------------------------------------
// get pointer to index'th entry
//--------------------------------------------
addr_info_t *shm_addr_get(uint32_t index)
{
  return GET_ADDR(index);
}

//--------------------------------------------
// return 1 if entry is valid, 0 if its free
//--------------------------------------------
int is_addr_slot_empty(addr_info_t *entry)
{
  return (entry->tstart.tv_sec == 0 ? 1 : 0);
}

//--------------------------------------------
// return 1 if entry is valid, 0 if its free
//--------------------------------------------
int is_stream_slot_empty(stream_info_t *entry)
{
  return (entry->tstart.tv_sec == 0 ? 1 : 0);
}

//--------------------------------------------
// mark slot as empty
//--------------------------------------------
void set_stream_slot_empty(stream_info_t *entry)
{
  int idx = ((char *)entry - (char *)g_streams.ptr) / g_streams.entry_size;

  mga_free_entry(&g_streams, idx);
}

//--------------------------------------------
// mark slot as empty
//--------------------------------------------
void find_expired_streams(uint32_t tv_sec)
{
  for(int i=g_streams.p->idx_lo+1;i<g_streams.p->idx_hi;i++)
  {
    stream_info_t *ss = (stream_info_t *)mga_get(&g_streams, i);
    if (is_stream_slot_empty(ss)) continue;

    if ((tv_sec - ss->tlast.tv_sec) > STREAM_EXPIRED_SEC)
    {
      add_stream_to_cleanup_list(tv_sec, ss);
    }
  }
}

//--------------------------------------------
// user function for addr_info_t mgarray
//--------------------------------------------
entry_status_t
addr_isempty_func(char *_entry, mgaop_t op)
{
  addr_info_t *entry = (addr_info_t *)_entry;
  entry_status_t status = (entry->tstart.tv_sec == 0 ? ENTRY_AVAIL : ENTRY_TAKEN);
  
  switch(op)
  {
    case MGA_OP_SET:
     entry->tstart.tv_sec = 1; break;
  
    case MGA_OP_TSTSET:
     if (status == ENTRY_AVAIL) {
       entry->tstart.tv_sec = 1;
     }
     break;
 
    case MGA_OP_CLR:
     entry->tstart.tv_sec = 0; break;

    case MGA_OP_TST: 
    default:
     break;
  }

  return status;
}


//--------------------------------------------
// user function for stream_info_t mgarray
//--------------------------------------------
entry_status_t
stream_isempty_func(char *_entry, mgaop_t op)
{
  stream_info_t *entry = (stream_info_t *)_entry;
  entry_status_t status = (entry->tstart.tv_sec == 0 ? ENTRY_AVAIL : ENTRY_TAKEN);
  
  switch(op)
  {
    case MGA_OP_SET:
     entry->tstart.tv_sec = 1; break;
  
    case MGA_OP_TSTSET:
     if (status == ENTRY_AVAIL) {
       entry->tstart.tv_sec = 1;
     }
     break;
 
    case MGA_OP_CLR:
     entry->tstart.tv_sec = 0; break;

    case MGA_OP_TST: 
    default:
     break;
  }

  return status;
}

//--------------------------------------------
// compare v4addr keys (for HashTable)
// returns 0 if equal
//--------------------------------------------
int hcmp_v4addr_key(const void *_k1, const void *_k2)
{
  return memcmp(_k1,_k2,sizeof(addr_key_t));
}

//--------------------------------------------
// compute simple hash on v4addr key (for HashTable)
//--------------------------------------------
uint32_t
hash_v4addr_key(const void *_key)
{
  addr_key_t *key = (addr_key_t*)_key;
  uint32_t val = key->a[0] ^ key->a[1] ^ key->a[2] ^ key->a[3];
  return val;
}

//--------------------------------------------
// write address to string
//--------------------------------------------
char *
addr_info_to_s(char *dest, addr_info_t *addr)
{
  if (addr->isv6)
  {
    sprintf(dest, "V6 TODO");
  }
  else
  {
    ip2string(dest,addr->addr_offset);
  }
  return dest;
}

//--------------------------------------------
// write stream to string
// Format:
//  PROTO|leftaddr:leftport|rightaddr:rightport
//--------------------------------------------
char *
stream_info_to_s(char *dest, stream_info_t *stream, addr_info_t *aleft, addr_info_t *aright)
{
  char left[32],right[32];
  if (aleft == NULL) aleft = shm_addr_get(stream->idx_left);
  if (aright == NULL) aright = shm_addr_get(stream->idx_right);
  sprintf(dest,"%s|%s:%d|%s:%d", get_proto_name(stream->ipproto),
    addr_info_to_s(left, aleft), stream->lport,
    addr_info_to_s(right, aright), stream->rport);
  return dest;
}

char *
stream_info_to_s2(char *dest, stream_info_t *stream)
{
  char left[32],right[32];
  sprintf(dest,"%s_idx=%d_%d|idx=%d_%d", (stream->ipproto == IPPROTO_TCP ? "TCP":"?"),
    stream->idx_left, stream->lport,
    stream->idx_right, stream->rport);
  return dest;
}

//--------------------------------------------
// write MAC address to string
//--------------------------------------------
char *addr_mac_to_s(char *dest, addr_info_t *addr)
{
  uint8_t *m = addr->mac;

  // TODO: support IPV6 MAC (8-byte?)

  sprintf(dest,"%02x:%02x:%02x:%02x:%02x:%02x", m[0],m[1],m[2],m[3],m[4],m[5]);

  return dest;
}

void addr_key_init_v4(addr_key_t *dest, uint32_t addr)
{
  *dest=zero_addr;
  dest->a[0] = addr;
}

//--------------------------------------------
// increment flow_stat counters
//
//--------------------------------------------
void shm_inc_flow_stats(stream_info_t *ss, int len)
{
  uint8_t i = ss->flowflags & 0x07;
  assert (i < SI_NUM_FLOWTYPES) ;

  g_shm->flow_stats[i].packets++;
  g_shm->flow_stats[i].bytes+= len;
}

void shm_update_capstats(int recv, int ifdrops, int kdrops)
{
  g_shm->ifdrops = ifdrops;
  g_shm->ifdrops = kdrops;
}

proto_info_t *shm_get_protocol(uint8_t ipproto, uint16_t rport)
{
  if (rport >= SI_NUM_TCP_PROTOS) return NULL;

  // There are 9100 entries for TCP, followed by 9100 entries for UDP

  if (ipproto == IPPROTO_UDP) rport += SI_NUM_TCP_PROTOS;

  return &g_shm->protos[rport];
}

