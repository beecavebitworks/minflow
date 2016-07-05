#ifndef _EXPORT_H_
#define _EXPORT_H_

#include <stdint.h>
#include <common/structs.h>

typedef enum {SortByNone=0,SortByPkt,SortByBytes,SortByTxPkt,SortByTxBytes,SortByRxPkt,SortByRxBytes,SortByScannedPorts,SortByServedPorts,SortByMax} SortBy;

void on_stat_period_open(uint32_t ts, int period_seconds);

void on_topn_stream(uint32_t ts, SortBy sortby, uint32_t flowflags, int i, stream_info_t *stream, addr_info_t *aleft, addr_info_t *aright);

void on_topn_address(uint32_t ts, SortBy sortby, uint32_t flowflags, int i, addr_info_t *addr);

void on_topn_protocol(uint32_t ts, SortBy sortby, uint32_t flowflags, int i, uint16_t port, uint8_t ipproto, proto_info_t *stream);

void on_stat_period_close(int period_seconds);

#endif // _EXPORT_H_
