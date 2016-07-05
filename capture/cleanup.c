#include <common/shmdata.h>
#include <common/sharedif.h>
#include <stdlib.h> // calloc
#include <stdio.h>
#include <string.h> // memset
#include <capture/cleanup.h>

#define STAT_PROCESS_TIME_SEC 15
#define DBG 0

// seconds_inside_minute = (ts.tv_sec % 60) 
// if (seconds_inside_minute < STAT_PROCESS_TIME_SEC)
//   put_on_wait_queue(item)
// else
//   put_in_main_list()
//
// if (seconds_inside_minute >= STAT_PROCESS_TIME_SEC && wait_queue_size > 0)
//   move_wait_queue_to_main_list()
//
// TCP FIN/RST - can cleanup in few seconds if need space, but want statproc chance to see it
// UDP - periodically look through list to find UDP streams with no activity in 30 seconds, clean those up
//
// Add TCP_FIN/RST to head of list, timeouts to end of list, that way we can cleanup aggressively if needed

typedef struct {
  void ** entries;
  uint32_t i;
  uint32_t num_entries;
}
alist_t;

static alist_t cleanup_streams_list;
static alist_t cleanup_streams_wait;

void alist_alloc(alist_t *l, int num_entries)
{
  l->entries = (void **)calloc(num_entries * sizeof(void *),1);
  l->i = 0;
  l->num_entries = num_entries;
}

void cleanup_init()
{
  alist_alloc(&cleanup_streams_list, SI_NUM_STREAMS / 10);
  alist_alloc(&cleanup_streams_wait, SI_NUM_STREAMS / 20);
}

#define LIST_ADD(l,val) do {                            \
    if (l.i >= l.num_entries) { /* no room */           \
      if (DBG) printf("ERROR: no room in cleanup list\n"); }       \
    else { l.entries[l.i++] = val; }                    \
  } while(0)

#define alist_is_empty(l) ((l).i == 0)

//----------------------------------------------------------
// add stream to cleanup list
//----------------------------------------------------------
void add_stream_to_cleanup_list(uint32_t tv_sec, stream_info_t *ss)
{
  uint32_t seconds_inside_minute = (tv_sec % 60);
  if (seconds_inside_minute < STAT_PROCESS_TIME_SEC || seconds_inside_minute >= 55)
  {
    if (DBG) {
      char pbuf[80];
      printf("#QUEUE_CLEANUP stream %s\n", stream_info_to_s(pbuf, ss, NULL, NULL));
    }

    LIST_ADD(cleanup_streams_list, ss);
  }
  else
  {
    LIST_ADD(cleanup_streams_wait, ss);
  }
}
void find_expired_streams(uint32_t tv_sec);

//----------------------------------------------------------
// time to cleanup streams
//----------------------------------------------------------
void cleanup_expired_streams(uint32_t tv_sec)
{
  uint32_t seconds_inside_minute = (tv_sec % 60);
  if (seconds_inside_minute < STAT_PROCESS_TIME_SEC || alist_is_empty(cleanup_streams_list)) return;

  find_expired_streams(tv_sec);

  if (DBG) printf("CLEANUP %d i=%d num=%d\n", tv_sec, cleanup_streams_list.i, cleanup_streams_list.num_entries);

  for (int i=0;i<cleanup_streams_list.i;i++)
  {
    stream_info_t *ss = (stream_info_t*)cleanup_streams_list.entries[i];

    if (DBG) {
      char pbuf[80];
      stream_info_to_s(pbuf, ss, NULL, NULL);
      printf("#FREE[%d] stream %s\n", i, pbuf);
    }

    // create a stream_key_t from stream for lookup
    stream_key_t key;
    stream_init_key_from_stream(&key, ss);

    // remove stream hash table entry

    shm_stream_lookup_free(&key);

    // decrement address stream counters

    addr_info_t *aleft = shm_addr_get(ss->idx_left);
    addr_info_t *aright = shm_addr_get(ss->idx_right);

    aleft->nstreams_active--;
    aright->nstreams_active--;

    // free addrs if no active streams

    if (aleft->nstreams_active == 0) shm_addr_free(ss->idx_left);
    if (aright->nstreams_active == 0) shm_addr_free(ss->idx_right);

    // mark slot as empty

    set_stream_slot_empty(ss);
  }

  // zero out cleanup list

  cleanup_streams_list.i = 0;

  // copy wait items to list
 
  for (int i=0; i<cleanup_streams_wait.i;i++)
  {
    stream_info_t *ss = (stream_info_t *)cleanup_streams_wait.entries[i];
    LIST_ADD(cleanup_streams_list, ss);
  }

  // zero out wait list

  cleanup_streams_wait.i = 0;
}

