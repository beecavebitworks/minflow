#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset
#include <time.h>
#include <utils/mgarray.h>

#define DEBUG 1
#include <assert.h>

typedef struct myval_t
{
  uint32_t ts;
  uint32_t b;
  uint32_t num;
}
myval_t;

#define ARR_SIZE 21
#define MA_IDX_MIN 9
#define MA_IDX_MAX 15

myval_t *g_arr=NULL;

mga_info_t ma_indexes;
mgarray_t ma;

entry_status_t my_isempty_func(char *_entry, mgaop_t op);
void init_random_item(myval_t *val, uint32_t idx);

//-------------------------------------------------------------
// main
//-------------------------------------------------------------
int main(int argc, char *argv[])
{
  uint32_t idx;
  char pbuf[80];

  g_arr = calloc(ARR_SIZE, sizeof(myval_t));

  mga_init(&ma, (char*)g_arr, &ma_indexes, sizeof(myval_t), my_isempty_func, MA_IDX_MIN, MA_IDX_MAX);

  uint32_t mid = MA_IDX_MIN + (MA_IDX_MAX - MA_IDX_MIN)/2;

  assert(mga_capacity(&ma) == (MA_IDX_MAX - MA_IDX_MIN + 1));
  assert(mga_lo(&ma) == (mid));
  assert(mga_hi(&ma) == (mid+1));

  int capacity = mga_capacity(&ma);
  uint32_t lo_start = mga_lo(&ma);
  uint32_t hi_start = mga_hi(&ma);

  // fill up

  for (int i=0;i<capacity;i++)
  {
    myval_t *val;
   
    val = (myval_t*)mga_alloc_entry(&ma, &idx);

    assert(val != NULL);
    if (i == 0) assert(idx == lo_start);
    if (i == 1) assert(idx == hi_start);

    init_random_item(val,idx);

    assert(mga_count(&ma) == (i + 1));
  }

  assert(mga_lo(&ma) == (MA_IDX_MIN - 1));
  assert(mga_hi(&ma) == (MA_IDX_MAX + 1));

  // add should fail

  assert(mga_alloc_entry(&ma, &idx) == NULL);

  // delete internal entries

  mga_free_entry(&ma, mid);
  mga_free_entry(&ma, mid + 1);
  mga_free_entry(&ma, mid - 1);

  assert(mga_count(&ma) == (capacity - 3));

  // add entries to internal slots

  assert(mga_alloc_entry(&ma, &idx) != NULL);
  assert(mga_alloc_entry(&ma, &idx) != NULL);
  assert(mga_alloc_entry(&ma, &idx) != NULL);

  assert(mga_count(&ma) == mga_capacity(&ma));

  // delete from mid down to lo and make sure idx_lo comes back up to mid afterwards

  for (int i=mid; i >= MA_IDX_MIN; i--) mga_free_entry(&ma, i);

  assert(mga_lo(&ma) == mid);

  // delete from mid up to hi and make sure idx_lo comes back up to mid afterwards

  for (int i=mid; i <= MA_IDX_MAX; i++) mga_free_entry(&ma, i);

  assert(mga_lo(&ma) == mid);
  assert(mga_hi(&ma) == (mid+1));

  // delete entire array, even those outside the range

  for (int i=0; i < ARR_SIZE; i++) mga_free_entry(&ma, i);

  assert(mga_count(&ma) == 0);

  assert(mga_lo(&ma) == (mid));
  assert(mga_hi(&ma) == (mid+1 ));

  printf("mgarray tests OK\n");
}

//-------------------------------------------------------------
// isempty func
//-------------------------------------------------------------
entry_status_t
my_isempty_func(char *_entry, mgaop_t op)
{
  myval_t *entry = (myval_t *)_entry;
  entry_status_t status = (entry->ts == 0 ? ENTRY_AVAIL : ENTRY_TAKEN);

  switch(op)
  {
    case MGA_OP_SET:
     entry->ts = time(NULL); break;

    case MGA_OP_TSTSET:
     if (status == ENTRY_AVAIL) {
       entry->ts = time(NULL);
     }
     break;

    case MGA_OP_CLR:
     entry->ts = 0; break;

    case MGA_OP_TST:
    default:
     break;
  }

  return status;
}

//-------------------------------------------------------------
// initialize a new item
//-------------------------------------------------------------
void init_random_item(myval_t *val, uint32_t idx)
{
  uint32_t now = time(NULL);

  val->ts = now;
  val->num = idx;
  val->b = 0xBABE;
}

