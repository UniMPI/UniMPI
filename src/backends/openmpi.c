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
        *comm = (MPI_Comm)(size_t)ptr;
        return TFTK_MPI_OK;
    }
    return TFTK_MPI_ERR_SYMBOL_NOT_FOUND;
}

int tftk_mpi_vtable_init_openmpi(tftk_mpi_lib_handle_t handle) {
    /* Environment Management */
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

    /* Point-to-Point - Standard */
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
    tftk_mpi.sendrecv = (int (*)(const void*, int, MPI_Datatype, int, int, void*, int, MPI_Datatype, int, int, MPI_Comm, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Sendrecv");

    /* Point-to-Point - Sync/Buffered/Ready */
    tftk_mpi.ssend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Ssend");
    tftk_mpi.ssend_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ssend_init");
    tftk_mpi.bsend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Bsend");
    tftk_mpi.bsend_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Bsend_init");
    tftk_mpi.buffer_attach = (int (*)(void*, int))
        tftk_mpi_platform_dlsym(handle, "MPI_Buffer_attach");
    tftk_mpi.buffer_detach = (int (*)(void*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Buffer_detach");
    tftk_mpi.rsend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Rsend");
    tftk_mpi.rsend_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Rsend_init");

    /* Nonblocking test and wait */
    tftk_mpi.test = (int (*)(MPI_Request*, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Test");
    tftk_mpi.testany = (int (*)(int, MPI_Request*, int*, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Testany");
    tftk_mpi.testsome = (int (*)(int, MPI_Request*, int*, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Testsome");
    tftk_mpi.testall = (int (*)(int, MPI_Request*, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Testall");
    tftk_mpi.waitany = (int (*)(int, MPI_Request*, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Waitany");
    tftk_mpi.waitsome = (int (*)(int, MPI_Request*, int*, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Waitsome");

    /* Message probing */
    tftk_mpi.probe = (int (*)(int, int, MPI_Comm, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Probe");
    tftk_mpi.iprobe = (int (*)(int, int, MPI_Comm, int*, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iprobe");

    /* Persistent communication */
    tftk_mpi.send_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Send_init");
    tftk_mpi.recv_init = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Recv_init");
    tftk_mpi.start = (int (*)(MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Start");
    tftk_mpi.startall = (int (*)(int, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Startall");
    tftk_mpi.request_free = (int (*)(MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Request_free");

    /* Cancel and status */
    tftk_mpi.cancel = (int (*)(MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Cancel");
    tftk_mpi.test_cancelled = (int (*)(const TFTK_MPI_Status*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Test_cancelled");
    tftk_mpi.get_count = (int (*)(const TFTK_MPI_Status*, MPI_Datatype, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Get_count");
    tftk_mpi.get_elements = (int (*)(const TFTK_MPI_Status*, MPI_Datatype, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Get_elements");

    /* Collective - Standard */
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

    /* Collective - Variable length */
    tftk_mpi.gatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Gatherv");
    tftk_mpi.allgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Allgatherv");
    tftk_mpi.scatterv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Scatterv");
    tftk_mpi.alltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Alltoall");
    tftk_mpi.alltoallv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Alltoallv");

    /* Collective - Reduce-scatter and scan */
    tftk_mpi.reduce_scatter = (int (*)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Reduce_scatter");
    tftk_mpi.reduce_scatter_block = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Reduce_scatter_block");
    tftk_mpi.scan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Scan");
    tftk_mpi.exscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Exscan");

    /* Communicator */
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

    /* Datatypes - Creation */
    tftk_mpi.type_commit = (int (*)(MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_commit");
    tftk_mpi.type_free = (int (*)(MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_free");
    tftk_mpi.type_contiguous = (int (*)(int, MPI_Datatype, MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_contiguous");
    tftk_mpi.type_vector = (int (*)(int, int, int, MPI_Datatype, MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_vector");
    tftk_mpi.type_indexed = (int (*)(int, const int*, const int*, MPI_Datatype, MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_indexed");
    tftk_mpi.type_create_indexed_block = (int (*)(int, int, const int*, MPI_Datatype, MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_create_indexed_block");
    tftk_mpi.type_create_subarray = (int (*)(int, const int*, const int*, const int*, int, MPI_Datatype, MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_create_subarray");
    tftk_mpi.type_dup = (int (*)(MPI_Datatype, MPI_Datatype*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_dup");

    /* Datatypes - Query */
    tftk_mpi.type_get_extent = (int (*)(MPI_Datatype, MPI_Aint*, MPI_Aint*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_get_extent");
    tftk_mpi.type_get_size = (int (*)(MPI_Datatype, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_get_size");
    tftk_mpi.type_get_name = (int (*)(MPI_Datatype, char*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_get_name");
    tftk_mpi.type_set_name = (int (*)(MPI_Datatype, const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Type_set_name");

    /* Pack/Unpack */
    tftk_mpi.pack = (int (*)(const void*, int, MPI_Datatype, void*, int, int*, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Pack");
    tftk_mpi.unpack = (int (*)(const void*, int, int*, void*, int, MPI_Datatype, MPI_Comm))
        tftk_mpi_platform_dlsym(handle, "MPI_Unpack");
    tftk_mpi.pack_size = (int (*)(int, MPI_Datatype, MPI_Comm, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Pack_size");

    /* Get predefined communicator values */
    get_openmpi_comm_world(handle, &TFTK_MPI_COMM_WORLD);

    return TFTK_MPI_OK;
}
