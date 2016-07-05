#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "mgarray.h"

#define DBG 0

#ifndef NULL
#define NULL 0L
#endif

#define ENTRY_PTR(ma, index) ((ma)->ptr + (index)*((ma)->entry_size))
#define ENTRY_IS_FREE(ma, index) ((ma)->isempty_func(ENTRY_PTR(ma,index), MGA_OP_TST)==ENTRY_AVAIL)
#define ENTRY_IS_TAKEN(ma, index) ((ma)->isempty_func(ENTRY_PTR(ma,index), MGA_OP_TST)==ENTRY_TAKEN)

//---------------------------------------------------------------------
// return true if lo index is farther from mid-point than hi index
//---------------------------------------------------------------------
int _mga_more_lo_than_hi(mgarray_t *ma)
{
  uint32_t mid = ma->idx_min + ma->total_entries / 2;
  return ((mid - ma->p->idx_lo) > (ma->p->idx_hi - 1 - mid));
}

/**---------------------------------------------------------------------
 * initialize pre-allocated mga struct.  The raw_array_ptr is also pre-allocated.
 *---------------------------------------------------------------------*/
void   mga_init(mgarray_t *mga, char *raw_array_ptr, mga_info_t *pinfo, int entry_size,  mga_isempty_func func, uint32_t index_min, uint32_t index_max)
{
  if (raw_array_ptr == NULL) { fprintf(stderr, "ERROR: raw_array_ptr needs to be allocated by caller\n"); return; }

  mga->ptr = raw_array_ptr;
  mga->entry_size = entry_size;
  mga->idx_max = index_max;
  mga->idx_min = index_min;
  mga->total_entries = index_max - index_min + 1;
  mga->isempty_func = func;
  mga->p = pinfo;
  mga->p->num_entries = 0;
  mga->p->idx_lo = mga->idx_min + mga->total_entries / 2;
  mga->p->idx_hi = mga->p->idx_lo + 1;
}

/**---------------------------------------------------------------------
 * find and return a entry at idx
 * returns NULL if idx invalid
 *---------------------------------------------------------------------*/
void * mga_get(mgarray_t *ma, uint32_t idx)
{
  if (idx < ma->idx_min || idx > ma->idx_max) { assert(0); return NULL;}

  return ENTRY_PTR(ma, idx);
}

/**---------------------------------------------------------------------
 * find and return a new entry in array.
 * returns NULL if none are available.  Otherwise, will return pointer to entry and *idx will contain index of item on return.
 *---------------------------------------------------------------------*/
void * mga_alloc_entry(mgarray_t *ma, uint32_t *idx)
{
  entry_status_t prev;
  if (ma->p->num_entries >= ma->total_entries) return NULL;

  int diff = (ma->p->idx_hi - ma->p->idx_lo - 1);
  assert(diff >= 0);

  if (diff > ma->p->num_entries)
  {
    // find an interior item
    for (uint32_t i=ma->p->idx_lo+1; i <ma->p->idx_hi; i++)
    {
      if (ma->isempty_func(ENTRY_PTR(ma,i), MGA_OP_TSTSET) == ENTRY_AVAIL) {
        *idx = i;
        ma->p->num_entries++;
        return ENTRY_PTR(ma,i);
      }
    }
    assert(0); // should never get here
  }

  if (_mga_more_lo_than_hi(ma))
  {
    // add to hi

    prev = ma->isempty_func(ENTRY_PTR(ma, ma->p->idx_hi), MGA_OP_TSTSET);
    assert(prev == ENTRY_AVAIL);

    *idx = ma->p->idx_hi;
    ma->p->num_entries++;
    ma->p->idx_hi++;

    assert(ENTRY_IS_TAKEN(ma, *idx));

    return ENTRY_PTR(ma,*idx);
  }
  else
  {
    // add to lo

    prev = ma->isempty_func(ENTRY_PTR(ma, ma->p->idx_lo), MGA_OP_TSTSET);
    assert(prev == ENTRY_AVAIL);

    *idx = ma->p->idx_lo--;
    ma->p->num_entries++;

    assert(ENTRY_IS_TAKEN(ma, *idx));

    return ENTRY_PTR(ma,*idx);
  }
}

/**---------------------------------------------------------------------
 * Release entry.
 *---------------------------------------------------------------------*/
void mga_free_entry(mgarray_t *ma, uint32_t idx)
{
  //printf("mga_free %d status=%d\n", idx, ma->isempty_func(ENTRY_PTR(ma, idx), MGA_OP_TST));

  if (idx <= ma->p->idx_lo || idx >= ma->p->idx_hi) {
    if (DBG) fprintf(stderr, "ERROR: mga_free_entry (idx out of range) %u \n", idx);
    return; }

  if (ma->isempty_func(ENTRY_PTR(ma, idx), MGA_OP_TST) == ENTRY_AVAIL) { return; }   // already freed?
  
  ma->p->num_entries--;
  ma->isempty_func(ENTRY_PTR(ma, idx), MGA_OP_CLR);

  uint32_t mid = ma->idx_min + ma->total_entries / 2;

  while (ma->p->idx_lo < mid && ENTRY_IS_FREE(ma,ma->p->idx_lo + 1)) ma->p->idx_lo++;
  while (ma->p->idx_hi > (ma->p->idx_lo + 1) && ENTRY_IS_FREE(ma,ma->p->idx_hi - 1)) ma->p->idx_hi--;
}

int mga_capacity(mgarray_t *ma) { return ma->total_entries; }

uint32_t mga_lo(mgarray_t *ma) { return ma->p->idx_lo; }

uint32_t mga_hi(mgarray_t *ma) { return ma->p->idx_hi; }

int mga_count(mgarray_t *ma) { return ma->p->num_entries; }

/**---------------------------------------------------------------------
 * sprint mgarray details to pbuf. returns pbuf
 *---------------------------------------------------------------------*/
char *mga_to_s(char *pbuf, mgarray_t *ma)
{
  sprintf(pbuf, "[%d / %d]  %d <-> %d", ma->p->num_entries, ma->total_entries, ma->p->idx_lo, ma->p->idx_hi);
  return pbuf;
}

