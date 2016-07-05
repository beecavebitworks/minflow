#include<utils/sort.h>

#define SWAP(list, idx1, idx2) {         \
          void * temp = (list)[idx1];    \
          (list)[idx1] = (list)[idx2];   \
          (list)[idx2] = temp;           \
}
 
 
void quicksort(void* list[],int m,int n, cmp_func cmp)
{
    void* key;
    int i,j,k;

    if( m < n)
    {
        k = (m + n) / 2; //choose_pivot(m,n);
        SWAP(list, m, k); //swap(&list[m],&list[k]);
        key = list[m];
        i = m+1;
        j = n;
        while(i <= j)
        {
            while((i <= n) && (cmp(list[i], key) <= 0))
                i++;
            while((j >= m) && cmp(list[j], key) > 0)
                j--;
            if( i < j)
                SWAP(list, i, j); //swap(&list[i],&list[j]);
        }
        /* swap two elements */
        SWAP(list,m,j); //swap(&list[m],&list[j]);
 
        /* recursively sort the lesser list */
        quicksort(list,m,j-1, cmp);
        quicksort(list,j+1,n, cmp);
    }
}


