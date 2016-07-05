#include <common/tval.h>
#include <string.h>

uint32_t tval_diff_ms(tval_t *prev, tval_t *cur)
{
  uint32_t diff = (cur->tv_sec - prev->tv_sec) * 1000;

  if (cur->tv_usec < prev->tv_usec)
    diff += (((int64_t)0x0FFFFFFFFL + cur->tv_usec) - prev->tv_usec) / 1000;
  else
    diff += (cur->tv_usec - prev->tv_usec) / 1000;

  return diff;
}

// return 0 if equal, -1 if prev < cur, 1 if prev > cur
int tval_cmp(tval_t *prev, tval_t *cur)
{
  return memcmp(prev,cur,sizeof(tval_t));
  //uint32_t sec = cur->tv_sec - prev->tv_sec;
  //uint32_t us = cur->tv_usec - prev->tv_usec;
}
