/* src/backends/openmpi.c */
#include <stdio.h>
#include "unimpi_vtable.h"
#include "unimpi_platform.h"
#include "unimpi.h"
#include "unimpi_mt.h"
#include "unimpi_errors.h"
#include "unimpi_mt_constants.h"

/* UniMPI status field accessors for the OpenMPI 24-byte status layout:
 * MPI_SOURCE/TAG/ERROR at offsets 0/4/8 (the vtable `base` member). See the
 * vtable comment on status_get_source. These match the standard
 * MPI_Status_get_* signature: return an MPI error code and write the field
 * through the out parameter. */
static int status_openmpi_get_source(const MPI_Status *status, int *source) {
    *source = status->base.MPI_SOURCE;
    return MPI_SUCCESS;
}
static int status_openmpi_get_tag(const MPI_Status *status, int *tag) {
    *tag = status->base.MPI_TAG;
    return MPI_SUCCESS;
}
static int status_openmpi_get_error(const MPI_Status *status, int *error) {
    *error = status->base.MPI_ERROR;
    return MPI_SUCCESS;
}

#if UNIMPI_MPI_AT_LEAST(3,0)
/* OpenMPI's MPI-2.2-era tools interface does not use the MPI-3.0 standard
 * signatures: MPI_T_cvar_get_info takes 10 args and MPI_T_pvar_get_info takes
 * 13 (no cvar_handle/var_extra, extra desc/bind/readonly/continuous/atomic
 * outputs, and a different argument order). The MT vtable exposes the
 * MPI-3.0 canonical 9/8-arg slots, so bridge the two here and drop the outputs
 * the canonical signature has no carrier for (cvar_handle, binding, var_extra).
 * The bound PMPI_* variants are resolved via the backend handle below. */
static int (*ompi_t_cvar_get_info_native)(int, char*, int*, int*, MPI_Datatype*,
    MPI_T_enum*, char*, int*, int*, int*) = NULL;
static int (*ompi_t_pvar_get_info_native)(int, char*, int*, int*, int*,
    MPI_Datatype*, MPI_T_enum*, char*, int*, int*, int*, int*, int*) = NULL;

static int ompi_bridge_cvar_get_info(int cvar_index, char *name, int *name_len,
    MPI_Datatype *datatype, MPI_T_enum *enumtype, MPI_T_cvar_handle *cvar_handle,
    int *verbosity, int *scope, void *var_extra) {
    int op_verb = 0, op_scope = 0, op_bind = 0, op_desc_len = 256;
    char op_desc[256] = {0};
    int rc = ompi_t_cvar_get_info_native(cvar_index, name, name_len,
        &op_verb, datatype, enumtype, op_desc, &op_desc_len,
        &op_bind, &op_scope);
    if (rc == 0) {
        if (verbosity) *verbosity = op_verb;
        if (scope) *scope = op_scope;
    }
    if (cvar_handle) *cvar_handle = 0;
    (void)var_extra;
    return rc;
}

static int ompi_bridge_pvar_get_info(int pvar_index, char *name, int *name_len,
    MPI_T_enum *enumtype, MPI_T_pvar_session *binding, int *verbosity,
    int *var_class, void *var_extra) {
    int op_verb = 0, op_vclass = 0, op_bind = 0, op_desc_len = 0;
    int op_ro = 0, op_cont = 0, op_atomic = 0;
    MPI_Datatype op_dt = 0;
    char op_desc[256] = {0};
    int rc = ompi_t_pvar_get_info_native(pvar_index, name, name_len,
        &op_verb, &op_vclass,
        &op_dt, enumtype, op_desc, &op_desc_len, &op_bind,
        &op_ro, &op_cont, &op_atomic);
    if (rc == 0) {
        if (verbosity) *verbosity = op_verb;
        if (var_class) *var_class = op_vclass;
    }
    if (binding) *binding = 0;
    (void)var_extra;
    return rc;
}
#endif /* UNIMPI_MPI_AT_LEAST(3,0) */

/* OpenMPI uses pointers for all opaque types */
typedef struct ompi_communicator_t* ompi_comm_t;
typedef struct ompi_datatype_t* ompi_datatype_t;
typedef struct ompi_op_t* ompi_op_t;
typedef struct ompi_request_t* ompi_request_t;
typedef struct ompi_info_t* ompi_info_t;
typedef struct ompi_group_t* ompi_group_t;
typedef struct ompi_win_t* ompi_win_t;
typedef struct ompi_file_t* ompi_file_t;

/* Initialize OpenMPI-specific error codes */
static void init_openmpi_error_codes(void) {
    /* OpenMPI error codes differ from MPICH standard */
    MPI_SUCCESS = 0;
    /* OpenMPI uses MPI_ANY_SOURCE = -1 (MPICH-family use -2). Set it to this
     * backend's value so wildcard-source operations always use the right
     * sentinel regardless of the process-wide default. MPI_ANY_TAG = -1 is
     * correct on all backends. */
    MPI_ANY_SOURCE = -1;
    /* OpenMPI also uses MPI_PROC_NULL = -2 and MPI_ROOT = -4 (MPICH-family
     * use -1/-3, which match the UniMPI defaults and need no override). The
     * MPI C standard requires a no-op for sends/ops targeting MPI_PROC_NULL,
     * so passing UniMPI's default -1 would instead collide with OpenMPI's
     * MPI_ANY_SOURCE and be rejected as an invalid rank. */
    MPI_PROC_NULL = -2;
    MPI_ROOT = -4;
    /* OpenMPI's in-place sentinel is (void*)1 (see its mpi.h). Pass the
     * loaded implementation's own value so MPI_Iallreduce(MPI_IN_PLACE, ...)
     * reaches the native call correctly. */
    MPI_IN_PLACE = (void *)1;
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
    MPI_ERR_IN_STATUS = 18;    /* OpenMPI: 18 */
    MPI_ERR_PENDING = 19;      /* OpenMPI: 19 */
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
    MPI_ERR_RMA_CONFLICT = 46;  /* OpenMPI specific */
    MPI_ERR_RMA_SYNC = 47;      /* OpenMPI specific */
    MPI_ERR_SERVICE = 48;       /* OpenMPI specific */
    MPI_ERR_SIZE = 49;          /* OpenMPI specific */
    MPI_ERR_SPAWN = 50;         /* OpenMPI specific */
    MPI_ERR_UNSUPPORTED_DATAREP = 51;      /* OpenMPI specific */
    MPI_ERR_UNSUPPORTED_OPERATION = 52;  /* OpenMPI specific */
    MPI_ERR_WIN = 53;           /* OpenMPI specific */
    /* MPI_ERR_PROC_FAILED not defined in OpenMPI system headers */
    /* MPI_ERR_PROC_FAIL_STOP not defined in OpenMPI */
    MPI_ERR_LASTCODE = 92;      /* OpenMPI 4.x defines 92 */
}

/* Helper: Get the address of an OpenMPI global symbol (for datatypes/ops)
 * For datatype/op symbols, the symbol itself is the pointer we need
 */
static intptr_t get_ompi_symbol_addr(unimpi_lib_handle_t handle, const char *symbol) {
    void *addr = unimpi_platform_dlsym(handle, symbol);
    if (addr) {
        return (intptr_t)addr;
    }
    return 0;
}

/* Get predefined communicator values from OpenMPI globals
 * Note: OpenMPI uses different symbol patterns across versions:
 * - Modern OpenMPI (≥1.7): Uses "_addr" suffix symbols that contain pointers
 * - Older OpenMPI (<1.7): Direct symbols are the pointer values
 */
static int get_openmpi_comm_values(unimpi_lib_handle_t handle) {
    intptr_t world = 0;
    intptr_t self = 0;

    /* Try _addr variants first (modern OpenMPI, initialized before MPI_Init) */
    void **world_addr = (void**)unimpi_platform_dlsym(handle, "ompi_mpi_comm_world_addr");
    void **self_addr = (void**)unimpi_platform_dlsym(handle, "ompi_mpi_comm_self_addr");

    if (world_addr && *world_addr) {
        world = (intptr_t)(*world_addr);
    }
    if (self_addr && *self_addr) {
        self = (intptr_t)(*self_addr);
    }

    /* Fallback to direct symbols for older OpenMPI versions */
    if (!world) {
        void *world_direct = unimpi_platform_dlsym(handle, "ompi_mpi_comm_world");
        if (world_direct) {
            world = (intptr_t)world_direct;
        }
    }
    if (!self) {
        void *self_direct = unimpi_platform_dlsym(handle, "ompi_mpi_comm_self");
        if (self_direct) {
            self = (intptr_t)self_direct;
        }
    }

    /* Validate that we got valid communicator values */
    if (!world || !self) {
        /* At least one communicator failed to load - this is a critical error */
        fprintf(stderr, "[unimpi:ERROR] Failed to load OpenMPI communicator symbols\n");
        fprintf(stderr, "  UNIMPI_COMM_WORLD: %s\n", world ? "OK" : "FAILED");
        fprintf(stderr, "  UNIMPI_COMM_SELF: %s\n", self ? "OK" : "FAILED");
        return UNIMPI_ERR_BACKEND_INIT_FAILED;
    }

    UNIMPI_COMM_WORLD = world;
    UNIMPI_COMM_SELF = self;

    /* Get MPI_REQUEST_NULL - it's the address of ompi_request_null global */
    UNIMPI_REQUEST_NULL = get_ompi_symbol_addr(handle, "ompi_request_null");

    /* Open MPI defines both status-ignore sentinels as NULL. */
    UNIMPI_STATUS_IGNORE = NULL;

    /* Get MPI_INFO_NULL */
    UNIMPI_INFO_NULL = get_ompi_symbol_addr(handle, "ompi_mpi_info_null");

    /* Get the predefined file handle and error handlers as symbol addresses
     * (OMPI_PREDEFINED_GLOBAL resolves each to &global). Required so that
     * MPI_File_set_errhandler(MPI_FILE_NULL, MPI_ERRORS_RETURN) reaches the
     * backend with handles it recognizes. */
    UNIMPI_FILE_NULL = get_ompi_symbol_addr(handle, "ompi_mpi_file_null");
    UNIMPI_ERRORS_ARE_FATAL = get_ompi_symbol_addr(handle, "ompi_mpi_errors_are_fatal");
    UNIMPI_ERRORS_RETURN = get_ompi_symbol_addr(handle, "ompi_mpi_errors_return");
    UNIMPI_ERRORS_ABORT = get_ompi_symbol_addr(handle, "ompi_mpi_errors_abort");

    return UNIMPI_OK;
}

/* Get predefined datatype values from OpenMPI globals
 * For datatypes, we use get_ompi_symbol_addr which returns the symbol address directly
 */
static int get_openmpi_datatype_values(unimpi_lib_handle_t handle) {
    int failed_count = 0;

    /* Table of datatype symbol mappings */
    static const struct {
        const char *symbol;
        MPI_Datatype *dest;
    } datatype_map[] = {
        {"ompi_mpi_char", &UNIMPI_CHAR},
        {"ompi_mpi_signed_char", &UNIMPI_SIGNED_CHAR},
        {"ompi_mpi_unsigned_char", &UNIMPI_UNSIGNED_CHAR},
        {"ompi_mpi_byte", &UNIMPI_BYTE},
        {"ompi_mpi_short", &UNIMPI_SHORT},
        {"ompi_mpi_unsigned_short", &UNIMPI_UNSIGNED_SHORT},
        {"ompi_mpi_int", &UNIMPI_INT},
        {"ompi_mpi_unsigned", &UNIMPI_UNSIGNED},
        {"ompi_mpi_long", &UNIMPI_LONG},
        {"ompi_mpi_unsigned_long", &UNIMPI_UNSIGNED_LONG},
        {"ompi_mpi_float", &UNIMPI_FLOAT},
        {"ompi_mpi_double", &UNIMPI_DOUBLE},
        {"ompi_mpi_long_double", &UNIMPI_LONG_DOUBLE},
        {"ompi_mpi_long_long_int", &UNIMPI_LONG_LONG_INT},
        {"ompi_mpi_unsigned_long_long", &UNIMPI_UNSIGNED_LONG_LONG},
    };

    /* Load all datatypes from table */
    for (size_t i = 0; i < sizeof(datatype_map) / sizeof(datatype_map[0]); i++) {
        *datatype_map[i].dest = get_ompi_symbol_addr(handle, datatype_map[i].symbol);
        if (!*datatype_map[i].dest) {
            failed_count++;
        }
    }

    /* LONG_LONG is an alias for LONG_LONG_INT */
    UNIMPI_LONG_LONG = UNIMPI_LONG_LONG_INT;

    if (failed_count > 0) {
        fprintf(stderr, "[unimpi:WARN] Failed to load %d OpenMPI datatype symbols\n", failed_count);
        /* Don't fail - some datatypes might not be critical */
    }

    return UNIMPI_OK;
}

/* Get predefined operation values from OpenMPI globals
 * For operations, we use get_ompi_symbol_addr which returns the symbol address directly
 */
static int get_openmpi_op_values(unimpi_lib_handle_t handle) {
    int failed_count = 0;

    /* Table of operation symbol mappings */
    static const struct {
        const char *symbol;
        MPI_Op *dest;
    } op_map[] = {
        {"ompi_mpi_op_max", &UNIMPI_MAX},
        {"ompi_mpi_op_min", &UNIMPI_MIN},
        {"ompi_mpi_op_sum", &UNIMPI_SUM},
        {"ompi_mpi_op_prod", &UNIMPI_PROD},
        {"ompi_mpi_op_land", &UNIMPI_LAND},
        {"ompi_mpi_op_band", &UNIMPI_BAND},
        {"ompi_mpi_op_lor", &UNIMPI_LOR},
        {"ompi_mpi_op_bor", &UNIMPI_BOR},
        {"ompi_mpi_op_lxor", &UNIMPI_LXOR},
        {"ompi_mpi_op_bxor", &UNIMPI_BXOR},
        {"ompi_mpi_op_minloc", &UNIMPI_MINLOC},
        {"ompi_mpi_op_maxloc", &UNIMPI_MAXLOC},
    };

    /* Load all operations from table */
    for (size_t i = 0; i < sizeof(op_map) / sizeof(op_map[0]); i++) {
        *op_map[i].dest = get_ompi_symbol_addr(handle, op_map[i].symbol);
        if (!*op_map[i].dest) {
            failed_count++;
        }
    }

    if (failed_count > 0) {
        fprintf(stderr, "[unimpi:WARN] Failed to load %d OpenMPI operation symbols\n", failed_count);
    }

    return UNIMPI_OK;
}


int unimpi_vtable_init_openmpi(unimpi_lib_handle_t handle) {
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
    unimpi.waitall = (int (*)(int, MPI_Request*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitall");
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
    unimpi.testany = (int (*)(int, MPI_Request*, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Testany");
    unimpi.testsome = (int (*)(int, MPI_Request*, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Testsome");
    unimpi.testall = (int (*)(int, MPI_Request*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Testall");
    unimpi.waitany = (int (*)(int, MPI_Request*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitany");
    unimpi.waitsome = (int (*)(int, MPI_Request*, int*, int*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Waitsome");

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
    unimpi.startall = (int (*)(int, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Startall");
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
    /* alltoallw */
    unimpi.alltoallw = (int (*)(const void*, const int*, const int*, const MPI_Datatype*, void*, const int*, const int*, const MPI_Datatype*, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Alltoallw");

    /* Collective - Reduce-scatter and scan */
    unimpi.reduce_scatter = (int (*)(const void*, void*, const int*, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Reduce_scatter");
    unimpi.reduce_scatter_block = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Reduce_scatter_block");
    unimpi.scan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Scan");
    unimpi.exscan = (int (*)(const void*, void*, int, MPI_Datatype, MPI_Op, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Exscan");

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

    /* Datatypes - Creation */
    unimpi.type_commit = (int (*)(MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_commit");
    unimpi.type_free = (int (*)(MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_free");
    unimpi.type_contiguous = (int (*)(int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_contiguous");
    unimpi.type_vector = (int (*)(int, int, int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_vector");
    unimpi.type_hvector = (int (*)(int, int, MPI_Aint, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_hvector");
    unimpi.type_indexed = (int (*)(int, const int*, const int*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_indexed");
    unimpi.type_hindexed = (int (*)(int, const int*, const MPI_Aint*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_hindexed");
    unimpi.type_create_indexed_block = (int (*)(int, int, const int*, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_indexed_block");
    unimpi.type_create_subarray = (int (*)(int, const int*, const int*, const int*, int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_subarray");
    unimpi.type_create_darray = (int (*)(int, int, int, const int*, const int*, const int*, const int*, int, MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_darray");
    unimpi.type_dup = (int (*)(MPI_Datatype, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_dup");

    /* MPI-2.2 Extended datatypes */
    unimpi.type_create_resized = (int (*)(MPI_Datatype, MPI_Aint, MPI_Aint, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_resized");
    unimpi.type_get_envelope = (int (*)(MPI_Datatype, int*, int*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_envelope");
    unimpi.type_get_contents = (int (*)(MPI_Datatype, int, int, int, int*, MPI_Aint*, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_contents");

    /* Datatypes - Query */
    unimpi.type_get_extent = (int (*)(MPI_Datatype, MPI_Aint*, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_extent");
    unimpi.type_get_true_extent = (int (*)(MPI_Datatype, MPI_Aint*, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_true_extent");
    unimpi.type_get_size = (int (*)(MPI_Datatype, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_size");
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
    unimpi.type_get_name = (int (*)(MPI_Datatype, char*, int*))
        unimpi_platform_dlsym(handle, "MPI_Type_get_name");
    unimpi.type_set_name = (int (*)(MPI_Datatype, const char*))
        unimpi_platform_dlsym(handle, "MPI_Type_set_name");
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
    unimpi.pack_external = (int (*)(const char*, const void*, int, MPI_Datatype, void*, MPI_Aint, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Pack_external");
    unimpi.unpack_external = (int (*)(const char*, const void*, MPI_Aint, MPI_Aint*, void*, int, MPI_Datatype))
        unimpi_platform_dlsym(handle, "MPI_Unpack_external");
    unimpi.pack_external_size = (int (*)(const char*, int, MPI_Datatype, MPI_Aint*))
        unimpi_platform_dlsym(handle, "MPI_Pack_external_size");

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
    unimpi.ialltoallw = (int (*)(const void*, const int*, const int*, const MPI_Datatype*, void*, const int*, const int*, const MPI_Datatype*, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ialltoallw");
    unimpi.ialltoallw = (int (*)(const void*, const int*, const int*, const MPI_Datatype*, void*, const int*, const int*, const MPI_Datatype*, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ialltoallw");
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
    unimpi.neighbor_alltoallv = (int (*)(const void*, const int*, const MPI_Aint*, const MPI_Datatype*, void*, const int*, const MPI_Aint*, const MPI_Datatype*, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Neighbor_alltoallv");
    unimpi.neighbor_alltoallw = (int (*)(const void*, const int*, const MPI_Aint*, const MPI_Datatype*, void*, const int*, const MPI_Aint*, const MPI_Datatype*, MPI_Comm))
        unimpi_platform_dlsym(handle, "MPI_Neighbor_alltoallw");
    unimpi.ineighbor_allgather = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_allgather");
    unimpi.ineighbor_allgatherv = (int (*)(const void*, int, MPI_Datatype, void*, const int*, const int*, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_allgatherv");
    unimpi.ineighbor_alltoall = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_alltoall");
    unimpi.ineighbor_alltoallv = (int (*)(const void*, const int*, const MPI_Aint*, const MPI_Datatype*, void*, const int*, const MPI_Aint*, const MPI_Datatype*, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_alltoallv");
    unimpi.ineighbor_alltoallw = (int (*)(const void*, const int*, const MPI_Aint*, const MPI_Datatype*, void*, const int*, const MPI_Aint*, const MPI_Datatype*, MPI_Comm, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Ineighbor_alltoallw");
#endif

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

    /* RMA - Window creation */
    unimpi.win_create = (int (*)(void*, MPI_Aint, int, MPI_Info, MPI_Comm, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_create");
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 win_alloc_shared */
    unimpi.win_allocate = (int (*)(MPI_Aint, int, MPI_Info, MPI_Comm, void*, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_allocate");
    unimpi.win_allocate_shared = (int (*)(MPI_Aint, int, MPI_Info, MPI_Comm, void*, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_allocate_shared");
    unimpi.win_create_dynamic = (int (*)(MPI_Info, MPI_Comm, MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_create_dynamic");
#endif
    unimpi.win_free = (int (*)(MPI_Win*))
        unimpi_platform_dlsym(handle, "MPI_Win_free");
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
    unimpi.raccumulate = (int (*)(const void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Raccumulate");
    unimpi.rget_accumulate = (int (*)(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, int, MPI_Aint, int, MPI_Datatype, MPI_Op, MPI_Win, MPI_Request*))
        unimpi_platform_dlsym(handle, "MPI_Rget_accumulate");
#endif

    /* RMA Synchronization */
    unimpi.win_fence = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_fence");
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
    unimpi.win_flush_local = (int (*)(int, MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_flush_local");
    unimpi.win_sync = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_sync");
#endif
#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 win_dynamic */
    unimpi.win_attach = (int (*)(MPI_Win, void*, MPI_Aint))
        unimpi_platform_dlsym(handle, "MPI_Win_attach");
    unimpi.win_detach = (int (*)(MPI_Win, void*))
        unimpi_platform_dlsym(handle, "MPI_Win_detach");
    unimpi.win_shared_query = (int (*)(MPI_Win, int, MPI_Aint*, int*, void**))
        unimpi_platform_dlsym(handle, "MPI_Win_shared_query");
    unimpi.win_flush_local_all = (int (*)(MPI_Win))
        unimpi_platform_dlsym(handle, "MPI_Win_flush_local_all");
    unimpi.win_get_info = (int (*)(MPI_Win, MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_Win_get_info");
    unimpi.win_set_info = (int (*)(MPI_Win, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_Win_set_info");
#endif

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
    unimpi.file_get_info = (int (*)(MPI_File, MPI_Info*))
        unimpi_platform_dlsym(handle, "MPI_File_get_info");
    unimpi.file_set_info = (int (*)(MPI_File, MPI_Info))
        unimpi_platform_dlsym(handle, "MPI_File_set_info");

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
    unimpi.comm_spawn_multiple = (int (*)(int, char*[], char**[], const int[], const MPI_Info[], int, MPI_Comm, MPI_Comm*, int[]))
        unimpi_platform_dlsym(handle, "MPI_Comm_spawn_multiple");
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
    unimpi.type_create_struct = (int (*)(int, const int*, const MPI_Aint*, const MPI_Datatype*, MPI_Datatype*))
        unimpi_platform_dlsym(handle, "MPI_Type_create_struct");
    unimpi.type_struct = (int (*)(int, const int*, const MPI_Aint*, const MPI_Datatype*, MPI_Datatype*))
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

    /* Memory Allocation */
    unimpi.alloc_mem = (int (*)(MPI_Aint, MPI_Info, void*))
        unimpi_platform_dlsym(handle, "MPI_Alloc_mem");
    unimpi.free_mem = (int (*)(void*))
        unimpi_platform_dlsym(handle, "MPI_Free_mem");

    /* Reduction operations */
    unimpi.op_create = (int (*)(void (*)(void*, void*, int*, MPI_Datatype*), int, MPI_Op*))
        unimpi_platform_dlsym(handle, "MPI_Op_create");
    unimpi.op_free = (int (*)(MPI_Op*))
        unimpi_platform_dlsym(handle, "MPI_Op_free");
    /* op_commutative */
    unimpi.op_commutative = (int (*)(MPI_Op, int*))
        unimpi_platform_dlsym(handle, "MPI_Op_commutative");

    /* Status manipulation */
    unimpi.status_set_elements = (int (*)(MPI_Status*, MPI_Datatype, int))
        unimpi_platform_dlsym(handle, "MPI_Status_set_elements");
    unimpi.status_set_cancelled = (int (*)(MPI_Status*, int))
        unimpi_platform_dlsym(handle, "MPI_Status_set_cancelled");

    unimpi.status_f2c = (int (*)(const MPI_Fint*, MPI_Status*))
        unimpi_platform_dlsym(handle, "MPI_Status_f2c");
    unimpi.status_c2f = (int (*)(const MPI_Status*, MPI_Fint*))
        unimpi_platform_dlsym(handle, "MPI_Status_c2f");

    unimpi.status_get_source = status_openmpi_get_source;
    unimpi.status_get_tag = status_openmpi_get_tag;
    unimpi.status_get_error = status_openmpi_get_error;

    /* Error handling */
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
    /* Predefined attribute callback values (real OMPI_C_MPI_*_FN functions). */
    MPI_COMM_DUP_FN = (MPI_Comm_copy_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_COMM_DUP_FN");
    MPI_COMM_NULL_COPY_FN = (MPI_Comm_copy_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_COMM_NULL_COPY_FN");
    MPI_COMM_NULL_DELETE_FN = (MPI_Comm_delete_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_COMM_NULL_DELETE_FN");
    MPI_TYPE_DUP_FN = (MPI_Type_copy_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_TYPE_DUP_FN");
    MPI_TYPE_NULL_COPY_FN = (MPI_Type_copy_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_TYPE_NULL_COPY_FN");
    MPI_TYPE_NULL_DELETE_FN = (MPI_Type_delete_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_TYPE_NULL_DELETE_FN");
    MPI_WIN_DUP_FN = (MPI_Win_copy_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_WIN_DUP_FN");
    MPI_WIN_NULL_COPY_FN = (MPI_Win_copy_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_WIN_NULL_COPY_FN");
    MPI_WIN_NULL_DELETE_FN = (MPI_Win_delete_attr_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_WIN_NULL_DELETE_FN");
    MPI_DUP_FN = (MPI_Copy_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_DUP_FN");
    MPI_NULL_COPY_FN = (MPI_Copy_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_NULL_COPY_FN");
    MPI_NULL_DELETE_FN = (MPI_Delete_function*)unimpi_platform_dlsym(handle, "OMPI_C_MPI_NULL_DELETE_FN");
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

    /* Attributes */
    unimpi.attr_put = (int (*)(MPI_Comm, int, void*))
        unimpi_platform_dlsym(handle, "MPI_Attr_put");
    unimpi.attr_get = (int (*)(MPI_Comm, int, void*, int*))
        unimpi_platform_dlsym(handle, "MPI_Attr_get");
    unimpi.attr_delete = (int (*)(MPI_Comm, int))
        unimpi_platform_dlsym(handle, "MPI_Attr_delete");

    /* Get predefined values from OpenMPI globals */
    int ret;
    ret = get_openmpi_comm_values(handle);
    if (ret != UNIMPI_OK) {
        return ret;
    }
    ret = get_openmpi_datatype_values(handle);
    if (ret != UNIMPI_OK) {
        return ret;
    }
    ret = get_openmpi_op_values(handle);
    if (ret != UNIMPI_OK) {
        return ret;
    }

    /* Initialize OpenMPI-specific error codes */
    init_openmpi_error_codes();

    /* Initialize topology type constants (OpenMPI: MPI_CART=1, MPI_GRAPH=2) */
    UNIMPI_CART = 1;
    UNIMPI_GRAPH = 2;
    UNIMPI_DIST_GRAPH = 3;

#if UNIMPI_MPI_AT_LEAST(3,0)
    /* MPI-3.0 mpi_t_tools */
    /* Fall back to NULL (whole slot stays NULL) for any symbol the backend
     * does not export; the *_available() helper gates on that. Symbol
     * spellings follow the backend mpi.h. */
    unimpi_mt.t_init_thread = (int (*)(int, int*))
        unimpi_platform_dlsym(handle, "MPI_T_init_thread");
    unimpi_mt.t_finalize = (int (*)(void))
        unimpi_platform_dlsym(handle, "MPI_T_finalize");
    unimpi_mt.t_cvar_get_num = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_get_num");
    unimpi_mt.t_cvar_get_index = (int (*)(const char*, int*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_get_index");
    ompi_t_cvar_get_info_native = (int (*)(int, char*, int*, int*, MPI_Datatype*,
        MPI_T_enum*, char*, int*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_get_info");
    unimpi_mt.t_cvar_get_info = (int (*)(int, char*, int*, MPI_Datatype*,
                                         MPI_T_enum*, MPI_T_cvar_handle*,
                                         int*, int*, void*))ompi_bridge_cvar_get_info;
    unimpi_mt.t_cvar_handle_alloc = (int (*)(int, void*, MPI_T_cvar_handle*, int*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_handle_alloc");
    unimpi_mt.t_cvar_handle_free = (int (*)(MPI_T_cvar_handle*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_handle_free");
    unimpi_mt.t_cvar_read = (int (*)(MPI_T_cvar_handle, void*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_read");
    unimpi_mt.t_cvar_read_index = (int (*)(int, void*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_read_index");
    unimpi_mt.t_cvar_write = (int (*)(MPI_T_cvar_handle, const void*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_write");
    unimpi_mt.t_cvar_write_index = (int (*)(int, const void*))
        unimpi_platform_dlsym(handle, "MPI_T_cvar_write_index");
    unimpi_mt.t_pvar_get_num = (int (*)(int*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_get_num");
    unimpi_mt.t_pvar_get_index = (int (*)(const char*, int, int*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_get_index");
    ompi_t_pvar_get_info_native = (int (*)(int, char*, int*, int*, int*,
        MPI_Datatype*, MPI_T_enum*, char*, int*, int*, int*, int*, int*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_get_info");
    unimpi_mt.t_pvar_get_info = (int (*)(int, char*, int*, MPI_T_enum*,
                                         MPI_T_pvar_session*, int*, int*, void*))ompi_bridge_pvar_get_info;
    unimpi_mt.t_pvar_session_create = (int (*)(MPI_T_pvar_session*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_session_create");
    unimpi_mt.t_pvar_session_free = (int (*)(MPI_T_pvar_session*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_session_free");
    unimpi_mt.t_pvar_handle_alloc = (int (*)(MPI_T_pvar_session, int,
                                             MPI_T_handle, MPI_T_pvar_handle*, int*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_handle_alloc");
    unimpi_mt.t_pvar_handle_free = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_handle_free");
    unimpi_mt.t_pvar_start = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_start");
    unimpi_mt.t_pvar_stop = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_stop");
    unimpi_mt.t_pvar_read = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle, void*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_read");
    unimpi_mt.t_pvar_write = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle, const void*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_write");
    unimpi_mt.t_pvar_readreset = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle, void*))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_readreset");
    unimpi_mt.t_pvar_reset = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_reset");
    unimpi_mt.t_pvar_aggregate = (int (*)(MPI_T_pvar_session, MPI_T_pvar_handle))
        unimpi_platform_dlsym(handle, "MPI_T_pvar_aggregate");

    /* MPI_T_ERR_* error codes (from
     * /usr/lib/x86_64-linux-gnu/openmpi/include/mpi.h; OpenMPI omits NOT_SUPPORTED
     * and INVALID_ENUM, which stay 0). */
    MPI_T_ERR_MEMORY = 54;
    MPI_T_ERR_NOT_INITIALIZED = 55;
    MPI_T_ERR_CANNOT_INIT = 56;
    MPI_T_ERR_INVALID_INDEX = 57;
    MPI_T_ERR_INVALID_ITEM = 58;
    MPI_T_ERR_INVALID_HANDLE = 59;
    MPI_T_ERR_OUT_OF_HANDLES = 60;
    MPI_T_ERR_OUT_OF_SESSIONS = 61;
    MPI_T_ERR_INVALID_SESSION = 62;
    MPI_T_ERR_CVAR_SET_NOT_NOW = 63;
    MPI_T_ERR_PVAR_NO_STARTSTOP = 65;
    MPI_T_ERR_PVAR_NO_WRITE = 66;
    MPI_T_ERR_PVAR_NO_ATOMIC = 67;
    MPI_T_ERR_INVALID = 72;
    MPI_T_ERR_INVALID_NAME = 73;

    /* UNIMPI_T_* enums: OpenMPI anonymous enums start at 0 and increment
     * without an *_INVALID sentinel in the anonymous list */
    UNIMPI_T_VERBOSITY_USER_BASIC = 0; /* 0..8 */
    UNIMPI_T_VERBOSITY_USER_DETAIL = 1;
    UNIMPI_T_VERBOSITY_USER_ALL = 2;
    UNIMPI_T_VERBOSITY_TUNER_BASIC = 3;
    UNIMPI_T_VERBOSITY_TUNER_DETAIL = 4;
    UNIMPI_T_VERBOSITY_TUNER_ALL = 5;
    UNIMPI_T_VERBOSITY_MPIDEV_BASIC = 6;
    UNIMPI_T_VERBOSITY_MPIDEV_DETAIL = 7;
    UNIMPI_T_VERBOSITY_MPIDEV_ALL = 8;
    UNIMPI_T_SCOPE_CONSTANT = 0; /* 0..6 */
    UNIMPI_T_SCOPE_READONLY = 1;
    UNIMPI_T_SCOPE_LOCAL = 2;
    UNIMPI_T_SCOPE_GROUP = 3;
    UNIMPI_T_SCOPE_GROUP_EQ = 4;
    UNIMPI_T_SCOPE_ALL = 5;
    UNIMPI_T_SCOPE_ALL_EQ = 6;
    UNIMPI_T_BIND_NO_OBJECT = 0; /* 0..10 */
    UNIMPI_T_BIND_MPI_COMM = 1;
    UNIMPI_T_BIND_MPI_DATATYPE = 2;
    UNIMPI_T_BIND_MPI_ERRHANDLER = 3;
    UNIMPI_T_BIND_MPI_FILE = 4;
    UNIMPI_T_BIND_MPI_GROUP = 5;
    UNIMPI_T_BIND_MPI_OP = 6;
    UNIMPI_T_BIND_MPI_REQUEST = 7;
    UNIMPI_T_BIND_MPI_WIN = 8;
    UNIMPI_T_BIND_MPI_MESSAGE = 9;
    UNIMPI_T_BIND_MPI_INFO = 10;
    UNIMPI_T_PVAR_CLASS_STATE = 0; /* 0..9 */
    UNIMPI_T_PVAR_CLASS_LEVEL = 1;
    UNIMPI_T_PVAR_CLASS_SIZE = 2;
    UNIMPI_T_PVAR_CLASS_PERCENTAGE = 3;
    UNIMPI_T_PVAR_CLASS_HIGHWATERMARK = 4;
    UNIMPI_T_PVAR_CLASS_LOWWATERMARK = 5;
    UNIMPI_T_PVAR_CLASS_COUNTER = 6;
    UNIMPI_T_PVAR_CLASS_AGGREGATE = 7;
    UNIMPI_T_PVAR_CLASS_TIMER = 8;
    UNIMPI_T_PVAR_CLASS_GENERIC = 9;
    /* OpenMPI has no SOURCE_* / *_INVALID / *_NULL constants; leave 0. */
#endif

    return UNIMPI_OK;
}
