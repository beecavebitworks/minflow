#ifndef _CLEANUP_H_
#define _CLEANUP_H_

void add_stream_to_cleanup_list(uint32_t tv_sec, stream_info_t *ss);
void cleanup_expired_streams(uint32_t tv_sec);
void cleanup_init();

#endif 
