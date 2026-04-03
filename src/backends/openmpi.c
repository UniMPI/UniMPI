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

    /* MPI-3 Non-blocking Collectives */
    tftk_mpi.ibarrier = (int (*)(MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ibarrier");
    tftk_mpi.ibcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ibcast");
    tftk_mpi.igather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Igather");
    tftk_mpi.igatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Igatherv");
    tftk_mpi.iscatter = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iscatter");
    tftk_mpi.iscatterv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iscatterv");
    tftk_mpi.iallgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iallgather");
    tftk_mpi.iallgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iallgatherv");
    tftk_mpi.ialltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ialltoall");
    tftk_mpi.ialltoallv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ialltoallv");
    tftk_mpi.ireduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ireduce");
    tftk_mpi.iallreduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iallreduce");
    tftk_mpi.ireduce_scatter = (int (*)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ireduce_scatter");
    tftk_mpi.ireduce_scatter_block = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Ireduce_scatter_block");
    tftk_mpi.iscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iscan");
    tftk_mpi.iexscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Iexscan");

    /* Group operations */
    tftk_mpi.group_size = (int (*)(MPI_Group, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_size");
    tftk_mpi.group_rank = (int (*)(MPI_Group, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_rank");
    tftk_mpi.group_translate_ranks = (int (*)(MPI_Group, int, const int*, MPI_Group, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_translate_ranks");
    tftk_mpi.group_compare = (int (*)(MPI_Group, MPI_Group, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_compare");
    tftk_mpi.group_union = (int (*)(MPI_Group, MPI_Group, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_union");
    tftk_mpi.group_intersection = (int (*)(MPI_Group, MPI_Group, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_intersection");
    tftk_mpi.group_difference = (int (*)(MPI_Group, MPI_Group, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_difference");
    tftk_mpi.group_incl = (int (*)(MPI_Group, int, const int*, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_incl");
    tftk_mpi.group_excl = (int (*)(MPI_Group, int, const int*, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_excl");
    tftk_mpi.group_range_incl = (int (*)(MPI_Group, int, int[][3], MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_range_incl");
    tftk_mpi.group_range_excl = (int (*)(MPI_Group, int, int[][3], MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_range_excl");
    tftk_mpi.group_free = (int (*)(MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Group_free");

    /* Communicator extended */
    tftk_mpi.comm_create = (int (*)(MPI_Comm, MPI_Group, MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_create");
    tftk_mpi.comm_group = (int (*)(MPI_Comm, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_group");
    tftk_mpi.comm_set_name = (int (*)(MPI_Comm, const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_set_name");
    tftk_mpi.comm_get_name = (int (*)(MPI_Comm, char*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_get_name");

    /* RMA - Window creation */
    tftk_mpi.win_create = (int (*)(void*, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win*))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_create");
    tftk_mpi.win_allocate = (int (*)(MPI_Aint, int, MPI_Info, MPI_Comm, void*, MPI_Win*))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_allocate");
    tftk_mpi.win_free = (int (*)(MPI_Win*))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_free");

    /* RMA Operations */
    tftk_mpi.put = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Put");
    tftk_mpi.get = (int (*)(void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Get");
    tftk_mpi.accumulate = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Accumulate");
    tftk_mpi.get_accumulate = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Get_accumulate");
    tftk_mpi.fetch_and_op = (int (*)(const void*, void*, MPI_Datatype, int, MPI_Aint, MPI_Op, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Fetch_and_op");
    tftk_mpi.compare_and_swap = (int (*)(const void*, const void*, void*, MPI_Datatype, int, MPI_Aint, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Compare_and_swap");
    tftk_mpi.rput = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Rput");
    tftk_mpi.rget = (int (*)(void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_Rget");

    /* RMA Synchronization */
    tftk_mpi.win_fence = (int (*)(int, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_fence");
    tftk_mpi.win_lock = (int (*)(int, int, int, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_lock");
    tftk_mpi.win_unlock = (int (*)(int, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_unlock");
    tftk_mpi.win_lock_all = (int (*)(int, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_lock_all");
    tftk_mpi.win_unlock_all = (int (*)(MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_unlock_all");
    tftk_mpi.win_flush = (int (*)(int, MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_flush");
    tftk_mpi.win_flush_all = (int (*)(MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_flush_all");
    tftk_mpi.win_sync = (int (*)(MPI_Win))
        tftk_mpi_platform_dlsym(handle, "MPI_Win_sync");

    /* Parallel I/O - File Operations */
    tftk_mpi.file_open = (int (*)(MPI_Comm, const char*, int, MPI_Info, MPI_File*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_open");
    tftk_mpi.file_close = (int (*)(MPI_File*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_close");
    tftk_mpi.file_delete = (int (*)(const char*, MPI_Info))
        tftk_mpi_platform_dlsym(handle, "MPI_File_delete");
    tftk_mpi.file_set_size = (int (*)(MPI_File, MPI_Offset))
        tftk_mpi_platform_dlsym(handle, "MPI_File_set_size");
    tftk_mpi.file_preallocate = (int (*)(MPI_File, MPI_Offset))
        tftk_mpi_platform_dlsym(handle, "MPI_File_preallocate");
    tftk_mpi.file_get_size = (int (*)(MPI_File, MPI_Offset*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_get_size");
    tftk_mpi.file_get_group = (int (*)(MPI_File, MPI_Group*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_get_group");
    tftk_mpi.file_get_amode = (int (*)(MPI_File, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_get_amode");

    /* Parallel I/O - Read/Write */
    tftk_mpi.file_read = (int (*)(MPI_File, void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_read");
    tftk_mpi.file_read_all = (int (*)(MPI_File, void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_read_all");
    tftk_mpi.file_write = (int (*)(MPI_File, const void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_write");
    tftk_mpi.file_write_all = (int (*)(MPI_File, const void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_write_all");
    tftk_mpi.file_read_at = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_read_at");
    tftk_mpi.file_read_at_all = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_read_at_all");
    tftk_mpi.file_write_at = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_write_at");
    tftk_mpi.file_write_at_all = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype, TFTK_MPI_Status*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_write_at_all");

    /* Parallel I/O - Non-blocking */
    tftk_mpi.file_iread = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_iread");
    tftk_mpi.file_iwrite = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_iwrite");
    tftk_mpi.file_iread_at = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_iread_at");
    tftk_mpi.file_iwrite_at = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype, MPI_Request*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_iwrite_at");

    /* Parallel I/O - Views */
    tftk_mpi.file_set_view = (int (*)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, const char*, MPI_Info))
        tftk_mpi_platform_dlsym(handle, "MPI_File_set_view");
    tftk_mpi.file_get_view = (int (*)(MPI_File, MPI_Offset*, MPI_Datatype*, MPI_Datatype*, char*))
        tftk_mpi_platform_dlsym(handle, "MPI_File_get_view");

    /* Dynamic Process Management */
    tftk_mpi.comm_spawn = (int (*)(const char*, char*[], int, MPI_Info, int, MPI_Comm, MPI_Comm*, int[]))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_spawn");
    tftk_mpi.comm_spawn_multiple = (int (*)(int, char*[], char**[], const int[], const MPI_Info[], int, MPI_Comm, MPI_Comm*, int[]))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_spawn_multiple");
    tftk_mpi.comm_accept = (int (*)(const char*, MPI_Info, int, MPI_Comm, MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_accept");
    tftk_mpi.comm_connect = (int (*)(const char*, MPI_Info, int, MPI_Comm, MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_connect");
    tftk_mpi.comm_disconnect = (int (*)(MPI_Comm*))
        tftk_mpi_platform_dlsym(handle, "MPI_Comm_disconnect");

    /* Port and Name Service */
    tftk_mpi.open_port = (int (*)(MPI_Info, char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Open_port");
    tftk_mpi.close_port = (int (*)(const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Close_port");
    tftk_mpi.publish_name = (int (*)(const char*, MPI_Info, const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Publish_name");
    tftk_mpi.unpublish_name = (int (*)(const char*, MPI_Info, const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Unpublish_name");
    tftk_mpi.lookup_name = (int (*)(const char*, MPI_Info, char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Lookup_name");

    /* Info Operations */
    tftk_mpi.info_create = (int (*)(MPI_Info*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_create");
    tftk_mpi.info_free = (int (*)(MPI_Info*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_free");
    tftk_mpi.info_set = (int (*)(MPI_Info, const char*, const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_set");
    tftk_mpi.info_get = (int (*)(MPI_Info, const char*, int, char*, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_get");
    tftk_mpi.info_delete = (int (*)(MPI_Info, const char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_delete");
    tftk_mpi.info_get_nkeys = (int (*)(MPI_Info, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_get_nkeys");
    tftk_mpi.info_get_nthkey = (int (*)(MPI_Info, int, char*))
        tftk_mpi_platform_dlsym(handle, "MPI_Info_get_nthkey");

    /* Thread Support */
    tftk_mpi.init_thread = (int (*)(int*, char***, int, int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Init_thread");
    tftk_mpi.query_thread = (int (*)(int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Query_thread");
    tftk_mpi.is_thread_main = (int (*)(int*))
        tftk_mpi_platform_dlsym(handle, "MPI_Is_thread_main");

    /* Memory Allocation */
    tftk_mpi.alloc_mem = (int (*)(MPI_Aint, MPI_Info, void*))
        tftk_mpi_platform_dlsym(handle, "MPI_Alloc_mem");
    tftk_mpi.free_mem = (int (*)(void*))
        tftk_mpi_platform_dlsym(handle, "MPI_Free_mem");

    /* Get predefined communicator values */
    get_openmpi_comm_world(handle, &TFTK_MPI_COMM_WORLD);

    return TFTK_MPI_OK;
}
