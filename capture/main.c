#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <common/sharedif.h>
#include <capture/readcap.h>
#include <net/parse.h>
#include <net/pktinfo.h>
#include <common/shmdata.h>
#include <statproc/statproc_sim.h>
#include <capture/cleanup.h>
#include <string.h> // strcpy
#include <unistd.h> // getopt
#include <net/iputils.h>

#ifdef LINUX
#include <utils/sigsegv.h>	// only works on linux
#endif

#define DBG 0


#ifdef NOPCAP
  static int READ_FROM_FILE=1;
#else
  static int READ_FROM_FILE=0;
#endif // NOPCAP

void inc_addr_ctrs(addr_info_t *left, addr_info_t *right, PktDir dir, int pktlen);
void set_stream_flow_flags(stream_info_t *ss, addr_info_t *shm_left, addr_info_t *shm_right);
void inc_protocol_counters(uint8_t ipproto, uint16_t rport, PktDir dir, int len);
int is_web(uint16_t port);

extern void sim_finish(uint32_t tv_sec, HSHMDATA h);

static HSHMDATA h_shm;
static uint32_t last_sec = 0;
int gVerbose=0;

//----------------------------------------------------------
// increment stream counters
//----------------------------------------------------------
void inc_stream_counters(stream_info_t *si, PktDir dir, int pktlen)
{
  si->packets++;
  si->bytes+=pktlen;

  addr_info_t *addr_left = shm_addr_get(si->idx_left);
  addr_info_t *addr_right = shm_addr_get(si->idx_right);

  // inc num streams for each address if this is a new stream

  if (si->packets == 1)
  {
    addr_left->nstreams++;
    addr_right->nstreams++;
    addr_left->nstreams_active++;
    addr_right->nstreams_active++;
  }

  inc_addr_ctrs(addr_left, addr_right, dir, pktlen);
}

//----------------------------------------------------------
// increment address counters
//----------------------------------------------------------
void inc_addr_ctrs(addr_info_t *left, addr_info_t *right, PktDir dir, int pktlen)
{
  if (dir == LEFT_TO_RIGHT)
  {
    left->txpackets++;
    left->txbytes += pktlen;
    right->rxpackets++;
    right->rxbytes += pktlen;
  }
  else
  {
    right->txpackets++;
    right->txbytes += pktlen;
    left->rxpackets++;
    left->rxbytes += pktlen;
  }

  // TODO: bcast_packets?
}

#define PKT_MAC_LEFT(pktinfo, is_flipped) ((is_flipped) ? pkt_dst_mac(pktinfo) : pkt_src_mac(pktinfo))
#define PKT_MAC_RIGHT(pktinfo, is_flipped) ((is_flipped) ? pkt_src_mac(pktinfo) : pkt_dst_mac(pktinfo))

//----------------------------------------------------------
// on packet
//----------------------------------------------------------
int on_packet(pcap_pkthdr32_t *hdr, char *pkt, int pktnum)
{
  if (READ_FROM_FILE) sim_before_packet(hdr->ts, h_shm);

  //printf("%d:%d caplen=%d len=%d\n", hdr->ts.tv_sec, hdr->ts.tv_usec, hdr->caplen, hdr->len);

  pktinfo_t pktinfo;

  int status = net_parse(hdr, pkt, &pktinfo);

  if (status != PARSE_ERR_OK) return 0;		// TODO: return error value?

  // make stream key
  int8_t is_flipped;
  stream_key_t key;

  is_flipped = pkt_init_key(&pktinfo, &key);
  PktDir dir = (is_flipped ? RIGHT_TO_LEFT : LEFT_TO_RIGHT );	// dir of packet in relation to key src/dst

  // dont support 0 addrs

  if (net_stream4_left(&key) == 0 || net_stream4_right(&key) == 0) return 0;

  shm_inc_global(hdr->ts, hdr->len);

  if (pkt_is_v6(&pktinfo)) return 0; // TODO: support IPV6

  // lookup stream state

  stream_info_t *ss = shm_stream_lookup(&key);
  if (ss == NULL)
  {
     ss = shm_stream_alloc(&key);
     if (ss == NULL) {
       if (DBG) printf("Unable to allocate stream\n");
       return 0;
     }

     // allocate addr_info
     uint32_t idx_left, idx_right;
     addr_info_t *shm_left,*shm_right;

     if (pkt_is_v6(&pktinfo))
     {
       // TODO: support IPV6
       return 0;
     }
     else
     {
       shm_left = shm_addr_lookup_v4(net_stream4_left(&key), &idx_left);
       if (shm_left == NULL) {
         shm_left = shm_addr_alloc_v4(net_stream4_left(&key), PKT_MAC_LEFT(&pktinfo, is_flipped), &idx_left);
         if (shm_left != NULL) {
           shm_left->tstart = hdr->ts;
         } else
           if (DBG) printf("ADDR alloc failed **************\n");
       }

       shm_right = shm_addr_lookup_v4(net_stream4_right(&key), &idx_right);
       if (shm_right == NULL) {
         shm_right = shm_addr_alloc_v4(net_stream4_right(&key), PKT_MAC_RIGHT(&pktinfo, is_flipped), &idx_right);
         if (shm_right != NULL) {
           shm_right->tstart = hdr->ts;
         } else
           if (DBG) printf("ADDR alloc failed **************\n");
       }
       ss->ipproto = key.v4.ipproto;
       ss->lport = key.v4.lport;
       ss->rport = key.v4.rport;

     }

     ss->idx_left = idx_left;
     ss->idx_right = idx_right;
     ss->tstart = hdr->ts;

     set_stream_flow_flags(ss, shm_left, shm_right);

     //char sbuf[80];
     //printf("NEW stream:%s\n", stream_info_to_s(sbuf, ss, NULL, NULL));
  }

  ss->tlast = hdr->ts;
 
  if (pktinfo.istcp)
  {
    ss->tcpflags |= pktinfo.tcpflags;			// stream flags is OR of all packets

    if (pktinfo.isfin && (ss->flowflags & FLOW_FLAG_CLEANUP) == 0)
    {
      add_stream_to_cleanup_list(hdr->ts.tv_sec, ss);
      ss->flowflags |= FLOW_FLAG_CLEANUP;		// special flag used for cleanup
    }
  } 

  if (ss->packets == 1)
  {
    // this is second packet. Classify vlan

    ss->vlan = pkt_get_vlan(&pktinfo);
  }

  //char sbuf[80];
  //printf("stream:%s\n", stream_info_to_s(sbuf, ss));

  inc_stream_counters(ss, dir, hdr->len);

  // protocols and vlans

  shm_inc_flow_stats(ss, hdr->len);
  inc_protocol_counters(ss->ipproto, ss->rport, dir, hdr->len);

  if (READ_FROM_FILE) sim_after_packet(hdr->ts, h_shm);

  if ((hdr->ts.tv_sec - last_sec) > 0) {
    cleanup_expired_streams(hdr->ts.tv_sec);
    last_sec = hdr->ts.tv_sec;
  }

  return 0;
}


int main_file(char *filename)
{

  int fd = open_pcap(filename);
  if (fd <= 0) {
    printf("ERROR: unable to open %s\n", filename);
    return -1;
  }

  read_pcap_loop(fd, on_packet);

  sim_finish(0, h_shm);		// finish

  extern uint32_t gProfileSumSeconds;
  extern uint32_t gProfileNumMins;

  //printf("#PROFILER: %d minute reports, average %2.2f seconds processing\n", gProfileNumMins,
  //     ((gProfileSumSeconds + 0.0) / gProfileNumMins));

  close(fd);

  return 0;
}


//----------------------------------------------------------
// usage()
//----------------------------------------------------------
void usage(char *cmdname)
{
  printf("usage: %s <options>\n", cmdname);
  printf(" options:\n");
  printf(" -v            Verbose output\n");
  printf(" -f <file>     Read in a capture file\n");
  printf("\n");
  exit(0);
}

//----------------------------------------------------------
// main()
//----------------------------------------------------------
int main(int argc, char *argv[])
{
  int vflag=0, ch;
  char filename[128]="";

  while ((ch = getopt(argc, argv, "vf:")) != -1) {

    switch (ch) {

    case 'v':
      gVerbose = 1;
      break;

    case 'f':
      strcpy(filename, optarg);
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }

  if ((argc - optind) < 1 && strlen(filename) <= 0) usage(argv[0]);

  argc -= optind;
  argv += optind;

  if (argc > 0) strcpy(filename, argv[0]);

  //setup_sigsegv();

  //printf("sharedif = %u bytes\n", (unsigned int)sizeof(sharedif_t));

  h_shm = shm_data_init(!READ_FROM_FILE);
  if (h_shm == 0L)
  {
    printf("ERROR: unable to initialize data structures\n");
    return 0;
  }

  cleanup_init();

  if (READ_FROM_FILE)
  {
    //char *filename="/opt/data/terraria.cap";
    //char *filename="/opt/data/portscan.cap";
    main_file(filename);
  }
  else
  {
#ifndef NOPCAP
    extern void go_live_pcap(void);
    go_live_pcap();
#endif

  }

  //shm_stats_print();

  // TODO: close SHM

  return 0;
}

//----------------------------------------------------------
// inc protocol counters
//----------------------------------------------------------
void inc_protocol_counters(uint8_t ipproto, uint16_t rport, PktDir dir, int len)
{
  proto_info_t *entry = shm_get_protocol(ipproto, rport);
  if (entry == NULL) return;

  entry->packets++;
  entry->bytes += len;

  if (dir == RIGHT_TO_LEFT) {
    entry->_packets++;
    entry->_bytes += len;
  }
}

//----------------------------------------------------------
// return 1 if port is a standard web port, 0 otherwise
//----------------------------------------------------------
int is_web(uint16_t port)
{
  switch(port)
  {
    case 80:
    case 443:
    case 8080:
      return 1;
    default:
      return 0;
  }
}

//----------------------------------------------------------
// classify stream flow-flags
//----------------------------------------------------------
void set_stream_flow_flags(stream_info_t *ss, addr_info_t *shm_left, addr_info_t *shm_right)
{
   if (shm_left->internal && shm_right->internal)
   {
     // dont want IN/OUT flag set here

     ss->flowflags = FLOW_TYPE_INTERNAL;
     return;
   }

   if (is_web(ss->rport))
     ss->flowflags |= FLOW_FLAG_WEB;

   if (shm_right->internal)
     ss->flowflags |= FLOW_FLAG_IN;
}
