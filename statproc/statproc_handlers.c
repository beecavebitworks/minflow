// functions for stats process
#include <assert.h>
#include <common/sharedif.h>
#include <statproc/statproc_utils.h>
#include <net/ports.h>
#include <string.h> // memcpy
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/sort.h>
#include <export/export.h>

#ifndef min //(a,b)
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define DBG 0
#define TOPN_PROTOS 20
#define NUM_TOPN_STREAMS 20

#define IPPROTO_UDP 6
#define IPPROTO_TCP 17

#define IS_EMPTY_SLOT(entryptr) ((entryptr)->tstart.tv_sec == 0)

// local struct defs =============

typedef struct
{
   flow_stats_t               flow_stats[SI_NUM_FLOWTYPES];
   uint32_t                   ifdrops;                  // interface drops
   uint32_t                   kdrops;                   // kernel drops

   mga_info_t                 addr_lan_indexes;         // location of internal addr_info
   mga_info_t                 addr_wan_indexes;         // location of external addr_info
   mga_info_t                 stream_indexes;
}
second_stats_t;

static flow_stats_t sec_flows[SI_NUM_FLOWTYPES][60 + 8];
static int         sec_i=0;

// forward function decls =================


void _process_streams(uint32_t ts, sharedif_t *g_shm);
void _process_addrs(uint32_t ts, sharedif_t *g_shm);
void _process_protocols(uint32_t ts, sharedif_t *g_shm);
static void inc_sec_min_ctrs(uint32_t packets, uint32_t bytes);
static void mark_dest_port(addr_info_t *addr, uint16_t port);
static void mark_serve_port(addr_info_t *addr, uint16_t port);


// Global variables ================

static second_stats_t stats_last_sec;
static second_stats_t stats_cur_sec;
static int initialized=0;

static uint32_t tlast_min = 0;
static uint64_t last_min_packets=0;
static uint64_t last_min_bytes=0;

uint32_t gProfileNumMins = 0;
uint32_t gProfileSumSeconds = 0;

int cmp_stream_pkt(void *_a, void *_b);
int rcmp_stream_pkt(void *_a, void *_b);
int cmp_stream_bytes(void *_a, void *_b);
int rcmp_stream_bytes(void *_a, void *_b);
int cmp_protos_pkt(void *_a, void *_b);
int rcmp_protos_pkt(void *_a, void *_b);
int rcmp_addr_txpkt(void *_a, void *_b);
int rcmp_addr_scanned_ports(void *_a, void *_b);
int rcmp_addr_served_ports(void *_a, void *_b);

//#define DIFF(f) u32diff( stats_last_sec.##f, stats_cur_sec.f)

static sharedif_t *shm_copy;
static sharedif_t *shm_last;
static stream_info_t **stream_ptrs;
static addr_info_t   **addr_ptrs;
static proto_info_t  **proto_ptrs;

static addr_info_t **addr_ptrs_scanners;
static addr_info_t **addr_ptrs_targets;

#define CALC_OFF_SIZE(mga, off, size, typ) {       \
  off = (mga).idx_lo * sizeof(typ);                  \
  size = ((mga).idx_hi - (mga).idx_lo) * sizeof(typ); \
}

#define PRINT_SEC(T) do {\
  printf("STAT,S%d,", FLOW_TYPE_##T); \
  for (int i=0;i<60;i++) printf("%u,%u,",sec_flows[FLOW_TYPE_##T][i].packets, sec_flows[FLOW_TYPE_##T][i].bytes);\
  printf("\n"); \
} while (0)

//-------------------------------------------------------------------
// run minute interval stats and report
//-------------------------------------------------------------------
void
check_minute(uint32_t ts, sharedif_t *g_shm)
{
  //if ((ts - tlast_min) < 60) return;

  // a minute has elapsed ====

  time_t profileMinStart = time(NULL);
  gProfileNumMins++;

  if (ts == 0) {
    // end of capture file
    ts = tlast_min + 60;
  }

  on_stat_period_open(ts, sec_i);

  printf("STAT,T,%llu,%llu\n", last_min_packets, last_min_bytes);

  PRINT_SEC(WEB_IN);
  PRINT_SEC(WEB_OUT);
  PRINT_SEC(NONWEB_IN);
  PRINT_SEC(NONWEB_OUT);
  PRINT_SEC(INTERNAL);
 
  // copy indexes to working copy (lan, wan, streams)

  memcpy(&shm_copy->addr_lan_indexes, &g_shm->addr_lan_indexes, sizeof(mga_info_t)*3);

  uint32_t soff, ssize;
  uint32_t loff, lsize;
  uint32_t woff, wsize;

  CALC_OFF_SIZE(shm_copy->stream_indexes, soff, ssize, stream_info_t);
  memcpy((char *)&shm_copy->streams + soff, (char *)&g_shm->streams + soff, ssize);

  CALC_OFF_SIZE(shm_copy->addr_lan_indexes, loff, lsize, addr_info_t);
  memcpy((char *)&shm_copy->addrs + loff, (char *)&g_shm->addrs + loff, lsize);

  CALC_OFF_SIZE(shm_copy->addr_wan_indexes, woff, wsize, addr_info_t);
  memcpy((char *)&shm_copy->addrs + woff, (char *)&g_shm->addrs + woff, wsize);

  memcpy((char *)&shm_copy->protos , (char *)&g_shm->protos, SI_NUM_PROTOS*sizeof(proto_info_t));

  printf("STAT,I,streams:%d,lanaddrs:%d,wanaddrs:%d\n", shm_copy->stream_indexes.num_entries,
       shm_copy->addr_lan_indexes.num_entries,
       shm_copy->addr_wan_indexes.num_entries);
  
  // calculate DIFFs of counter

  _process_streams(ts, g_shm);			// do streams before addrs
  _process_protocols(ts, g_shm);
  _process_addrs(ts, g_shm);

  // reset counters and timestamp

  last_min_packets = 0;
  last_min_bytes = 0;

  tlast_min = (ts / 60 ) * 60;
  sec_i = ts % 60;

  gProfileSumSeconds += time(NULL) - profileMinStart;

  on_stat_period_close(60);
}

void _process_addrs2(uint32_t ts, sharedif_t *g_shm, uint32_t flowflags, mga_info_t *indexes);
 
//---------------------------------------------------------------
// figure out topN for addresses
//---------------------------------------------------------------
void _process_addrs(uint32_t ts, sharedif_t *g_shm)
{
  _process_addrs2(ts, g_shm, FLOW_TYPE_INTERNAL, &shm_copy->addr_lan_indexes);
  _process_addrs2(ts, g_shm, 0, &shm_copy->addr_wan_indexes);
}

uint16_t count_bits16(uint16_t val)
{
  uint16_t num=0;
  for (int i=0;i<16;i++) {
    num += (val & 0x01);
    val = val >> 1;
  }
  return num;
}

// keep an eye on address that connect to (or are targeted) too many ports in a minute
#define SUSPICIOUS_PORTS_MIN  10

//---------------------------------------------------------------
// calculate topN addresses for flowflags
//---------------------------------------------------------------
void _process_addrs2(uint32_t ts, sharedif_t *g_shm, uint32_t flowflags, mga_info_t *indexes)
{
  uint32_t num_addrs=0;
  uint32_t num_addrs_scanners=0;
  uint32_t num_addrs_targets=0;

  for (uint32_t i= indexes->idx_lo+1;i < indexes->idx_hi;i++)
  {
    addr_info_t *addr = &shm_copy->addrs[i];

    if (IS_EMPTY_SLOT(addr)) continue;

    // first time through the list (called with flowflags==0), tally up port counts for this address

    if (flowflags == FLOW_TYPE_INTERNAL) // port-scan for LAN only
    {
      addr->resvd = count_bits16(addr->resvd);
      addr->resvd2 = count_bits16(addr->resvd2);

      //char pbuf[42];
      //printf("%s destports=%d srvports=%d\n", addr_info_to_s(pbuf, addr), addr->resvd, addr->resvd2);

      if (addr->resvd > SUSPICIOUS_PORTS_MIN)
        addr_ptrs_scanners[num_addrs_scanners++] = addr;

      if (addr->resvd2 > SUSPICIOUS_PORTS_MIN)
        addr_ptrs_targets[num_addrs_targets++] = addr;
    }

    // do we already have this entry from last interval?

    if (tval_cmp(&addr->tstart, &shm_last->addrs[i].tstart)== 0)
    {
      // existing entry.  Take diff of counters
      uint32_t dtxpackets = u32diff(shm_last->addrs[i].txpackets, addr->txpackets);
      uint32_t dtxbytes = u32diff(shm_last->addrs[i].txbytes, addr->txbytes);
      uint32_t drxpackets = u32diff(shm_last->addrs[i].rxpackets, addr->rxpackets);
      uint32_t drxbytes = u32diff(shm_last->addrs[i].rxbytes, addr->rxbytes);

      // copy to last
      shm_last->addrs[i].txpackets = addr->txpackets;
      shm_last->addrs[i].txbytes = addr->txbytes;
      shm_last->addrs[i].rxpackets = addr->rxpackets;
      shm_last->addrs[i].rxbytes = addr->rxbytes;

      // put diff in cur
      addr->txpackets = dtxpackets;
      addr->txbytes = dtxbytes;
      addr->rxpackets = drxpackets;
      addr->rxbytes = drxbytes;
    }
    else
    {
      // different stream than one in last.  copy to last

      memcpy(&shm_last->addrs[i], addr, sizeof(addr_info_t));
    }

    addr_ptrs[num_addrs] = addr;
    num_addrs++;

  }

  //printf("num_LAN addrs=%d sorting...\n", num_addrs);

  // sort by pkts desc

  quicksort((void**)addr_ptrs, 0, num_addrs-1, rcmp_addr_txpkt);

  for (int i=0;i<min(10,num_addrs);i++)
  {
    if (addr_ptrs[i] == NULL) continue;

    on_topn_address(ts, SortByTxPkt, flowflags, i, addr_ptrs[i]);
  }

  if (num_addrs_scanners > 0)
  {
    // sort by num ports desc

    quicksort((void**)addr_ptrs_scanners, 0, num_addrs_scanners-1, rcmp_addr_scanned_ports);

    for (int i=0;i<min(10,num_addrs_scanners);i++)
    {
      if (addr_ptrs_scanners[i] == NULL) continue;

      on_topn_address(ts, SortByScannedPorts, flowflags, i, addr_ptrs_scanners[i]);
    }
  }

  if (num_addrs_targets > 0)
  {
    // sort by num ports desc

    quicksort((void**)addr_ptrs_targets, 0, num_addrs_targets-1, rcmp_addr_served_ports);

    for (int i=0;i<min(10,num_addrs_targets);i++)
    {
      if (addr_ptrs_targets[i] == NULL) continue;

      on_topn_address(ts, SortByServedPorts, flowflags, i, addr_ptrs_targets[i]);
    }
  }

}  

//---------------------------------------------------------------
// figure out topN for protocols
//---------------------------------------------------------------
void _process_protocols(uint32_t ts, sharedif_t *g_shm)
{
  // populate arrays of pointers to valid elements

  uint32_t num_protos=0;
  for (uint32_t i=0;i<SI_NUM_PROTOS;i++)
  {
    if (shm_copy->protos[i].packets == 0) continue;

    if (shm_copy->protos[i].packets == shm_last->protos[i].packets) continue; // hasnt changed

    {
      // existing entry.  Take diff of counters
      uint32_t dpackets = u32diff(shm_last->protos[i].packets, shm_copy->protos[i].packets);
      uint32_t dbytes = u32diff(shm_last->protos[i].bytes, shm_copy->protos[i].bytes);

      // copy to last
      shm_last->protos[i].packets = shm_copy->protos[i].packets;
      shm_last->protos[i].bytes = shm_copy->protos[i].bytes;

      // put diff in cur
      shm_copy->protos[i].packets = dpackets;
      shm_copy->protos[i].bytes = dbytes;
    }

    proto_ptrs[num_protos] = &shm_copy->protos[i];
    num_protos++;
  }

  //printf("num_protocols=%d sorting...\n", num_protos);


  // sort streams by pkts desc
  quicksort((void**)proto_ptrs, 0, num_protos-1, rcmp_protos_pkt);

  for (int i=0;i<min(TOPN_PROTOS,num_protos);i++)
  {
    if (proto_ptrs[i] == NULL) continue;

    // determine index (also port number) from offset
    uint16_t port = ((char*)proto_ptrs[i]-(char*)shm_copy->protos)/sizeof(proto_info_t);

    // there are 9100 TCP ports, followed by 9100 UDP ports

    uint8_t ipproto = IPPROTO_TCP;
    if (port >= SI_NUM_TCP_PROTOS) {
      port -= SI_NUM_TCP_PROTOS;
      ipproto = IPPROTO_UDP;
    }

    on_topn_protocol(ts, SortByPkt, 0, i, port, ipproto, proto_ptrs[i]);
  }
}  

//---------------------------------------------------------------
// figure out topN for streams
//---------------------------------------------------------------
void _process_streams(uint32_t ts, sharedif_t *g_shm)
{
  // populate arrays of pointers to valid elements

  uint32_t num_streams=0;
  for (uint32_t i=shm_copy->stream_indexes.idx_lo+1;i<shm_copy->stream_indexes.idx_hi;i++)
  {
    stream_info_t *stream = &shm_copy->streams[i];

    if (IS_EMPTY_SLOT(stream)) continue;

    if (tval_cmp(&stream->tstart, &shm_last->streams[i].tstart)== 0)
    {
      // existing entry.  Take diff of counters
      uint32_t dpackets = u32diff(shm_last->streams[i].packets, stream->packets);
      uint32_t dbytes = u32diff(shm_last->streams[i].bytes, stream->bytes);

      // copy to last
      shm_last->streams[i].packets = stream->packets;
      shm_last->streams[i].bytes = stream->bytes;

      // put diff in cur
      stream->packets = dpackets;
      stream->bytes = dbytes;
    }
    else
    {
      // different stream than one in last.  copy to last

      memcpy(&shm_last->streams[i], stream, sizeof(stream_info_t));
    }

    // keep track of all dest ports from each address to help with port scan detection

    addr_info_t *aleft=&shm_copy->addrs[stream->idx_left];
    if (aleft->internal) mark_dest_port(aleft,stream->rport);

    addr_info_t *aright=&shm_copy->addrs[stream->idx_right];
    if (aright->internal) mark_serve_port(aright,stream->rport);

    // add to list for sorting, increment

    stream_ptrs[num_streams] = stream;
    num_streams++;
  }

  //printf("num_streams=%d sorting...\n", num_streams);

  // sort streams by pkts desc

  quicksort((void**)stream_ptrs, 0, num_streams-1, rcmp_stream_pkt);

  for (int i=0;i<min(NUM_TOPN_STREAMS,num_streams);i++)
  {
    if (stream_ptrs[i] == NULL) continue;

    addr_info_t *aleft=&shm_copy->addrs[stream_ptrs[i]->idx_left];
    addr_info_t *aright=&shm_copy->addrs[stream_ptrs[i]->idx_right];

    on_topn_stream(ts, SortByPkt, 0, i, stream_ptrs[i], aleft, aright);
  }

  // sort streams by bytes desc

  quicksort((void**)stream_ptrs, 0, num_streams-1, rcmp_stream_bytes);

  for (int i=0;i<min(NUM_TOPN_STREAMS,num_streams);i++)
  {
    if (stream_ptrs[i] == NULL) continue;

    addr_info_t *aleft=&shm_copy->addrs[stream_ptrs[i]->idx_left];
    addr_info_t *aright=&shm_copy->addrs[stream_ptrs[i]->idx_right];

    on_topn_stream(ts, SortByBytes, 0, i, stream_ptrs[i], aleft, aright);
  }
}  

//---------------------------------------------------------------
// initialize state
//---------------------------------------------------------------
static void _init_state(tval_t ts, sharedif_t *g_shm)
{
    initialized = 1;

    // important: keep minute reports on 60-second boundaries

    tlast_min = (ts.tv_sec / 60 ) * 60;
    sec_i = ts.tv_sec % 60;

    memcpy(&stats_last_sec, (second_stats_t *)&g_shm->flow_stats, sizeof(second_stats_t));

    shm_copy = calloc(sizeof(sharedif_t), 1); // alloc memory for working copy
    shm_last = calloc(sizeof(sharedif_t), 1); // alloc memory for prev working copy

    stream_ptrs = (stream_info_t **)calloc(SI_NUM_STREAMS * sizeof(void*),1);
    addr_ptrs = calloc(SI_NUM_ADDRS * sizeof(void*),1);
    proto_ptrs = (proto_info_t **)calloc(SI_NUM_PROTOS * sizeof(void*),1);

    addr_ptrs_scanners = calloc((SI_NUM_ADDRS/2) * sizeof(void*),1);
    addr_ptrs_targets  = calloc((SI_NUM_ADDRS/2) * sizeof(void*),1);

}

//---------------------------------------------------------------
// every_second
//---------------------------------------------------------------
void every_second(tval_t ts, sharedif_t *g_shm)
{
  if (!initialized) {
    _init_state(ts,g_shm);
    return;
  }

  // take snapshot

  memcpy(&stats_cur_sec, (second_stats_t *)&g_shm->flow_stats, sizeof(second_stats_t));

  // diff

  uint32_t ifdrops = u32diff(stats_last_sec.ifdrops, stats_cur_sec.ifdrops);

  for(int i=0;i<SI_NUM_FLOWTYPES;i++)
  {
    sec_flows[i][sec_i].packets = u32diff(stats_last_sec.flow_stats[i].packets, stats_cur_sec.flow_stats[i].packets);
    sec_flows[i][sec_i].bytes = u32diff(stats_last_sec.flow_stats[i].bytes, stats_cur_sec.flow_stats[i].bytes);

    if (i == FLOW_TYPE_TOTAL)
    {
      last_min_packets += sec_flows[i][sec_i].packets;
      last_min_bytes += sec_flows[i][sec_i].bytes;
    }
  }
  sec_i++;
  assert(sec_i <= 60);

  // copy current to last

  memcpy(&stats_last_sec, &stats_cur_sec, sizeof(stats_cur_sec));


  // if we are at minute boundary, cycle

  if ((ts.tv_sec - tlast_min) >= 60)
    check_minute(ts.tv_sec, g_shm);
}

void sim_finish(uint32_t tv_sec, sharedif_t *h_shm)
{
  check_minute(tv_sec, h_shm);
}

//---------------------------------------------------------------
// set port bit
//---------------------------------------------------------------
static void _set_port_bit(uint16_t *pbitmask, uint16_t port)
{
  switch(port)
  {
    case 21:
      (*pbitmask |= BF_PORT_21);break;
    case 22:
      (*pbitmask |= BF_PORT_22);break;
    case 23:
      (*pbitmask |= BF_PORT_23);break;
    case 53:
      (*pbitmask |= BF_PORT_53);break;
    case 80:
      (*pbitmask |= BF_PORT_80);break;

    case 123:
      (*pbitmask |= BF_PORT_123);break;
    case 135:
      (*pbitmask |= BF_PORT_135);break;
    case 137:
      (*pbitmask |= BF_PORT_137);break;
    case 138:
      (*pbitmask |= BF_PORT_138);break;

    case 161:
      (*pbitmask |= BF_PORT_161);break;
    case 162:
      (*pbitmask |= BF_PORT_162);break;
    case 389:
      (*pbitmask |= BF_PORT_389);break;
    case 443:
      (*pbitmask |= BF_PORT_443);break;

    case 445:
      (*pbitmask |= BF_PORT_445);break;
    case 37:
      (*pbitmask |= BF_PORT_37);break;
    case 8080:
      (*pbitmask |= BF_PORT_8080);break;

    default:
     break;
  }
}

//---------------------------------------------------------------
// mark_dest_port
// use the resvd field
//---------------------------------------------------------------
static void mark_dest_port(addr_info_t *addr, uint16_t port)
{
  _set_port_bit(&addr->resvd, port);
}

//---------------------------------------------------------------
// mark_serve_port
// use the resvd2 field
//---------------------------------------------------------------
static void mark_serve_port(addr_info_t *addr, uint16_t port)
{
  _set_port_bit(&addr->resvd2, port);
}

//============== comparators for sorting ===================

//------------------------------------------------------------
// protocol sortby packet ASC
//------------------------------------------------------------
int cmp_protos_pkt(void *_a, void *_b)
{
  proto_info_t *a = (proto_info_t *)_a;
  proto_info_t *b = (proto_info_t *)_b;                 
   
  if (a->packets < b->packets) return -1;   
  if (a->packets > b->packets) return 1;    
   
  return 0; // equal
}

int rcmp_protos_pkt(void *_a, void *_b) { return cmp_protos_pkt(_b,_a); }

//------------------------------------------------------------
// address sortby txpackets ASC
//------------------------------------------------------------
int cmp_addr_txpkt(void *_a, void *_b)
{
  addr_info_t *a = (addr_info_t *)_a;
  addr_info_t *b = (addr_info_t *)_b;                 
   
  if (a->txpackets < b->txpackets) return -1;   
  if (a->txpackets > b->txpackets) return 1;    
   
  return 0; // equal
}

int rcmp_addr_txpkt(void *_a, void *_b) { return cmp_addr_txpkt(_b,_a); }


//------------------------------------------------------------
// address sortby PortScanSource likeliness ASC
//------------------------------------------------------------
int cmp_addr_port_scan_src(void *_a, void *_b)
{
  addr_info_t *a = (addr_info_t *)_a;
  addr_info_t *b = (addr_info_t *)_b;                 
   
  if (a->resvd < b->resvd) return -1;   
  if (a->resvd > b->resvd) return 1;    
   
  return 0; // equal
}
int rcmp_addr_scanned_ports(void *_a, void *_b) { return cmp_addr_port_scan_src(_b,_a); }

//------------------------------------------------------------
// address sortby PortScanTarget likeliness ASC
//------------------------------------------------------------
int cmp_addr_port_scan_target(void *_a, void *_b)
{
  addr_info_t *a = (addr_info_t *)_a;
  addr_info_t *b = (addr_info_t *)_b;                 
   
  if (a->resvd2 < b->resvd2) return -1;   
  if (a->resvd2 > b->resvd2) return 1;    
   
  return 0; // equal
}
int rcmp_addr_served_ports(void *_a, void *_b) { return cmp_addr_port_scan_target(_b,_a); }

//------------------------------------------------------------
// stream sortby packets ASC
//------------------------------------------------------------
int cmp_stream_pkt(void *_a, void *_b)
{
  stream_info_t *a = (stream_info_t *)_a;
  stream_info_t *b = (stream_info_t *)_b;                 
   
  if (a->packets < b->packets) return -1;   
  if (a->packets > b->packets) return 1;    
   
  return 0; // equal
}

int rcmp_stream_pkt(void *_a, void *_b) { return cmp_stream_pkt(_b,_a); }

//------------------------------------------------------------
// stream sortby bytes ASC
//------------------------------------------------------------
int cmp_stream_bytes(void *_a, void *_b)
{
  stream_info_t *a = (stream_info_t *)_a;
  stream_info_t *b = (stream_info_t *)_b;                 
   
  if (a->bytes < b->bytes) return -1;   
  if (a->bytes > b->bytes) return 1;    
   
  return 0; // equal
}
int rcmp_stream_bytes(void *_a, void *_b) { return cmp_stream_bytes(_b,_a); }

