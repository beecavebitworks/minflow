#include <net/ether.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>

#include <net/parse.h>
#include <net/pktinfo.h>

#define min(a,b) ((a) < (b) ? (a):(b))

//---------------------------------------------------------------------
// _ether_skip_vlan
// skip over VLAN and VLAN2 (service vlan) ethertypes in ether header
//---------------------------------------------------------------------
static uint16_t
_ether_skip_vlan(char *buf, ether_hdr_t *ep, int *offset)
{
  uint16_t ethertype = ntohs(ep->ether_type);
  *offset = 0;

  if (ethertype == ETHERTYPE_VLAN2)
  {
    *offset = 8;
    return ntohs(*((uint16_t *)(buf + sizeof(ether_hdr_t) + *offset - 2)));

  } else if (ethertype == ETHERTYPE_VLAN) {

    *offset = 4;
    return ntohs(*(uint16_t *)(buf + sizeof(ether_hdr_t) + *offset - 2));
  }

  return ethertype;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
int ip_get_hlen(ip_hdr_t *ip)
{
  return IP_HL(ip) * 4;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
int _udp4_parse(char *buf, pktinfo_t *pktinfo, int remaining)
{
  pktinfo->udp = (udp_hdr_t *)buf;
  return 0;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
int _tcp4_parse(char *buf, pktinfo_t *pktinfo, int remaining)
{
  pktinfo->tcp = (tcp_hdr_t *)buf;

  pktinfo->istcp = 1;
  pktinfo->tcpflags = pktinfo->tcp->th_flags;
  pktinfo->isfin = ((pktinfo->tcpflags & (TH_FIN | TH_RST)) != 0);

  return 0;
}

//---------------------------------------------------------------------
// _ip_parse
// parse ipv4 header
//---------------------------------------------------------------------
int _ip_parse(char *buf, pktinfo_t *pktinfo, int remaining)
{
   if (remaining < sizeof(ip_hdr_t)) return PARSE_ERR_INCOMPLETE;

   ip_hdr_t *hdr = (ip_hdr_t *)buf;
   pktinfo->ip4 = hdr;

   // get lengths

   int hlen = ip_get_hlen(hdr);

   int16_t len = ntohs(hdr->ip_len);

   // make sure we have at least header if header is longer than usual

   if (hlen > sizeof(ip_hdr_t) && remaining < hlen) return PARSE_ERR_INCOMPLETE;

   // test fragment offset

   uint16_t off = ntohs(hdr->ip_off);

   if ((off & 0x1fff) == 0)
   {
      // send downstream

      int status;
      switch(hdr->ip_p)
      {
        case IPPROTO_TCP:
         status = _tcp4_parse(buf+hlen, pktinfo, min(remaining,len) - hlen);
         return status;

        case IPPROTO_UDP:
         status = _udp4_parse(buf+hlen, pktinfo, min(remaining,len) - hlen);
         return status;

	default:
	break;
      }
   }
   // else // handle fragmentation?

   return PARSE_ERR_UNSUPPORTED;
}


//---------------------------------------------------------------------
// net_parse
// parse ether,ip,tcp,udp headers and initialize pktinfo
//---------------------------------------------------------------------
int net_parse(pcap_pkthdr32_t *pcaphdr, char *buf, pktinfo_t *pktinfo)
{
  if (pcaphdr->caplen < ETHER_HDRLEN) return PARSE_ERR_INCOMPLETE;

  pkt_init(pktinfo);

  ether_hdr_t *ep = pktinfo->ether = (ether_hdr_t *)buf;

  int offset;
  uint16_t ether_type = _ether_skip_vlan(buf, ep, &offset);

  if (ether_type <= ETHERMTU) return PARSE_ERR_UNSUPPORTED;

  switch (ether_type)
  {
    case ETHERTYPE_IP:
      return _ip_parse(buf + ETHER_HDRLEN + offset, pktinfo, pcaphdr->caplen - ETHER_HDRLEN - offset);

    //case ETHERTYPE_IPV6:
    //  return _ipv6_parse(buf + ETHER_HDRLEN + offset, pktinfo, pcaphdr->caplen - ETHER_HDRLEN - offset);

    default:
      break;
  }
  
  return PARSE_ERR_UNSUPPORTED;
}
