#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include <stdint.h> // uint32_t
#include <common/tval.h>
#include <common/mgainfo.h>

// ================
// addr_info_t : stats on a device address.  Check mac addrs on new streams, as DHCP may change
// ================

typedef struct addr_info_t
{
   tval_t      tstart;
   uint32_t    iid;                     // Instance ID
   uint32_t    addr_offset;		// ipv4 address, or if ipv6 an offset into address memory of actual address
   uint8_t     isv6;			// isIPv4 or v6
   uint8_t     internal;		// internal address == 1

   uint16_t    resvd;			// reserved: init to zero
   uint16_t    resvd2;			// reserved: init to zero

   uint16_t    nstreams;
   uint16_t    nstreams_active;

   uint32_t    txpackets;               // left-to-right packets and bytes
   uint32_t    txbytes;
   uint32_t    rxpackets;               // right-to-left packets and bytes
   uint32_t    rxbytes;

   uint16_t    txbcastpkt;			// num broadcasted packets
   uint8_t     mac[6];			// can convert IPV6 mac from 8 to 6
}
addr_info_t;

// ================
// stream_info_t : stats on a stream
// ================

typedef struct stream_info_t
{
   tval_t      tstart;			// timestamp of first packet
   tval_t      tlast;			// timestamp of most recent packet
   uint32_t    iid;                     // Instance ID
   uint16_t    idx_left;	        // index of key left addr_info_t
   uint16_t    idx_right;	        // index of key right addr_info_t
   uint16_t    idx_proto;	        // index of proto_info_t
   uint16_t    lport;
   uint16_t    rport;
   uint8_t     ipproto;
   uint8_t     tcpflags;		// collection of TCP flags for stream
   uint8_t     vlan;
   uint8_t     flowflags;               // (1<<0)=WEB protos, IN,OUT,INWEB,OUTWEB ?
   uint32_t    packets;                 // total packets and bytes
   uint32_t    bytes;
   uint32_t    _packets;                // right-to-left packets and bytes
   uint32_t    _bytes;
}
stream_info_t;


#define FLOW_FLAG_WEB            (1<<0)		// port 80,443,8080
#define FLOW_FLAG_IN             (1<<1)		// if server is internal (connection from outside)

#define FLOW_FLAG_CLEANUP        (1<<7)		// been marked for cleanup

#define FLOW_TYPE_NONWEB_OUT     (0)                             // 0x00
#define FLOW_TYPE_WEB_OUT        (FLOW_FLAG_WEB)                 // 0x01
#define FLOW_TYPE_NONWEB_IN      (FLOW_FLAG_IN)		         // 0x02
#define FLOW_TYPE_WEB_IN         (FLOW_FLAG_WEB | FLOW_FLAG_IN)  // 0x03
#define FLOW_TYPE_INTERNAL       (0x04)                          // 0x04
#define FLOW_TYPE_TOTAL          (0x05)                          // 0x05

#define MASK_ANY (uint32_t)0xFFFFFFFF

// ================
// lan_info_t : stats on a vlan,subnet, or overall
// ================

typedef struct lan_info_t
{
   uint32_t    mask4;			// IPv4 mask to match addresses
   int8_t      vlan;			// -1=overall, 0 = none
   uint8_t     flowflags;
   uint16_t    rsvd;                      // 

  uint32_t  packets;			// total packets and bytes
  uint32_t  bytes;
  uint32_t  _packets;			// right-to-left packets and bytes
  uint32_t  _bytes;
}
lan_info_t;

typedef struct
{
  uint32_t  packets;
  uint32_t  bytes;
}
flow_stats_t;

#define PROTO_INFO_MAX_PORTS		4

// ================
// proto_info_t : stats on an application protocol, such as web, samba, ftp, etc
// ================

typedef struct proto_info_t
{
  ///uint32_t  port;
  uint32_t  packets;			// total packets and bytes
  uint32_t  bytes;
  uint32_t  _packets;			// right-to-left packets and bytes
  uint32_t  _bytes;
}
proto_info_t;

#define HK_NUM_STREAMS		16
#define HK_NUM_PROTOS		16

// ================
// housekeeping_t : struct used to communicate expired items
// ================

typedef struct housekeeping_t
{
   uint32_t tv_sec;			// if 0, capture process has handled them
   uint32_t streams[HK_NUM_STREAMS];	// streams to cleanup
   uint32_t protos[HK_NUM_PROTOS];	// ports to cleanup
}
housekeeping_t;

/*
 * write stream to string.  References master address data
 */
char* stream_info_to_s2(char *dest, stream_info_t *stream);

/*
 * write stream to string, with explicit address references
 */
char* stream_info_to_s(char *dest, stream_info_t *stream, addr_info_t *left, addr_info_t *right);

/*
 * write address to string
 */
char * addr_info_to_s(char *dest, addr_info_t *addr);


#endif // _STRUCTS_H_
