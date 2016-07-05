#ifndef _SHM_DATA_H_
#define _SHM_DATA_H_

/*
 * Convenience layer to abstract the sharedif interface
 */

#include <stdint.h>
#include <net/streamkey.h>
#include <common/structs.h>

typedef void * HSHMDATA;

/**
 * shm_data_init
 * returns 0L on error, non-zero handle on success
 */
HSHMDATA shm_data_init(int use_shm);

stream_info_t *shm_stream_alloc();

stream_info_t *shm_stream_lookup(stream_key_t *key);

void shm_stream_lookup_free(stream_key_t *key);

addr_info_t *shm_addr_alloc_v4(uint32_t addr, char *mac_addr, uint32_t *idx);

addr_info_t *shm_addr_lookup_v4(uint32_t addr, uint32_t *idx);

addr_info_t *shm_addr_get(uint32_t index);

void shm_addr_free(uint32_t idx);

proto_info_t *shm_get_protocol(uint8_t ipproto, uint16_t rport);

void shm_stats_print();

void shm_inc_global(tval_t ts, int pktlen);

void shm_inc_flow_stats(stream_info_t *ss, int len);

void stream_init_key_from_stream(stream_key_t *key, stream_info_t *stream);

/*
 * Return 1 if slot is free, or 0 if taken
 */
int  is_stream_slot_empty(stream_info_t *entry);

/*
 * free an entry in stream list
 */
void set_stream_slot_empty(stream_info_t *entry);

char *addr_mac_to_s(char *dest, addr_info_t *addr);

#endif // _SHM_DATA_H_
