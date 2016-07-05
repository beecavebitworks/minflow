#ifndef _TVAL_H_
#define _TVAL_H_

#include <stdint.h>
#include <sys/types.h>

// ================
// tval_t : timestamp
// ================
typedef struct tval_t
{
   uint32_t tv_sec;
   uint32_t tv_usec;
} tval_t;

uint32_t tval_diff_ms(tval_t *prev, tval_t *cur);

// return 0 if equal, -1 if prev < cur, 1 if prev > cur
int tval_cmp(tval_t *prev, tval_t *cur);

#endif // _TVAL_H_
