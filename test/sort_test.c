#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <utils/sort.h>

typedef struct {
  uint32_t bytes;
  uint32_t packets;
} my_t;

my_t arr[] = 
{
  {555, 5},
  {111, 1},
  {888, 8},
  {777, 7},
  {222, 2},
  {333, 3},
  {999, 9},
  {444, 4},
  {666, 6},
};

void *ptrs[] = {
  &arr[0],
  &arr[1],
  &arr[2],
  &arr[3],
  &arr[4],
  &arr[5],
  &arr[6],
  &arr[7],
  &arr[8],
};

int my_cmp(void *_a, void *_b)
{
  my_t *a = (my_t *)_a;
  my_t *b = (my_t *)_b;

  if (a->bytes < b->bytes) return -1;
  if (a->bytes > b->bytes) return 1;
  
  return 0; // if (a->bytes == b->bytes)
}

int my_rcmp(void *_a, void *_b)
{
  return my_cmp(_b, _a);
}

int main(int argc, char *argv[])
{
  int num = sizeof(arr) / sizeof(my_t);

  quicksort(ptrs, 0, num-1, my_rcmp);		// reverse sort

  assert(((my_t*)ptrs[0])->bytes == 999);
  assert(((my_t*)ptrs[8])->bytes == 111);

  assert(((my_t*)ptrs[3])->bytes == 666);

  assert(((my_t*)ptrs[0])->packets == 9);
  assert(((my_t*)ptrs[8])->packets == 1);

/*
  for (int i=0;i<num;i++)
  {
    my_t *item = (my_t*)ptrs[i];
    printf("[%d] bytes=%u pkts=%u\n", i, item->bytes, item->packets);
  }
*/

  printf("sort tests DONE\n");
}
