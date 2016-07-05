#ifndef _STREAM_STATE_H_
#define _STREAM_STATE_H_

#include <net/streamkey.h>

typedef struct stream_state_t
{
	net_stream_key _key;
	uint8_t is_swapped;  // key direction

	addr_info_t *shm_src;
	addr_info_t *shm_dst;

}
stream_state_t;

#endif // _STREAM_STATE_H_
