#ifndef _MGA_INFO_H_
#define _MGA_INFO_H_

#include <stdint.h>

typedef struct mga_info_t
{
  uint32_t num_entries;
  uint32_t idx_lo;
  uint32_t idx_hi;
}
mga_info_t;

#endif // _MGA_INFO_H_
