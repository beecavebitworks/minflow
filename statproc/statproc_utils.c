#include <statproc/statproc_utils.h>

inline uint32_t u32diff(uint32_t prev, uint32_t cur)
{
  if (cur >= prev) return cur - prev;

  int64_t diff = 0x0FFFFFFFFL;
  diff += cur;
  diff -= prev;

  return diff;
}

