#ifndef _MGARRAY_H_
#define _MGARRAY_H_

typedef enum {MGA_OP_TST=0, MGA_OP_SET, MGA_OP_TSTSET, MGA_OP_CLR} mgaop_t;

typedef enum {ENTRY_AVAIL=0, ENTRY_TAKEN=1} entry_status_t;

#include <common/mgainfo.h>

/**
 * user defined function.
 * @param int op  Defines behavior of call.
 *   MGA_OP_TST : function will return current state (ENTRY_AVAIL or ENTRY_TAKEN)
 *   MGA_OP_SET will set entry to unavailable and return previous state.
 *   MGA_OP_TSTSET if entry is free, set to unavailable and return previous state (ENTRY_AVAIL if success).
 *   MGA_OP_CLR will set entry to available and return previous state
 */
typedef entry_status_t (*mga_isempty_func)(char *, mgaop_t);


typedef struct mgarray_t
{
  mga_isempty_func isempty_func;
  char *           ptr;
  uint32_t         entry_size;
  uint32_t         total_entries;
  uint32_t         idx_min;
  uint32_t         idx_max;
  mga_info_t *     p;			// separating runtime lo,hi indexes for shared mem usage
}
mgarray_t;

/**
 * initialize pre-allocated mga struct.  The raw_array_ptr is also pre-allocated.
 */

void   mga_init(mgarray_t *mga, char *raw_array_ptr, mga_info_t *pinfo, int entry_size, mga_isempty_func func, uint32_t index_min, uint32_t index_max);


/**
 * find and return a new entry in array.
 * returns NULL if none are available.  Otherwise, will return pointer to entry and *idx will contain index of item on return.
 */
void * mga_alloc_entry(mgarray_t *ma, uint32_t *idx);

/**
 * Get the entry at index.
 * returns NULL if idx invalid.
 * TODO: checks for item allocated and returns NULL?
 */
void * mga_get(mgarray_t *ma, uint32_t idx);

/**
 * Release entry.
 */
void mga_free_entry(mgarray_t *ma, uint32_t idx);

/**
 *
 */
int mga_capacity(mgarray_t *ma);

uint32_t mga_lo(mgarray_t *ma);

uint32_t mga_hi(mgarray_t *ma);

int mga_count(mgarray_t *ma);

char *mga_to_s(char *pbuf, mgarray_t *ma);

#endif // _MGARRAY_H_
