/* src/backends/openmpi.c */
#include "tftk_mpi_vtable.h"
#include "tftk_mpi_platform.h"
#include "tftk_mpi.h"

/* OpenMPI uses pointers for communicators, so we need special handling */
typedef struct ompi_communicator_t* ompi_comm_t;
typedef struct ompi_datatype_t* ompi_datatype_t;
typedef struct ompi_op_t* ompi_op_t;
typedef struct ompi_request_t* ompi_request_t;

/* Get actual MPI_Comm values from OpenMPI globals */
static int get_openmpi_comm_world(tftk_mpi_lib_handle_t handle, MPI_Comm *comm) {
    void **ptr = (void**)tftk_mpi_platform_dlsym(handle, "ompi_mpi_comm_world");
    if (ptr) {
        *comm = (MPI_Comm)(size_t)ptr;  /* Store the pointer value */
        return TFTK_MPI_OK;
    }
    /* Fallback: try to get from MPI_Comm_f2c(0) */
    return TFTK_MPI_ERR_SYMBOL_NOT_FOUND;
}

int tftk_mpi_vtable_init_openmpi(tftk_mpi_lib_handle_t handle) {
    /* Load environment functions */
    tftk_mpi.init = (int (*)(int*, char***))
        tftk_mpi_platform_dlsym(handle, "MPI_Init");
    tftk_mpi.finalize = (int (*)(void))
        tftk_mpi_platform_dlsym(handle, "MPI_Finalize");
    tftk_mpi.initialized = (int (*)(int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Initialized");
    tftk_mpi.finalized = (int (*)(int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Finalized");
    tftk_mpi.abort = (int (*)(MPI_Comm, int))
        tftk_mpi_platform_dlsym(handle, "MPI_Abort");
    tftk_mpi.get_processor_name = (int (*)(char*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Get_processor_name");
    tftk_mpi.get_version = (int (*)(int*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Get_version");
    tftk_mpi.barrier = (int (*)(MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Barrier");
    tftk_mpi.wtime = (double (*)(void))
        tftk_mpi_platform_dlsym(handle, "MPI_Wtime");
    tftk_mpi.wtick = (double (*)(void))
        tftk_mpi_platform_dlsym(handle, "MPI_Wtick");


    /* Load P2P functions */
    tftk_mpi.send = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Send");
    tftk_mpi.recv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Recv");
    tftk_mpi.isend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Isend");
    tftk_mpi.irecv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Irecv");
    tftk_mpi.wait = (int (*)(MPI_Request*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Wait");
    tftk_mpi.waitall = (int (*)(int, MPI_Request*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Waitall");

    /* Load collective functions */
    tftk_mpi.bcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Bcast");
    tftk_mpi.reduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Reduce");
    tftk_mpi.allreduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Allreduce");
    tftk_mpi.gather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Gather");
    tftk_mpi.allgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Allgather");
    tftk_mpi.scatter = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Scatter");

    /* Load communicator functions */
    tftk_mpi.comm_size = (int (*)(MPI_Comm, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_size");
    tftk_mpi.comm_rank = (int (*)(MPI_Comm, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_rank");
    tftk_mpi.comm_dup = (int (*)(MPI_Comm, MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_dup");
    tftk_mpi.comm_split = (int (*)(MPI_Comm, int, int, MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_split");
    tftk_mpi.comm_free = (int (*)(MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_free");

    /* Get predefined communicator values */
    get_openmpi_comm_world(handle, &TFTK_MPI_COMM_WORLD);

    return TFTK_MPI_OK;
}
