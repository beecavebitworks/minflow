#define _SVID_SOURCE
#include <sys/ipc.h>
#include <sys/shm.h>

#define PERM_RO 0644

int shmem_create( key_t key, size_t mem_size_bytes )
{
        return shmget( key, mem_size_bytes, IPC_CREAT | PERM_RO );		// read only to others
}

int shmem_destroy( int h_shmem )
{
        return shmctl( h_shmem, IPC_RMID, 0L );
}

void* shmem_open( key_t key, size_t mem_size_bytes )
{       
        void* rv;
        int h;

        h = shmget( key, mem_size_bytes, PERM_RO );
        if ( h == -1 )
                return 0L;
        
        rv = shmat( h, 0L, 0 );
        if ( rv == (void*)-1 )
                return 0L;

        return rv;
}

int shmem_close( void* shmem_addr )
{
        return shmdt( shmem_addr );
}


