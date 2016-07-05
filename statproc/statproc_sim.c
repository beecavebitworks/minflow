// functions for stats process

#include <common/sharedif.h>

extern void every_second(tval_t ts, sharedif_t *g_shm);

static tval_t last_sec = {0,0};

void sim_before_packet(tval_t ts, sharedif_t *g_shm)
{
  if (last_sec.tv_sec == 0) {
    last_sec = ts;
    return;
  }

  uint32_t ms = tval_diff_ms(&last_sec, &ts);

  if (ms >= 1000) {
    every_second(ts, g_shm);
    last_sec = ts;
  }
  
}

void sim_after_packet(tval_t ts, sharedif_t *g_shm)
{
  sim_before_packet(ts, g_shm);
}

