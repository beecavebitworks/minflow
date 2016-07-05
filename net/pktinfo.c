#include <net/pktinfo.h>
#include <string.h> // memset
#include <net/iputils.h>
#include <stdio.h>

#include <netinet/in.h>

void pkt_get_ports_v4(pktinfo_t *pkt, uint8_t ipproto, uint16_t *sport, uint16_t *dport);

//-----------------------------------------------
// pkt_init
//-----------------------------------------------
void pkt_init(pktinfo_t *pktinfo)
{
  memset(pktinfo, 0, sizeof(*pktinfo));
}

//-----------------------------------------------
//	-1 for none, vlan value otherwise
//-----------------------------------------------
int16_t pkt_get_vlan(pktinfo_t *pktinfo)
{
  uint16_t ethertype = ntohs(pktinfo->ether->ether_type);

  if (ethertype == ETHERTYPE_VLAN)
  {
    char *buf = (char *)pktinfo->ether;
    return ntohs(*((uint16_t *)(buf + sizeof(ether_hdr_t) )));
  }
  else if (ethertype == ETHERTYPE_VLAN2)
  {
    char *buf = (char *)pktinfo->ether;
    return ntohs(*((uint16_t *)(buf + sizeof(ether_hdr_t) + 4 )));
  }

  return -1;

}

//-----------------------------------------------
// -1 for none, vlan2 value otherwise
//-----------------------------------------------
int16_t pkt_get_vlan2(pktinfo_t *pktinfo)
{
  uint16_t ethertype = ntohs(pktinfo->ether->ether_type);

  if (ethertype == ETHERTYPE_VLAN2)
  {
    char *buf = (char *)pktinfo->ether;
    return ntohs(*((uint16_t *)(buf + sizeof(ether_hdr_t) )));
  }

  return -1;
}

//-----------------------------------------------
// pkt_is_v6
//-----------------------------------------------
int pkt_is_v6(pktinfo_t *pktinfo)
{
  return pktinfo->isv6;
}

//-----------------------------------------------
// initialize stream_key_t from pktinfo_t
// return 1 if key is flipped from packet dir
//-----------------------------------------------
int pkt_init_key(pktinfo_t *pkt, stream_key_t *key)
{
  if (pkt_is_v6(pkt))
  {
    memset(key, 0, sizeof(*key));
    printf("ERROR: ipv6 not supported yet\n");
    return 0;
  }
  else
  {
    uint8_t ipproto = pkt->ip4->ip_p;
    uint16_t sport, dport;

    pkt_get_ports_v4(pkt, ipproto, &sport, &dport);

    return net_stream_init_v4(key, ipproto, pkt->ip4->ip_src, pkt->ip4->ip_dst, sport, dport);
  }
}

uint8_t pkt_tcp_flags(pktinfo_t *pkt)
{
  return pkt->tcpflags;
}

char *PROTO_NAME_TCP="TCP";
char *PROTO_NAME_UDP="UDP";
char *PROTO_NAME_UNKNOWN="?";

//-----------------------------------------------
// get_proto_name
//-----------------------------------------------
char *get_proto_name(uint8_t ipproto)
{
  switch(ipproto)
  {
    case IPPROTO_TCP: return PROTO_NAME_TCP;
    case IPPROTO_UDP: return PROTO_NAME_UDP;
    default: return PROTO_NAME_UNKNOWN;
  }
}

//-----------------------------------------------
// get layer 4 ports for ipv4
//-----------------------------------------------
void pkt_get_ports_v4(pktinfo_t *pkt, uint8_t ipproto, uint16_t *sport, uint16_t *dport)
{
  if (ipproto == IPPROTO_TCP)
  {
    *sport = ntohs(pkt->tcp->th_sport);
    *dport = ntohs(pkt->tcp->th_dport);

  } else {

    *sport = ntohs(pkt->udp->uh_sport);
    *dport = ntohs(pkt->udp->uh_dport);
  }
}

//-----------------------------------------------
// to_s
//-----------------------------------------------
char* pkt_to_s(char *dest, pktinfo_t *pkt)
{
  uint16_t sport, dport;

  if (pkt_is_v6(pkt))
  {
    sprintf(dest, "IPV6 STREAM HERE");
    //sprintf(dest, "%s_%s_%d_%s_%d", get_proto_name(pkt->ip6.ip_p),"?:?:?",sport, "?:?:?", dport);
    return dest;
  }
  else
  {
    uint8_t ipproto = pkt->ip4->ip_p;

    pkt_get_ports_v4(pkt, ipproto, &sport, &dport);

    char srcip[16], dstip[16];
    ip2string(srcip, pkt->ip4->ip_src);
    ip2string(dstip, pkt->ip4->ip_dst);

    sprintf(dest, "%s_%s_%d_%s_%d", get_proto_name(ipproto), srcip, sport, dstip, dport);
    return dest;
  }

}

//-----------------------------------------------
// pkt_src_mac
//-----------------------------------------------
char *pkt_src_mac(pktinfo_t *pktinfo)
{
  return (char *)&pktinfo->ether->ether_shost[0];
}

//-----------------------------------------------
// pkt_dst_mac
//-----------------------------------------------
char *pkt_dst_mac(pktinfo_t *pktinfo)
{
  return (char *)&pktinfo->ether->ether_dhost[0];
}
