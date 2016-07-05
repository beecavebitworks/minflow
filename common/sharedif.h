#ifndef _SHAREDIF_H_
#define _SHAREDIF_H_

#include <common/structs.h>

#define SI_NUM_ADDRS		(16*1024)
#define SI_NUM_STREAMS		(32*1024)
#define SI_ADDR6_BYTES		(16*32)
#define SI_NUM_FLOWTYPES	(6)
#define SI_NUM_LANS		(4)
#define SI_NUM_TCP_PROTOS	(9110)
#define SI_NUM_PROTOS		(SI_NUM_TCP_PROTOS*2)

#define SHMID_MINFLOWCAP	0x05430A1E
#define SHAREDIF_MAGIC 		0x466c6f77

typedef struct sharedif_t
{
   uint32_t                   magic;
   uint16_t                   lan_struct_len;		// struct lens for versioning
   uint16_t                   addr_struct_len;
   uint16_t                   stream_struct_len;
   uint16_t                   proto_struct_len;

   tval_t                     tstart;			// epoch seconds
   tval_t                     tnow;

   // == sec_stats_t {

   flow_stats_t               flow_stats[SI_NUM_FLOWTYPES];
   uint32_t                   ifdrops;			// interface drops
   uint32_t                   kdrops;			// kernel drops

   mga_info_t                 addr_lan_indexes;		// location of internal addr_info
   mga_info_t                 addr_wan_indexes;		// location of external addr_info
   mga_info_t                 stream_indexes;

   // == sec_stats_t }

   stream_info_t              streams[SI_NUM_STREAMS];
   proto_info_t               protos[SI_NUM_PROTOS];
   addr_info_t                addrs[SI_NUM_ADDRS];
   lan_info_t                 lans[SI_NUM_LANS];
   uint8_t                    addr6mem[SI_ADDR6_BYTES];

}
sharedif_t;


#endif // _SHARED_IF_
