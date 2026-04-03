/* src/backends/mpich.c */
#include "tftk_mpi_vtable.h"
#include "tftk_mpi_platform.h"
#include "tftk_mpi.h"

/* MPICH uses integers for communicators */
static int get_mpich_comm_world(tftk_mpi_lib_handle_t handle, MPI_Comm *comm) {
    int *ptr = (int*)tftk_mpi_platform_dlsym(handle, "MPIR_Comm_world");
    if (ptr) {
        *comm = *ptr;
        return TFTK_MPI_OK;
    }
    *comm = 91;
    return TFTK_MPI_OK;
}

int tftk_mpi_vtable_init_mpich(tftk_mpi_lib_handle_t handle) {
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

    /* Get predefined communicator values */
    get_mpich_comm_world(handle, &TFTK_MPI_COMM_WORLD);

    return TFTK_MPI_OK;
}
