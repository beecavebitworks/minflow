#include <export/export.h>
#include <stdio.h>
#include <utils/stringbuf.h>

static char *sort_name(SortBy sortby);
extern char *addr_mac_to_s(char *dest, addr_info_t *addr);

static stream_info_t *_streams[1024];
static int _num_streams;

static addr_info_t *_addrs[1024];
static int _num_addrs;

static sbuf_t *sbuf_addrs=NULL;
static sbuf_t *sbuf_streams=NULL;
static sbuf_t *sbuf_stats=NULL;
static char linebuf[2048];
static int initialized=0;

//---------------------------------------------------------------
// on_stat_period_open
//---------------------------------------------------------------
void on_stat_period_open(uint32_t ts, int period_seconds)
{
  if (!initialized) {
    sbuf_addrs=sbuf_new(8192);
    sbuf_streams=sbuf_new(8192);
    sbuf_stats=sbuf_new(8192);
  }

  printf("===,START,%u,%d\n", ts, period_seconds);
  _num_streams=0;
  _num_addrs=0;
}


//---------------------------------------------------------------
// on_stat_period_close
//---------------------------------------------------------------
void on_stat_period_close(int period_seconds)
{
  printf("_ADDR_,_IID_,_Internal_,_Label_,_MAC_,_____\n");
  puts(sbuf_to_s(sbuf_addrs));

  printf("_STRM_,_IID_,_FlowFlags_,_IpProto_,_LAddrIID_,_LPort_,_RAddrIID_,_RPort_,_Label_,_____\n");
  puts(sbuf_to_s(sbuf_streams));

  puts(sbuf_to_s(sbuf_stats));
  printf("===,END,%d\n", period_seconds);
}

//---------------------------------------------------------------
// seen_addr
// if already seen address in this period, return 1.
// Otherwise, add addr to list and return 0.
//---------------------------------------------------------------
static int seen_addr(addr_info_t *addr)
{
  for (int i=0;i<_num_addrs;i++)
    if (_addrs[i]->iid == addr->iid) return 1;

  _addrs[_num_addrs++] = addr;
  return 0;
}

//---------------------------------------------------------------
// seen_stream
// if already seen stream in this period, return 1.
// Otherwise, add to list and return 0.
//---------------------------------------------------------------
static int seen_stream(stream_info_t *stream, addr_info_t *aleft, addr_info_t *aright)
{
  for (int i=0;i < _num_streams;i++)
    if (_streams[i]->iid == stream->iid) return 1;

  _streams[_num_streams++] = stream;
  return 0;
}

//---------------------------------------------------------------
// on_new_addr
//---------------------------------------------------------------
void on_new_addr(addr_info_t *addr)
{
  char pbuf[80];
  addr_info_to_s(pbuf, addr);

  char mbuf[60]="";
  if (addr->internal)
    addr_mac_to_s(mbuf,addr);	// external MAC is router, not server

  sprintf(linebuf, "ADDR,%u,%d,%s,%s,\n", addr->iid, addr->internal, pbuf, mbuf);
  sbuf_append(sbuf_addrs,linebuf);
}

//---------------------------------------------------------------
// on_new_stream
//---------------------------------------------------------------
void on_new_stream(stream_info_t *stream, addr_info_t *aleft, addr_info_t *aright)
{
  if (!seen_addr(aleft)) on_new_addr(aleft);
  if (!seen_addr(aright)) on_new_addr(aright);

  char pbuf[80];
  stream_info_to_s(pbuf, stream, aleft, aright);

  sprintf(linebuf,"STRM,%u,%d,%d,%u,%d,%u,%d,%s\n", stream->iid, stream->flowflags, 
    stream->ipproto, aleft->iid, stream->lport, aright->iid, stream->rport,
    pbuf);
  sbuf_append(sbuf_streams, linebuf);
}

//---------------------------------------------------------------
// called for every stream in TopN
//---------------------------------------------------------------
void on_topn_stream(uint32_t ts, SortBy sortby, uint32_t flowflags, int i, stream_info_t *stream, addr_info_t *aleft, addr_info_t *aright)
{
  static char TYPE[] = "TSTR";
  if (i == 0) {
    sprintf(linebuf,"_%s_,_SortBy_,_FlowFlags_,_Idx_,_Packets_,_Bytes_,_Stream_,_____\n",TYPE);
    sbuf_append(sbuf_stats, linebuf);
  }

  if (!seen_stream(stream,aleft,aright)) on_new_stream(stream,aleft,aright);

  sprintf(linebuf,"%s,%s,%d,%d,%u,%u,%u\n", TYPE,sort_name(sortby),flowflags,i, stream->packets, stream->bytes, stream->iid);
  sbuf_append(sbuf_stats, linebuf);
}

//---------------------------------------------------------------
// called for every protocol in TopN
//---------------------------------------------------------------
void on_topn_protocol(uint32_t ts, SortBy sortby, uint32_t flowflags, int i, uint16_t port, uint8_t ipproto, proto_info_t *proto)
{
  static char TYPE[] = "PORT";
  if (i == 0) {
    sprintf(linebuf,"_%s_,_SortBy_,_FlowFlags_,_Idx_,_Port_,_IpProto_,_Packets_,_Bytes_,_____\n",TYPE);
    sbuf_append(sbuf_stats, linebuf);
  }

  sprintf(linebuf, "%s,%s,%d,%d,%u,%u,%u,%u\n", TYPE,sort_name(sortby), flowflags, i, port, ipproto, proto->packets, proto->bytes);
  sbuf_append(sbuf_stats, linebuf);
}

//---------------------------------------------------------------
// called for every protocol in TopN
//---------------------------------------------------------------
void on_topn_address(uint32_t ts, SortBy sortby, uint32_t flowflags, int i, addr_info_t *addr)
{
  static char TYPE[] = "TADR";
  if (i == 0) {
    sprintf(linebuf,"_%s_,_SortBy_,_FlowFlags_,_Idx_,_Address_,_TxPackets_,_TxBytes_,_RxPackets_,_RxBytes_,____\n",TYPE);
    sbuf_append(sbuf_stats, linebuf);
  }
  
  if (!seen_addr(addr)) on_new_addr(addr);

  //char pbuf[80];
  //addr_info_to_s(pbuf, addr);

  sprintf(linebuf,"%s,%s,%d,%d,%u,%u,%u,%u,%u\n", TYPE, sort_name(sortby),flowflags, i, addr->iid, addr->txpackets, addr->txbytes, addr->rxpackets, addr->rxbytes);
  sbuf_append(sbuf_stats, linebuf);
}

static char *SORT_NAMES[]={
 "?",
 "P",
 "B",
 "TXP",
 "TB",
 "RXP",
 "RB",
 "#P>",
 "#P<"
};

//---------------------------------------------------------------
// return abbreviation for sort type
//---------------------------------------------------------------
static char *sort_name(SortBy sortby)
{
  if ((int)sortby < 0 || sortby >= SortByMax) return SORT_NAMES[0];
  return SORT_NAMES[sortby];
}

