#ifndef _SORT_H_
#define _SORT_H_

typedef int (*cmp_func)(void *a, void *b);
void quicksort(void* list[],int m,int n, cmp_func cmp);

#endif // _SORT_H_
