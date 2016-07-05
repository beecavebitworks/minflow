#include <stdio.h>
#include <stdlib.h>
#include <common/sharedif.h>
#include <utils/ipc.h>
#include <time.h>
#include <unistd.h>

extern void every_second(tval_t ts, sharedif_t *g_shm);

static sharedif_t *g_shm = NULL;

int main(int argc, char *argv[])
{
    g_shm = (sharedif_t*)shmem_open(SHMID_MINFLOWCAP, sizeof(sharedif_t));
    if (g_shm == NULL) {
      printf("ERROR: unable to get pointer to shared memory segment\n");
      return 0;
    }

  tval_t ts={0,0};

  for (int i=0;i<60*60;i++)	// stop after hour
  {
    if (ts.tv_sec == 0) { ts.tv_sec = time(NULL); continue;}

    if ((time(NULL) - ts.tv_sec) >= 1)
    {
      ts.tv_sec = time(NULL);
      every_second(ts, g_shm);
    }

    usleep(250000);
  }
}
