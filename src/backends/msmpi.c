/* src/backends/msmpi.c */
#include "unimpi_vtable.h"
#include "unimpi_platform.h"
#include "unimpi.h"
#include "msmpi_wrappers.h"
#include <string.h>

/* MS-MPI uses MPICH-compatible error codes */
static void init_msmpi_error_codes(void) {
    /* MS-MPI (based on MPICH) uses standard error class values */
    MPI_SUCCESS = 0;
    MPI_ERR_BUFFER = 1;
    MPI_ERR_COUNT = 2;
    MPI_ERR_TYPE = 3;
    MPI_ERR_TAG = 4;
    MPI_ERR_COMM = 5;
    MPI_ERR_RANK = 6;
    MPI_ERR_REQUEST = 7;
    MPI_ERR_ROOT = 8;
    MPI_ERR_GROUP = 9;
    MPI_ERR_OP = 10;
    MPI_ERR_TOPOLOGY = 11;
    MPI_ERR_DIMS = 12;
    MPI_ERR_ARG = 13;
    MPI_ERR_UNKNOWN = 14;
    MPI_ERR_TRUNCATE = 15;
    MPI_ERR_OTHER = 16;
    MPI_ERR_INTERN = 17;
    MPI_ERR_IN_STATUS = 18;
    MPI_ERR_PENDING = 19;
    MPI_ERR_ACCESS = 20;
    MPI_ERR_AMODE = 21;
    MPI_ERR_ASSERT = 22;
    MPI_ERR_BAD_FILE = 23;
    MPI_ERR_BASE = 24;
    MPI_ERR_CONVERSION = 25;
    MPI_ERR_DISP = 26;
    MPI_ERR_DUP_DATAREP = 27;
    MPI_ERR_FILE_EXISTS = 28;
    MPI_ERR_FILE_IN_USE = 29;
    MPI_ERR_FILE = 30;
    MPI_ERR_INFO_KEY = 31;
    MPI_ERR_INFO_NOKEY = 32;
    MPI_ERR_INFO_VALUE = 33;
    MPI_ERR_INFO = 34;
    MPI_ERR_IO = 35;
    MPI_ERR_KEYVAL = 36;
    MPI_ERR_LOCKTYPE = 37;
    MPI_ERR_NAME = 38;
    MPI_ERR_NO_MEM = 39;
    MPI_ERR_NOT_SAME = 40;
    MPI_ERR_NO_SPACE = 41;
    MPI_ERR_NO_SUCH_FILE = 42;
    MPI_ERR_PORT = 43;
    MPI_ERR_QUOTA = 44;
    MPI_ERR_READ_ONLY = 45;
    MPI_ERR_RMA_CONFLICT = 46;
    MPI_ERR_RMA_SYNC = 47;
    MPI_ERR_SERVICE = 48;
    MPI_ERR_SIZE = 49;
    MPI_ERR_SPAWN = 50;
    MPI_ERR_UNSUPPORTED_DATAREP = 51;
    MPI_ERR_UNSUPPORTED_OPERATION = 52;
    MPI_ERR_WIN = 53;
    /* Note: MPI_ERR_PROC_FAILED and MPI_ERR_PROC_FAIL_STOP do not exist
     * in MS-MPI system headers per design principle "backend decides".
     * Only define error codes that actually exist in the backend. */
    MPI_ERR_LASTCODE = 54;
}

/* MS-MPI uses different predefined communicator values than MPICH */
/* MPI_COMM_WORLD = 0x44000000, MPI_COMM_SELF = 0x44000001 */



int unimpi_vtable_init_msmpi(unimpi_lib_handle_t handle) {
    /* Environment Management */
    unimpi.init = (int (*)(int*, char***))
        unimpi_platform_dlsym(handle, "MPI_Init");
    unimpi.init_thread = (int (*)(int*, char***, int, int*))
        unimpi_platform_dlsym(handle, "MPI_Init_thread");
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
    unimpi.get_library_version = (int (*)(char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Get_library_version");
    unimpi.barrier = (int (*)(MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Barrier");
    unimpi.wtime = (double (*)(void))
        unimpi_platform_dlsym(handle, "MPI_Wtime");
    unimpi.wtick = (double (*)(void))
        unimpi_platform_dlsym(handle, "MPI_Wtick");

    /* Point-to-Point - Standard */
    unimpi.send = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Send");
    unimpi.recv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Recv");
    unimpi.isend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Isend");
    unimpi.irecv = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Irecv");
    unimpi.wait = (int (*)(MPI_Request*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Wait");
    (msmpi_waitall = (int (*)(int, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitall"));
    unimpi.waitall = msmpi_wrap_waitall;
    unimpi.sendrecv = (int (*)(const void*, int, MPI_Datatype, int, int, void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Sendrecv");
    unimpi.sendrecv_replace = (int (*)(void*, int, MPI_Datatype, int, int, int, int, MPI_Comm, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Sendrecv_replace");

    /* Point-to-Point - Sync/Buffered/Ready */
    unimpi.ssend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Ssend");
    unimpi.ssend_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ssend_init");
    unimpi.bsend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Bsend");
    unimpi.bsend_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Bsend_init");
    unimpi.buffer_attach = (int (*)(void*, int))
        unimpi_platform_dlsym(handle, "MPI_Buffer_attach");
    unimpi.buffer_detach = (int (*)(void*, int*))
        unimpi_platform_dlsym(handle, "MPI_Buffer_detach");
    unimpi.rsend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Rsend");
    unimpi.rsend_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Rsend_init");

    /* Nonblocking test and wait */
    unimpi.test = (int (*)(MPI_Request*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Test");
    (msmpi_testany = (int (*)(int, int*, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Testany"));
    unimpi.testany = msmpi_wrap_testany;
    (msmpi_testsome = (int (*)(int, int*, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Testsome"));
    unimpi.testsome = msmpi_wrap_testsome;
    (msmpi_testall = (int (*)(int, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Testall"));
    unimpi.testall = msmpi_wrap_testall;
    (msmpi_waitany = (int (*)(int, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitany"));
    unimpi.waitany = msmpi_wrap_waitany;
    (msmpi_waitsome = (int (*)(int, int*, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitsome"));
    unimpi.waitsome = msmpi_wrap_waitsome;

    /* Message probing */
    unimpi.probe = (int (*)(int, int, MPI_Comm, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Probe");
    unimpi.iprobe = (int (*)(int, int, MPI_Comm, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Iprobe");

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 matched_probe */
    unimpi.mprobe = (int (*)(int, int, MPI_Comm, MPI_Message*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Mprobe");
    unimpi.improbe = (int (*)(int, int, MPI_Comm, int*, MPI_Message*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Improbe");
    unimpi.mrecv = (int (*)(void*, int, MPI_Datatype, MPI_Message*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Mrecv");
    unimpi.imrecv = (int (*)(void*, int, MPI_Datatype, MPI_Message*, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Imrecv");
#endif

    /* Persistent communication */
    unimpi.send_init = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Send_init");
    unimpi.recv_init = (int (*)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Recv_init");
    unimpi.start = (int (*)(MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Start");
    (msmpi_startall = (int (*)(int, int*))
        unimpi_platform_dlsym(handle, "MPI_Startall"));
    unimpi.startall = msmpi_wrap_startall;
    unimpi.request_free = (int (*)(MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Request_free");

    /* Cancel and status */
    unimpi.cancel = (int (*)(MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Cancel");
    unimpi.test_cancelled = (int (*)(const MPI_Status*, int*))
        unimpi_platform_dlsym(handle, "MPI_Test_cancelled");
    unimpi.get_count = (int (*)(const MPI_Status*, MPI_Datatype, int*))
        unimpi_platform_dlsym(handle, "MPI_Get_count");
    unimpi.get_elements = (int (*)(const MPI_Status*, MPI_Datatype, int*))
        unimpi_platform_dlsym(handle, "MPI_Get_elements");

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

    /* Collective - Variable length */
    unimpi.gatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Gatherv");
    unimpi.allgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Allgatherv");
    unimpi.scatterv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Scatterv");
    unimpi.alltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Alltoall");
    unimpi.alltoallv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Alltoallv");

    /* MPI-3 Alltoallw (wrapped: adapts 8-byte datatype arrays to the native
     * 4-byte handle representation) */
    msmpi_alltoallw = (int (*)(const void*, const int*,
            const int*, const int*, void*, const int*, const int*, const int*,
            MPI_Comm))
            unimpi_platform_dlsym(handle, "MPI_Alltoallw");
    if (msmpi_alltoallw) {
        unimpi.alltoallw = msmpi_wrap_alltoallw;
    }

    /* Collective - Reduce-scatter and scan */
    unimpi.reduce_scatter = (int (*)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Reduce_scatter");
    unimpi.reduce_scatter_block = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Reduce_scatter_block");
    unimpi.scan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Scan");
    unimpi.exscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Exscan");

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 nonblocking_collectives */
    unimpi.ibarrier = (int (*)(MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ibarrier");
    unimpi.ibcast = (int (*)(void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ibcast");
    unimpi.igather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Igather");
    unimpi.igatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Igatherv");
    unimpi.iscatter = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iscatter");
    unimpi.iscatterv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iscatterv");
    unimpi.iallgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iallgather");
    unimpi.iallgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iallgatherv");
    unimpi.ialltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ialltoall");
    unimpi.ialltoallv = (int (*)(const void*, const int*, const int*, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ialltoallv");

    /* MPI-3 Ialltoallw (wrapped: adapts 8-byte datatype arrays to the native
     * 4-byte handle representation) */
    msmpi_ialltoallw = (int (*)(const void*, const int*,
            const int*, const int*, void*, const int*, const int*, const int*,
            MPI_Comm, MPI_Request*))
            unimpi_platform_dlsym(handle, "MPI_Ialltoallw");
    if (msmpi_ialltoallw) {
        unimpi.ialltoallw = msmpi_wrap_ialltoallw;
    }
    unimpi.ireduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ireduce");
    unimpi.iallreduce = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iallreduce");
    unimpi.ireduce_scatter = (int (*)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ireduce_scatter");
    unimpi.ireduce_scatter_block = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ireduce_scatter_block");
    unimpi.iscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iscan");
    unimpi.iexscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Iexscan");
#endif

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 neighbor_collectives */
    unimpi.neighbor_allgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Neighbor_allgather");
    unimpi.neighbor_allgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Neighbor_allgatherv");
    unimpi.neighbor_alltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Neighbor_alltoall");
    /* Neighbor_alltoallv/w and Ineighbor_* wrapped (datatype arrays) */
    msmpi_neighbor_alltoallv = (int (*)(const void*, const int*, const MPI_Aint*, const int*,
            void*, const int*, const MPI_Aint*, const int*, MPI_Comm))
            unimpi_platform_dlsym(handle, "MPI_Neighbor_alltoallv");
    if (msmpi_neighbor_alltoallv)
        unimpi.neighbor_alltoallv = msmpi_wrap_neighbor_alltoallv;
    msmpi_neighbor_alltoallw = (int (*)(const void*, const int*, const MPI_Aint*, const int*,
            void*, const int*, const MPI_Aint*, const int*, MPI_Comm))
            unimpi_platform_dlsym(handle, "MPI_Neighbor_alltoallw");
    if (msmpi_neighbor_alltoallw)
        unimpi.neighbor_alltoallw = msmpi_wrap_neighbor_alltoallw;
    unimpi.ineighbor_allgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_allgather");
    unimpi.ineighbor_allgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_allgatherv");
    unimpi.ineighbor_alltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_alltoall");
    msmpi_ineighbor_alltoallv = (int (*)(const void*, const int*, const MPI_Aint*, const int*,
            void*, const int*, const MPI_Aint*, const int*, MPI_Comm, MPI_Request*))
            unimpi_platform_dlsym(handle, "MPI_Ineighbor_alltoallv");
    if (msmpi_ineighbor_alltoallv)
        unimpi.ineighbor_alltoallv = msmpi_wrap_ineighbor_alltoallv;
    msmpi_ineighbor_alltoallw = (int (*)(const void*, const int*, const MPI_Aint*, const int*,
            void*, const int*, const MPI_Aint*, const int*, MPI_Comm, MPI_Request*))
            unimpi_platform_dlsym(handle, "MPI_Ineighbor_alltoallw");
    if (msmpi_ineighbor_alltoallw)
        unimpi.ineighbor_alltoallw = msmpi_wrap_ineighbor_alltoallw;
#endif

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

    /* Group operations */
    unimpi.group_size = (int (*)(MPI_Group, int*))
        unimpi_platform_dlsym(handle, "MPI_Group_size");
    unimpi.group_rank = (int (*)(MPI_Group, int*))
        unimpi_platform_dlsym(handle, "MPI_Group_rank");
    unimpi.group_translate_ranks = (int (*)(MPI_Group, int, const int*, MPI_Group, int*))
        unimpi_platform_dlsym(handle, "MPI_Group_translate_ranks");
    unimpi.group_compare = (int (*)(MPI_Group, MPI_Group, int*))
        unimpi_platform_dlsym(handle, "MPI_Group_compare");
    unimpi.group_union = (int (*)(MPI_Group, MPI_Group, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_union");
    unimpi.group_intersection = (int (*)(MPI_Group, MPI_Group, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_intersection");
    unimpi.group_difference = (int (*)(MPI_Group, MPI_Group, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_difference");
    unimpi.group_incl = (int (*)(MPI_Group, int, const int*, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_incl");
    unimpi.group_excl = (int (*)(MPI_Group, int, const int*, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_excl");
    unimpi.group_range_incl = (int (*)(MPI_Group, int, int[][3], MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_range_incl");
    unimpi.group_range_excl = (int (*)(MPI_Group, int, int[][3], MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_range_excl");
    unimpi.group_free = (int (*)(MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Group_free");

    /* Communicator extended */
    unimpi.comm_create = (int (*)(MPI_Comm, MPI_Group, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_create");
    unimpi.comm_group = (int (*)(MPI_Comm, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Comm_group");
    unimpi.comm_compare = (int (*)(MPI_Comm, MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_compare");
    unimpi.comm_set_name = (int (*)(MPI_Comm, const char*))
        unimpi_platform_dlsym(handle, "MPI_Comm_set_name");
    unimpi.comm_get_name = (int (*)(MPI_Comm, char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_get_name");
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 comm_3x */
    unimpi.comm_dup_with_info = (int (*)(MPI_Comm, MPI_Info, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_dup_with_info");
    unimpi.comm_split_type = (int (*)(MPI_Comm, int, int, MPI_Info, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_split_type");
    unimpi.comm_create_group = (int (*)(MPI_Comm, MPI_Group, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_create_group");
    unimpi.comm_get_info = (int (*)(MPI_Comm, MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_Comm_get_info");
    unimpi.comm_set_info = (int (*)(MPI_Comm, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_Comm_set_info");
    unimpi.comm_idup = (int (*)(MPI_Comm, MPI_Comm*, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Comm_idup");
#endif

    /* Intercommunicator Operations (MPI-2.2) */
    unimpi.intercomm_create = (int (*)(MPI_Comm, int, MPI_Comm, int, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Intercomm_create");
    unimpi.intercomm_merge = (int (*)(MPI_Comm, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Intercomm_merge");
    unimpi.comm_remote_size = (int (*)(MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_remote_size");
    unimpi.comm_remote_group = (int (*)(MPI_Comm, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_remote_group");
    unimpi.comm_test_inter = (int (*)(MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_test_inter");

    /* Process Topologies - Cartesian */
    unimpi.cart_create = (int (*)(MPI_Comm, int, const int*, const int*, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Cart_create");
    unimpi.cartdim_get = (int (*)(MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Cartdim_get");
    unimpi.cart_get = (int (*)(MPI_Comm, int, int*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Cart_get");
    unimpi.cart_rank = (int (*)(MPI_Comm, const int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Cart_rank");
    unimpi.cart_coords = (int (*)(MPI_Comm, int, int, int*))
        unimpi_platform_dlsym(handle, "MPI_Cart_coords");
    unimpi.cart_shift = (int (*)(MPI_Comm, int, int, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Cart_shift");
    unimpi.cart_sub = (int (*)(MPI_Comm, const int*, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Cart_sub");
    unimpi.cart_map = (int (*)(MPI_Comm, int, const int*, const int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Cart_map");
    unimpi.dims_create = (int (*)(int, int, int*))
        unimpi_platform_dlsym(handle, "MPI_Dims_create");

    /* Process Topologies - Graph */
    unimpi.graph_create = (int (*)(MPI_Comm, int, const int*, const int*, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Graph_create");
    unimpi.graphdims_get = (int (*)(MPI_Comm, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Graphdims_get");
    unimpi.graph_get = (int (*)(MPI_Comm, int, int, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Graph_get");
    unimpi.graph_neighbors_count = (int (*)(MPI_Comm, int, int*))
        unimpi_platform_dlsym(handle, "MPI_Graph_neighbors_count");
    unimpi.graph_neighbors = (int (*)(MPI_Comm, int, int, int*))
        unimpi_platform_dlsym(handle, "MPI_Graph_neighbors");
    unimpi.graph_map = (int (*)(MPI_Comm, int, const int*, const int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Graph_map");

    unimpi.dist_graph_create = (int (*)(MPI_Comm, int, const int*, const int*, const int*, const int*, MPI_Info, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Dist_graph_create");
    unimpi.dist_graph_create_adjacent = (int (*)(MPI_Comm, int, const int*, const int*, int, const int*, const int*, MPI_Info, int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Dist_graph_create_adjacent");
    unimpi.dist_graph_neighbors_count = (int (*)(MPI_Comm, int*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Dist_graph_neighbors_count");
    unimpi.dist_graph_neighbors = (int (*)(MPI_Comm, int, int*, int*, int, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Dist_graph_neighbors");

    /* Topology Testing */
    unimpi.topo_test = (int (*)(MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Topo_test");

    /* Datatypes - Creation */
    unimpi.type_commit = (int (*)(MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_commit");
    unimpi.type_free = (int (*)(MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_free");
    unimpi.type_contiguous = (int (*)(int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_contiguous");
    unimpi.type_vector = (int (*)(int, int, int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_vector");
    unimpi.type_indexed = (int (*)(int, const int*, const int*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_indexed");
    unimpi.type_create_indexed_block = (int (*)(int, int, const int*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_indexed_block");
    unimpi.type_create_subarray = (int (*)(int, const int*, const int*, const int*, int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_subarray");
    unimpi.type_dup = (int (*)(MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_dup");

    /* MPI-2.2 Extended datatypes */
    unimpi.type_create_resized = (int (*)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_resized");
    unimpi.type_get_envelope = (int (*)(MPI_Datatype, int*, int*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_envelope");
    /* Type_get_contents (wrapped) */
    msmpi_type_get_contents = (int (*)(MPI_Datatype, int, int, int,
            int*, MPI_Aint*, MPI_Datatype*))
            unimpi_platform_dlsym(handle, "MPI_Type_get_contents");
    if (msmpi_type_get_contents) {
        unimpi.type_get_contents = msmpi_wrap_type_get_contents;
    }

    /* MPI-3 Extended datatypes */
    unimpi.type_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_hvector");
    unimpi.type_hindexed = (int (*)(int, const int*, const MPI_Aint*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_hindexed");
    unimpi.type_create_darray = (int (*)(int, int, int, const int*, const int*, const int*, const int*, int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_darray");

    /* Datatypes - Query */
    unimpi.type_get_extent = (int (*)(MPI_Datatype, MPI_Aint*, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_extent");
    unimpi.type_get_size = (int (*)(MPI_Datatype, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_size");
    unimpi.type_get_name = (int (*)(MPI_Datatype, char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_name");
    unimpi.type_set_name = (int (*)(MPI_Datatype, const char*))
        unimpi_platform_dlsym(handle, "MPI_Type_set_name");

    /* MPI-3 Extended datatype query */
    unimpi.type_get_true_extent = (int (*)(MPI_Datatype, MPI_Aint*, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_true_extent");
    unimpi.type_size = (int (*)(MPI_Datatype, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_size");
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 large_count */
    unimpi.type_size_x = (int (*)(MPI_Datatype, MPI_Count*))
        unimpi_platform_dlsym(handle, "MPI_Type_size_x");
    unimpi.type_get_extent_x = (int (*)(MPI_Datatype, MPI_Count*, MPI_Count*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_extent_x");
    unimpi.type_get_true_extent_x = (int (*)(MPI_Datatype, MPI_Count*, MPI_Count*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_true_extent_x");
    unimpi.type_create_hindexed_block = (int (*)(int, int, const MPI_Aint*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_hindexed_block");
    unimpi.get_elements_x = (int (*)(const MPI_Status*, MPI_Datatype, MPI_Count*))
        unimpi_platform_dlsym(handle, "MPI_Get_elements_x");
    unimpi.status_set_elements_x = (int (*)(MPI_Status*, MPI_Datatype, MPI_Count))
        unimpi_platform_dlsym(handle, "MPI_Status_set_elements_x");
#endif
    unimpi.type_extent = (int (*)(MPI_Datatype, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_extent");
    unimpi.type_lb = (int (*)(MPI_Datatype, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_lb");
    unimpi.type_ub = (int (*)(MPI_Datatype, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_ub");

    /* Pack/Unpack */
    unimpi.pack = (int (*)(const void*, int, MPI_Datatype, void*, int, int*, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Pack");
    unimpi.unpack = (int (*)(const void*, int, int*, void*, int, MPI_Datatype, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Unpack");
    unimpi.pack_size = (int (*)(int, MPI_Datatype, MPI_Comm, int*))
        unimpi_platform_dlsym(handle, "MPI_Pack_size");

    /* MPI-3 External pack/unpack */
    unimpi.pack_external = (int (*)(const char*, const void*, int, MPI_Datatype, void*, MPI_Aint, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Pack_external");
    unimpi.unpack_external = (int (*)(const char*, const void*, MPI_Aint, MPI_Aint*, void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_Unpack_external");
    unimpi.pack_external_size = (int (*)(const char*, int, MPI_Datatype, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Pack_external_size");

    /* RMA - Window creation */
    unimpi.win_create = (int (*)(void*, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_create");
    unimpi.win_allocate = (int (*)(MPI_Aint, int, MPI_Info, MPI_Comm, void*, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_allocate");
    unimpi.win_free = (int (*)(MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_free");

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 win_alloc_shared */
    unimpi.win_allocate_shared = (int (*)(MPI_Aint, int, MPI_Info, MPI_Comm, void*, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_allocate_shared");
    unimpi.win_create_dynamic = (int (*)(MPI_Info, MPI_Comm, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_create_dynamic");
#endif
    unimpi.win_set_name = (int (*)(MPI_Win, const char*))
        unimpi_platform_dlsym(handle, "MPI_Win_set_name");
    unimpi.win_get_name = (int (*)(MPI_Win, char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Win_get_name");

    /* RMA Operations */
    unimpi.put = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Put");
    unimpi.get = (int (*)(void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Get");
    unimpi.accumulate = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Accumulate");
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 rma_atomics */
    unimpi.get_accumulate = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Get_accumulate");
    unimpi.fetch_and_op = (int (*)(const void*, void*, MPI_Datatype, int, MPI_Aint, MPI_Op, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Fetch_and_op");
    unimpi.compare_and_swap = (int (*)(const void*, const void*, void*, MPI_Datatype, int, MPI_Aint, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Compare_and_swap");
    unimpi.rput = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Rput");
    unimpi.rget = (int (*)(void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Win, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Rget");

    /* MPI-3 Extended RMA operations */
    unimpi.raccumulate = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Raccumulate");
    unimpi.rget_accumulate = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Rget_accumulate");
#endif

    /* RMA Synchronization */
    unimpi.win_fence = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_fence");
    unimpi.win_lock = (int (*)(int, int, int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_lock");
    unimpi.win_unlock = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_unlock");
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 rma_sync_3x */
    unimpi.win_lock_all = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_lock_all");
    unimpi.win_unlock_all = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_unlock_all");
    unimpi.win_flush = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_flush");
    unimpi.win_flush_all = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_flush_all");
    unimpi.win_sync = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_sync");
    unimpi.win_flush_local = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_flush_local");
#endif
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 win_dynamic */
    unimpi.win_attach = (int (*)(MPI_Win, void*, MPI_Aint))
        unimpi_platform_dlsym(handle, "MPI_Win_attach");
    unimpi.win_detach = (int (*)(MPI_Win, void*))
        unimpi_platform_dlsym(handle, "MPI_Win_detach");
    unimpi.win_shared_query = (int (*)(MPI_Win, int, MPI_Aint*, int*, void*))
        unimpi_platform_dlsym(handle, "MPI_Win_shared_query");
    unimpi.win_flush_local_all = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_flush_local_all");
    unimpi.win_get_info = (int (*)(MPI_Win, MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_Win_get_info");
    unimpi.win_set_info = (int (*)(MPI_Win, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_Win_set_info");
#endif
    unimpi.win_start = (int (*)(MPI_Group, int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_start");
    unimpi.win_complete = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_complete");
    unimpi.win_post = (int (*)(MPI_Group, int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_post");
    unimpi.win_wait = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_wait");
    unimpi.win_test = (int (*)(MPI_Win, int*))
        unimpi_platform_dlsym(handle, "MPI_Win_test");

    /* Parallel I/O - File Operations */
    unimpi.file_open = (int (*)(MPI_Comm, const char*, int, MPI_Info, MPI_File*))
        unimpi_platform_dlsym(handle, "MPI_File_open");
    unimpi.file_close = (int (*)(MPI_File*))
        unimpi_platform_dlsym(handle, "MPI_File_close");
    unimpi.file_delete = (int (*)(const char*, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_File_delete");
    unimpi.file_set_size = (int (*)(MPI_File, MPI_Offset))
        unimpi_platform_dlsym(handle, "MPI_File_set_size");
    unimpi.file_preallocate = (int (*)(MPI_File, MPI_Offset))
        unimpi_platform_dlsym(handle, "MPI_File_preallocate");
    unimpi.file_get_size = (int (*)(MPI_File, MPI_Offset*))
        unimpi_platform_dlsym(handle, "MPI_File_get_size");
    unimpi.file_get_group = (int (*)(MPI_File, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_File_get_group");
    unimpi.file_get_amode = (int (*)(MPI_File, int*))
        unimpi_platform_dlsym(handle, "MPI_File_get_amode");

    /* MPI-3 Extended file operations */
    unimpi.file_get_info = (int (*)(MPI_File, MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_File_get_info");
    unimpi.file_set_info = (int (*)(MPI_File, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_File_set_info");
    unimpi.file_seek = (int (*)(MPI_File, MPI_Offset, int))
        unimpi_platform_dlsym(handle, "MPI_File_seek");
    unimpi.file_get_position = (int (*)(MPI_File, MPI_Offset*))
        unimpi_platform_dlsym(handle, "MPI_File_get_position");
    unimpi.file_get_byte_offset = (int (*)(MPI_File, MPI_Offset, MPI_Offset*))
        unimpi_platform_dlsym(handle, "MPI_File_get_byte_offset");

    unimpi.file_seek_shared = (int (*)(MPI_File, MPI_Offset, int))
        unimpi_platform_dlsym(handle, "MPI_File_seek_shared");
    unimpi.file_get_position_shared = (int (*)(MPI_File, MPI_Offset*))
        unimpi_platform_dlsym(handle, "MPI_File_get_position_shared");
    unimpi.file_get_type_extent = (int (*)(MPI_File, MPI_Datatype, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_File_get_type_extent");

    /* Parallel I/O - Read/Write */
    unimpi.file_read = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read");
    unimpi.file_read_all = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_all");
    unimpi.file_write = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write");
    unimpi.file_write_all = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_all");
    unimpi.file_read_at = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_at");
    unimpi.file_read_at_all = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_at_all");
    unimpi.file_write_at = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_at");
    unimpi.file_write_at_all = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_at_all");

    /* MPI-3 Extended file read/write */
    unimpi.file_read_shared = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_shared");
    unimpi.file_write_shared = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_shared");
    unimpi.file_read_ordered = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_ordered");
    unimpi.file_write_ordered = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_ordered");

    unimpi.file_read_all_begin = (int (*)(MPI_File, void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_File_read_all_begin");
    unimpi.file_read_all_end = (int (*)(MPI_File, void*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_all_end");
    unimpi.file_read_at_all_begin = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_File_read_at_all_begin");
    unimpi.file_read_at_all_end = (int (*)(MPI_File, void*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_at_all_end");
    unimpi.file_read_ordered_begin = (int (*)(MPI_File, void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_File_read_ordered_begin");
    unimpi.file_read_ordered_end = (int (*)(MPI_File, void*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_read_ordered_end");
    unimpi.file_write_all_begin = (int (*)(MPI_File, const void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_File_write_all_begin");
    unimpi.file_write_all_end = (int (*)(MPI_File, const void*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_all_end");
    unimpi.file_write_at_all_begin = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_File_write_at_all_begin");
    unimpi.file_write_at_all_end = (int (*)(MPI_File, const void*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_at_all_end");
    unimpi.file_write_ordered_begin = (int (*)(MPI_File, const void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_File_write_ordered_begin");
    unimpi.file_write_ordered_end = (int (*)(MPI_File, const void*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_File_write_ordered_end");

    /* Parallel I/O - Non-blocking */
    unimpi.file_iread = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_File_iread");
    unimpi.file_iwrite = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_File_iwrite");
    unimpi.file_iread_at = (int (*)(MPI_File, MPI_Offset, void*, int, MPI_Datatype, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_File_iread_at");
    unimpi.file_iwrite_at = (int (*)(MPI_File, MPI_Offset, const void*, int, MPI_Datatype, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_File_iwrite_at");

    unimpi.file_iread_shared = (int (*)(MPI_File, void*, int, MPI_Datatype, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_File_iread_shared");
    unimpi.file_iwrite_shared = (int (*)(MPI_File, const void*, int, MPI_Datatype, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_File_iwrite_shared");

    /* Parallel I/O - Views */
    unimpi.file_set_view = (int (*)(MPI_File, MPI_Offset, MPI_Datatype, MPI_Datatype, const char*, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_File_set_view");
    unimpi.file_get_view = (int (*)(MPI_File, MPI_Offset*, MPI_Datatype*, MPI_Datatype*, char*))
        unimpi_platform_dlsym(handle, "MPI_File_get_view");

    /* Dynamic Process Management */
    unimpi.comm_spawn = (int (*)(const char*, char*[], int, MPI_Info, int, MPI_Comm, MPI_Comm*, int[]))
        unimpi_platform_dlsym(handle, "MPI_Comm_spawn");
    /* Comm_spawn_multiple (wrapped: array_of_info input array conversion) */
    msmpi_comm_spawn_multiple = (int (*)(int, char*[], char**[],
            const int[], const int[], int, MPI_Comm, MPI_Comm*, int[]))
            unimpi_platform_dlsym(handle, "MPI_Comm_spawn_multiple");
    if (msmpi_comm_spawn_multiple) {
        unimpi.comm_spawn_multiple = msmpi_wrap_comm_spawn_multiple;
    }
    unimpi.comm_accept = (int (*)(const char*, MPI_Info, int, MPI_Comm, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_accept");
    unimpi.comm_connect = (int (*)(const char*, MPI_Info, int, MPI_Comm, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_connect");
    unimpi.comm_disconnect = (int (*)(MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_disconnect");

    /* comm_join */
    unimpi.comm_join = (int (*)(int, MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_join");
    unimpi.comm_get_parent = (int (*)(MPI_Comm*))
        unimpi_platform_dlsym(handle, "MPI_Comm_get_parent");

    /* Port and Name Service */
    unimpi.open_port = (int (*)(MPI_Info, char*))
        unimpi_platform_dlsym(handle, "MPI_Open_port");
    unimpi.close_port = (int (*)(const char*))
        unimpi_platform_dlsym(handle, "MPI_Close_port");
    unimpi.publish_name = (int (*)(const char*, MPI_Info, const char*))
        unimpi_platform_dlsym(handle, "MPI_Publish_name");
    unimpi.unpublish_name = (int (*)(const char*, MPI_Info, const char*))
        unimpi_platform_dlsym(handle, "MPI_Unpublish_name");
    unimpi.lookup_name = (int (*)(const char*, MPI_Info, char*))
        unimpi_platform_dlsym(handle, "MPI_Lookup_name");

    /* Info Operations */
    unimpi.info_create = (int (*)(MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_Info_create");
    unimpi.info_free = (int (*)(MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_Info_free");
    unimpi.info_set = (int (*)(MPI_Info, const char*, const char*))
        unimpi_platform_dlsym(handle, "MPI_Info_set");
    unimpi.info_get = (int (*)(MPI_Info, const char*, int, char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Info_get");
    unimpi.info_delete = (int (*)(MPI_Info, const char*))
        unimpi_platform_dlsym(handle, "MPI_Info_delete");
    unimpi.info_get_nkeys = (int (*)(MPI_Info, int*))
        unimpi_platform_dlsym(handle, "MPI_Info_get_nkeys");
    unimpi.info_get_nthkey = (int (*)(MPI_Info, int, char*))
        unimpi_platform_dlsym(handle, "MPI_Info_get_nthkey");

    /* MPI-2 base completeness additions (datatypes, address, p2p, info, etc.) */
    unimpi.ibsend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ibsend");
    unimpi.irsend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Irsend");
    unimpi.issend = (int (*)(const void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Issend");
    unimpi.request_get_status = (int (*)(MPI_Request, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Request_get_status");

    unimpi.grequest_start = (int (*)(MPI_Grequest_query_function*, MPI_Grequest_free_function*, MPI_Grequest_cancel_function*, void*, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Grequest_start");
    unimpi.grequest_complete = (int (*)(MPI_Request))
        unimpi_platform_dlsym(handle, "MPI_Grequest_complete");
    unimpi.reduce_local = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op))
        unimpi_platform_dlsym(handle, "MPI_Reduce_local");
    unimpi.info_dup = (int (*)(MPI_Info, MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_Info_dup");
    unimpi.info_get_valuelen = (int (*)(MPI_Info, const char*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Info_get_valuelen");
    unimpi.type_create_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_hvector");
    unimpi.type_create_hindexed = (int (*)(int, const int*, const MPI_Aint*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_hindexed");
    /* Type_create_struct / Type_struct (wrapped) */
    msmpi_type_create_struct = (int (*)(int, const int*, const MPI_Aint*,
            const int*, MPI_Datatype*))
            unimpi_platform_dlsym(handle, "MPI_Type_create_struct");
    if (msmpi_type_create_struct) {
        unimpi.type_create_struct = msmpi_wrap_type_create_struct;
    }
    msmpi_type_struct = (int (*)(int, const int*, const MPI_Aint*,
            const int*, MPI_Datatype*))
            unimpi_platform_dlsym(handle, "MPI_Type_struct");
    if (msmpi_type_struct) {
        unimpi.type_struct = msmpi_wrap_type_struct;
    }
        unimpi_platform_dlsym(handle, "MPI_Type_struct");
    unimpi.type_match_size = (int (*)(MPI_Datatype, int, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_match_size");
    unimpi.type_create_f90_integer = (int (*)(int, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_f90_integer");
    unimpi.type_create_f90_real = (int (*)(int, int, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_f90_real");
    unimpi.type_create_f90_complex = (int (*)(int, int, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_f90_complex");
    unimpi.get_address = (int (*)(const void*, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Get_address");
    unimpi.address = (int (*)(void*, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Address");


    /* Thread Support */
    unimpi.init_thread = (int (*)(int*, char***, int, int*))
        unimpi_platform_dlsym(handle, "MPI_Init_thread");
    unimpi.query_thread = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Query_thread");
    unimpi.is_thread_main = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Is_thread_main");

    /* Memory Allocation */
    unimpi.alloc_mem = (int (*)(MPI_Aint, MPI_Info, void*))
        unimpi_platform_dlsym(handle, "MPI_Alloc_mem");
    unimpi.free_mem = (int (*)(void*))
        unimpi_platform_dlsym(handle, "MPI_Free_mem");

    /* MPI-3 Reduction operations */
    unimpi.op_create = (int (*)(void (*)(void*, void*, int*, MPI_Datatype*), int, MPI_Op*))
        unimpi_platform_dlsym(handle, "MPI_Op_create");
    unimpi.op_free = (int (*)(MPI_Op*))
        unimpi_platform_dlsym(handle, "MPI_Op_free");
    /* op_commutative */
    unimpi.op_commutative = (int (*)(MPI_Op, int*))
        unimpi_platform_dlsym(handle, "MPI_Op_commutative");

    /* MPI-3 Status manipulation */
    unimpi.status_set_elements = (int (*)(MPI_Status*, MPI_Datatype, int))
        unimpi_platform_dlsym(handle, "MPI_Status_set_elements");
    unimpi.status_set_cancelled = (int (*)(MPI_Status*, int))
        unimpi_platform_dlsym(handle, "MPI_Status_set_cancelled");

    unimpi.status_f2c = (int (*)(const MPI_Fint*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Status_f2c");
    unimpi.status_c2f = (int (*)(const MPI_Status*, MPI_Fint*))
        unimpi_platform_dlsym(handle, "MPI_Status_c2f");

    /* MPI-3 Error handling */
    unimpi.errhandler_create = (int (*)(void (*)(MPI_Comm*, int*, ...), MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Errhandler_create");
    unimpi.errhandler_free = (int (*)(MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Errhandler_free");
    unimpi.errhandler_set = (int (*)(MPI_Comm, MPI_Errhandler))
        unimpi_platform_dlsym(handle, "MPI_Errhandler_set");
    unimpi.errhandler_get = (int (*)(MPI_Comm, MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Errhandler_get");

    unimpi.comm_get_errhandler = (int (*)(MPI_Comm, MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Comm_get_errhandler");
    unimpi.comm_set_errhandler = (int (*)(MPI_Comm, MPI_Errhandler))
        unimpi_platform_dlsym(handle, "MPI_Comm_set_errhandler");
    unimpi.win_get_errhandler = (int (*)(MPI_Win, MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Win_get_errhandler");
    unimpi.win_set_errhandler = (int (*)(MPI_Win, MPI_Errhandler))
        unimpi_platform_dlsym(handle, "MPI_Win_set_errhandler");

    unimpi.comm_create_errhandler = (int (*)(void (*)(MPI_Comm*, int*, ...), MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Comm_create_errhandler");
    unimpi.comm_call_errhandler = (int (*)(MPI_Comm, int))
        unimpi_platform_dlsym(handle, "MPI_Comm_call_errhandler");
    unimpi.win_create_errhandler = (int (*)(void (*)(MPI_Win*, int*, ...), MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_Win_create_errhandler");
    unimpi.win_create_keyval = (int (*)(MPI_Win_copy_attr_function*, MPI_Win_delete_attr_function*, int*, void*))
        unimpi_platform_dlsym(handle, "MPI_Win_create_keyval");
    unimpi.win_free_keyval = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Win_free_keyval");
    unimpi.win_set_attr = (int (*)(MPI_Win, int, void*))
        unimpi_platform_dlsym(handle, "MPI_Win_set_attr");
    unimpi.win_get_attr = (int (*)(MPI_Win, int, void*, int*))
        unimpi_platform_dlsym(handle, "MPI_Win_get_attr");
    unimpi.win_delete_attr = (int (*)(MPI_Win, int))
        unimpi_platform_dlsym(handle, "MPI_Win_delete_attr");
    unimpi.comm_create_keyval = (int (*)(MPI_Comm_copy_attr_function*, MPI_Comm_delete_attr_function*, int*, void*))
        unimpi_platform_dlsym(handle, "MPI_Comm_create_keyval");
    unimpi.comm_free_keyval = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_free_keyval");
    unimpi.comm_set_attr = (int (*)(MPI_Comm, int, void*))
        unimpi_platform_dlsym(handle, "MPI_Comm_set_attr");
    unimpi.comm_get_attr = (int (*)(MPI_Comm, int, void*, int*))
        unimpi_platform_dlsym(handle, "MPI_Comm_get_attr");
    unimpi.comm_delete_attr = (int (*)(MPI_Comm, int))
        unimpi_platform_dlsym(handle, "MPI_Comm_delete_attr");
    unimpi.type_create_keyval = (int (*)(MPI_Type_copy_attr_function*, MPI_Type_delete_attr_function*, int*, void*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_keyval");
    unimpi.type_free_keyval = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Type_free_keyval");
    unimpi.type_set_attr = (int (*)(MPI_Datatype, int, void*))
        unimpi_platform_dlsym(handle, "MPI_Type_set_attr");
    unimpi.type_get_attr = (int (*)(MPI_Datatype, int, void*, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_attr");
    unimpi.type_delete_attr = (int (*)(MPI_Datatype, int))
        unimpi_platform_dlsym(handle, "MPI_Type_delete_attr");
    unimpi.keyval_create = (int (*)(MPI_Copy_function*, MPI_Delete_function*, int*, void*))
        unimpi_platform_dlsym(handle, "MPI_Keyval_create");
    unimpi.keyval_free = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Keyval_free");
    unimpi.register_datarep = (int (*)(const char*, MPI_Datarep_conversion_function*, MPI_Datarep_conversion_function*, MPI_Datarep_extent_function*, void*))
        unimpi_platform_dlsym(handle, "MPI_Register_datarep");
    /* Predefined attribute callback values (MPICH-family: shared MPIR_Dup_fn). */
    MPI_COMM_DUP_FN = (MPI_Comm_copy_attr_function*)unimpi_platform_dlsym(handle, "MPIR_Dup_fn");
    MPI_TYPE_DUP_FN = (MPI_Type_copy_attr_function*)unimpi_platform_dlsym(handle, "MPIR_Dup_fn");
    MPI_WIN_DUP_FN = (MPI_Win_copy_attr_function*)unimpi_platform_dlsym(handle, "MPIR_Dup_fn");
    MPI_DUP_FN = (MPI_Copy_function*)unimpi_platform_dlsym(handle, "MPIR_Dup_fn");
    unimpi.win_get_group = (int (*)(MPI_Win, MPI_Group*))
        unimpi_platform_dlsym(handle, "MPI_Win_get_group");
    unimpi.win_call_errhandler = (int (*)(MPI_Win, int))
        unimpi_platform_dlsym(handle, "MPI_Win_call_errhandler");
    unimpi.file_create_errhandler = (int (*)(void (*)(MPI_File*, int*, ...), MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_File_create_errhandler");
    unimpi.file_set_atomicity = (int (*)(MPI_File, int))
        unimpi_platform_dlsym(handle, "MPI_File_set_atomicity");
    unimpi.file_get_atomicity = (int (*)(MPI_File, int*))
        unimpi_platform_dlsym(handle, "MPI_File_get_atomicity");
    unimpi.file_sync = (int (*)(MPI_File))
        unimpi_platform_dlsym(handle, "MPI_File_sync");
    unimpi.file_call_errhandler = (int (*)(MPI_File, int))
        unimpi_platform_dlsym(handle, "MPI_File_call_errhandler");
    unimpi.file_set_errhandler = (int (*)(MPI_File, MPI_Errhandler))
        unimpi_platform_dlsym(handle, "MPI_File_set_errhandler");
    unimpi.file_get_errhandler = (int (*)(MPI_File, MPI_Errhandler*))
        unimpi_platform_dlsym(handle, "MPI_File_get_errhandler");
    unimpi.add_error_class = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_Add_error_class");
    unimpi.add_error_code = (int (*)(int, int*))
        unimpi_platform_dlsym(handle, "MPI_Add_error_code");
    unimpi.add_error_string = (int (*)(int, const char*))
        unimpi_platform_dlsym(handle, "MPI_Add_error_string");

    unimpi.error_class = (int (*)(int, int*))
        unimpi_platform_dlsym(handle, "MPI_Error_class");
    unimpi.error_string = (int (*)(int, char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Error_string");
    unimpi.pcontrol = (int (*)(const int, ...))
        unimpi_platform_dlsym(handle, "MPI_Pcontrol");

    /* MPI-3 Attributes */
    unimpi.attr_put = (int (*)(MPI_Comm, int, void*))
        unimpi_platform_dlsym(handle, "MPI_Attr_put");
    unimpi.attr_get = (int (*)(MPI_Comm, int, void*, int*))
        unimpi_platform_dlsym(handle, "MPI_Attr_get");
    unimpi.attr_delete = (int (*)(MPI_Comm, int))
        unimpi_platform_dlsym(handle, "MPI_Attr_delete");

    /* Set MS-MPI specific communicator values */
    UNIMPI_COMM_WORLD = (MPI_Comm)0x44000000;
    UNIMPI_COMM_SELF = (MPI_Comm)0x44000001;

    /* MS-MPI uses MPICH-compatible request null value */
    UNIMPI_REQUEST_NULL = (MPI_Request)0x2c000000;

    /* MS-MPI defines both status-ignore sentinels as pointer value 1. */
    UNIMPI_STATUS_IGNORE = (MPI_Status *)(intptr_t)1;

    /* MS-MPI uses MPICH-compatible info null value */
    UNIMPI_INFO_NULL = (MPI_Info)0x1c000000;

    /* MS-MPI uses MPICH-compatible file/error-handler magic handles. */
    UNIMPI_FILE_NULL = (MPI_File)0;
    UNIMPI_ERRORS_ARE_FATAL = (MPI_Errhandler)0x54000000;
    UNIMPI_ERRORS_RETURN = (MPI_Errhandler)0x54000001;
    UNIMPI_ERRORS_ABORT = (MPI_Errhandler)0x54000003;

    /* Initialize MS-MPI error codes (same as MPICH) */
    init_msmpi_error_codes();

    /* Initialize topology type constants (MS-MPI: same as MPICH) */
    UNIMPI_GRAPH = 1;
    UNIMPI_CART = 2;
    UNIMPI_DIST_GRAPH = 3;

    return UNIMPI_OK;
}
