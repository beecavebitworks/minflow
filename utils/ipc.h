#include <stdint.h>

/*********************************************************************
        Shared Memory Functions
*********************************************************************/

/*
        shmem_create()

        @param  key                             A key that will uniquely identify the shared memory
        @param  mem_size_bytes  The size of the memory to be allocated

        Create a new block of shared memory identified by key.

        Returns -1 on failure or if the block already exists, otherwise returns
        a valid segment identifier suitable for passing to destroy_shared_mem.
*/
int shmem_create( uint32_t key, size_t mem_size_bytes );

/*
        shmem_de()

        @param  h_shmem         A handle to the shared memory object

        Release the shared memory block back to the system.  Returns
        -1 on failure, 0 on success.
*/
int shmem_destroy( int h_shmem );

/*
        shmem_open()

        @param  key                     The identifier for an existing allocation of shared memory
        @param  mem_size        The size of the allocated memory, in bytes

        Get a handle to shared memory that was previously allocated and attach
        the shared memory to the current process space.  Returns non-zero on
        failure.
*/
void* shmem_open( uint32_t key, size_t mem_size_bytes );

/*
        shmem_close()

        Detach the shared memory segment from the current process.  Returns
        non-zero on failure.
*/
int shmem_close( void* shmem_addr );

