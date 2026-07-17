/* src/backends/msmpi.c */
#include "unimpi_vtable.h"
#include "unimpi_platform.h"
#include "unimpi.h"
#include <string.h>

/* MS-MPI uses different predefined communicator values than MPICH */
/* MPI_COMM_WORLD = 0x44000000, MPI_COMM_SELF = 0x44000001 */

/* Forward declaration for MPICH init - we need to implement our own version */
static int unimpi_vtable_init_msmpi_internal(unimpi_lib_handle_t handle);

int unimpi_vtable_init_msmpi(unimpi_lib_handle_t handle) {
    /* Call internal initialization that doesn't override communicators */
    return unimpi_vtable_init_msmpi_internal(handle);
}

/* Copy of MPICH init but with MS-MPI communicator values */
static int unimpi_vtable_init_msmpi_internal(unimpi_lib_handle_t handle) {
    /* Environment Management */
    unimpi.init = (int (*)(int*, char***))
        unimpi_platform_dlsym(handle, "MPI_Init");
    unimpi.finalize = (int (*)(void))
        unimpi_platform_dlsym(handle, "MPI_Finalize");
    unimpi.initialized = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Initialized");
    unimpi.finalized = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Finalized");
    unimpi.abort = (int (*)(MPI_Comm, int))
        unimpi_platform_dlsym(handle, "MPI_Abort");
    unimpi.get_processor_name = (int (*)(char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Get_processor_name");
    unimpi.get_version = (int (*)(int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Get_version");
    unimpi.barrier = (int (*)(MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Barrier");
    unimpi.wtime = (double (*)(void))
        unimpi_platform_dlsym(handle, "MPI_Wtime");
    unimpi.wtick = (double (*)(void))
        unimpi_platform_dlsym(handle, "MPI_Wtick");

    /* Point-to-Point - Standard */
    unimpi.send = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Send");
    unimpi.recv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, UNIMPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Recv");
    unimpi.isend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Isend");
    unimpi.irecv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Irecv");
    unimpi.wait = (int (*)(MPI_Request*, UNIMPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Wait");
    unimpi.waitall = (int (*)(int, MPI_Request*, UNIMPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitall");

    /* Communicator */
    unimpi.comm_size = (int (*)(MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_size");
    unimpi.comm_rank = (int (*)(MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_rank");
    unimpi.comm_dup = (int (*)(MPI_Comm, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_dup");
    unimpi.comm_split = (int (*)(MPI_Comm, int, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_split");
    unimpi.comm_free = (int (*)(MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_free");

    /* Collective - Standard */
    unimpi.bcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Bcast");
    unimpi.reduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Reduce");
    unimpi.allreduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Allreduce");
    unimpi.gather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Gather");
    unimpi.allgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Allgather");
    unimpi.scatter = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Scatter");

    /* Set MS-MPI specific communicator values */
    UNIMPI_COMM_WORLD = (MPI_Comm)0x44000000;
    UNIMPI_COMM_SELF = (MPI_Comm)0x44000001;

    return UNIMPI_OK;
}
