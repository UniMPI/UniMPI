#include "unimpi.h"
#include "unimpi_vtable.h"
#include <stdio.h>
#include <stdarg.h>

/* Global debug level */
static int g_debug_level = UNIMPI_DBG_NONE;

/* MPI Error Code Globals - initialized by backend */
int MPI_SUCCESS = 0;
int MPI_ERR_BUFFER = 1;
int MPI_ERR_COUNT = 2;
int MPI_ERR_TYPE = 3;
int MPI_ERR_TAG = 4;
int MPI_ERR_COMM = 5;
int MPI_ERR_RANK = 6;
int MPI_ERR_REQUEST = 7;
int MPI_ERR_ROOT = 8;
int MPI_ERR_GROUP = 9;
int MPI_ERR_OP = 10;
int MPI_ERR_TOPOLOGY = 11;
int MPI_ERR_DIMS = 12;
int MPI_ERR_ARG = 13;
int MPI_ERR_UNKNOWN = 14;
int MPI_ERR_TRUNCATE = 15;
int MPI_ERR_OTHER = 16;
int MPI_ERR_INTERN = 17;
int MPI_ERR_IN_STATUS = 18;
int MPI_ERR_PENDING = 19;
int MPI_ERR_ACCESS = 20;
int MPI_ERR_AMODE = 21;
int MPI_ERR_ASSERT = 22;
int MPI_ERR_BAD_FILE = 23;
int MPI_ERR_BASE = 24;
int MPI_ERR_CONVERSION = 25;
int MPI_ERR_DISP = 26;
int MPI_ERR_DUP_DATAREP = 27;
int MPI_ERR_FILE_EXISTS = 28;
int MPI_ERR_FILE_IN_USE = 29;
int MPI_ERR_FILE = 30;
int MPI_ERR_INFO_KEY = 31;
int MPI_ERR_INFO_NOKEY = 32;
int MPI_ERR_INFO_VALUE = 33;
int MPI_ERR_INFO = 34;
int MPI_ERR_IO = 35;
int MPI_ERR_KEYVAL = 36;
int MPI_ERR_LOCKTYPE = 37;
int MPI_ERR_NAME = 38;
int MPI_ERR_NO_MEM = 39;
int MPI_ERR_NOT_SAME = 40;
int MPI_ERR_NO_SPACE = 41;
int MPI_ERR_NO_SUCH_FILE = 42;
int MPI_ERR_PORT = 43;
int MPI_ERR_QUOTA = 44;
int MPI_ERR_READ_ONLY = 45;
int MPI_ERR_RMA_CONFLICT = 46;
int MPI_ERR_RMA_SYNC = 47;
int MPI_ERR_SERVICE = 48;
int MPI_ERR_SIZE = 49;
int MPI_ERR_SPAWN = 50;
int MPI_ERR_UNSUPPORTED_DATAREP = 51;
int MPI_ERR_UNSUPPORTED_OPERATION = 52;
int MPI_ERR_WIN = 53;
int MPI_ERR_PROC_FAILED = 54;
int MPI_ERR_PROC_FAIL_STOP = 55;
int MPI_ERR_LASTCODE = 56;

/* Thread levels */
int MPI_THREAD_SINGLE = 0;
int MPI_THREAD_FUNNELED = 1;
int MPI_THREAD_SERIALIZED = 2;
int MPI_THREAD_MULTIPLE = 3;

/* File operation constants */
int MPI_MODE_RDONLY = 1;
int MPI_MODE_RDWR = 2;
int MPI_MODE_WRONLY = 4;
int MPI_MODE_CREATE = 8;
int MPI_MODE_EXCL = 16;
int MPI_MODE_DELETE_ON_CLOSE = 32;
int MPI_MODE_UNIQUE_OPEN = 64;
int MPI_MODE_APPEND = 128;
int MPI_MODE_SEQUENTIAL = 256;

/* Seek constants */
int MPI_SEEK_SET = 100;
int MPI_SEEK_CUR = 101;
int MPI_SEEK_END = 102;

/* Order constants */
int MPI_ORDER_C = 100;
int MPI_ORDER_FORTRAN = 101;

/* Distribution constants */
int MPI_DISTRIBUTE_BLOCK = 100;
int MPI_DISTRIBUTE_CYCLIC = 101;
int MPI_DISTRIBUTE_NONE = 102;
int MPI_DISTRIBUTE_DFLT_DARG = -1;

/* Window constants */
int MPI_WIN_FLAVOR_CREATE = 1;
int MPI_WIN_FLAVOR_ALLOCATE = 2;
int MPI_WIN_FLAVOR_DYNAMIC = 3;
int MPI_WIN_FLAVOR_SHARED = 4;
int MPI_WIN_SEPARATE = 1;
int MPI_WIN_UNIFIED = 0;

/* Lock types */
int MPI_LOCK_EXCLUSIVE = 1;
int MPI_LOCK_SHARED = 2;

/* Compare results */
int MPI_IDENT = 0;
int MPI_CONGRUENT = 1;
int MPI_SIMILAR = 2;
int MPI_UNEQUAL = 3;

/* Special constants */
int MPI_ANY_SOURCE = -1;
int MPI_ANY_TAG = -1;
int MPI_UNDEFINED = -32766;
int MPI_TAG_UB = 0;
int MPI_HOST = 1;
int MPI_IO = 2;
int MPI_WTIME_IS_GLOBAL = 3;
int MPI_BSEND_OVERHEAD = 0;
int MPI_PROC_NULL = -1;
int MPI_ROOT = -3;

/* Status field indices */
int MPI_STATUS_SIZE = 5;
int MPI_SOURCE = 0;
int MPI_TAG = 1;
int MPI_ERROR = 2;

/* Communicator split types */
int MPI_COMM_TYPE_SHARED = 1;
int MPI_COMM_TYPE_HW_UNGUIDED = 2;
int MPI_COMM_TYPE_HW_THREAD = 3;

/* Maximum values - set to common defaults, backends may override */
int MPI_MAX_PROCESSOR_NAME = 256;
int MPI_MAX_LIBRARY_VERSION_STRING = 256;
int MPI_MAX_ERROR_STRING = 256;
int MPI_MAX_INFO_KEY = 255;
int MPI_MAX_INFO_VAL = 1024;
int MPI_MAX_OBJECT_NAME = 128;
int MPI_MAX_PORT_NAME = 256;
int MPI_MAX_DATAREP_STRING = 128;

/* Initialize error codes for specific backend
 * Called during vtable_init for each backend
 */
int unimpi_init_error_codes(unimpi_backend_type_t backend_type) {
    switch (backend_type) {
        case UNIMPI_BACKEND_OPENMPI:
            /* OpenMPI uses standard error class values */
            /* Most error codes match standard, but some differ */
            MPI_ERR_IN_STATUS = 17;    /* OpenMPI specific */
            MPI_ERR_PENDING = 18;      /* OpenMPI specific */
            break;

        case UNIMPI_BACKEND_MPICH:
        case UNIMPI_BACKEND_INTELMPI:
        case UNIMPI_BACKEND_MSMPI:
        default:
            /* Legacy backends use standard values */
            /* Already set to defaults above */
            break;
    }
    return UNIMPI_OK;
}

const char* unimpi_mpi_error_string(int error_code) {
    /* Map MPI error codes to strings using runtime values */
    if (error_code == MPI_SUCCESS) return "MPI_SUCCESS";
    if (error_code == MPI_ERR_BUFFER) return "MPI_ERR_BUFFER";
    if (error_code == MPI_ERR_COUNT) return "MPI_ERR_COUNT";
    if (error_code == MPI_ERR_TYPE) return "MPI_ERR_TYPE";
    if (error_code == MPI_ERR_TAG) return "MPI_ERR_TAG";
    if (error_code == MPI_ERR_COMM) return "MPI_ERR_COMM";
    if (error_code == MPI_ERR_RANK) return "MPI_ERR_RANK";
    if (error_code == MPI_ERR_REQUEST) return "MPI_ERR_REQUEST";
    if (error_code == MPI_ERR_ROOT) return "MPI_ERR_ROOT";
    if (error_code == MPI_ERR_GROUP) return "MPI_ERR_GROUP";
    if (error_code == MPI_ERR_OP) return "MPI_ERR_OP";
    if (error_code == MPI_ERR_TOPOLOGY) return "MPI_ERR_TOPOLOGY";
    if (error_code == MPI_ERR_DIMS) return "MPI_ERR_DIMS";
    if (error_code == MPI_ERR_ARG) return "MPI_ERR_ARG";
    if (error_code == MPI_ERR_UNKNOWN) return "MPI_ERR_UNKNOWN";
    if (error_code == MPI_ERR_TRUNCATE) return "MPI_ERR_TRUNCATE";
    if (error_code == MPI_ERR_OTHER) return "MPI_ERR_OTHER";
    if (error_code == MPI_ERR_INTERN) return "MPI_ERR_INTERN";
    if (error_code == MPI_ERR_IN_STATUS) return "MPI_ERR_IN_STATUS";
    if (error_code == MPI_ERR_PENDING) return "MPI_ERR_PENDING";
    if (error_code == MPI_ERR_ACCESS) return "MPI_ERR_ACCESS";
    if (error_code == MPI_ERR_AMODE) return "MPI_ERR_AMODE";
    if (error_code == MPI_ERR_ASSERT) return "MPI_ERR_ASSERT";
    if (error_code == MPI_ERR_BAD_FILE) return "MPI_ERR_BAD_FILE";
    if (error_code == MPI_ERR_BASE) return "MPI_ERR_BASE";
    if (error_code == MPI_ERR_CONVERSION) return "MPI_ERR_CONVERSION";
    if (error_code == MPI_ERR_DISP) return "MPI_ERR_DISP";
    if (error_code == MPI_ERR_DUP_DATAREP) return "MPI_ERR_DUP_DATAREP";
    if (error_code == MPI_ERR_FILE_EXISTS) return "MPI_ERR_FILE_EXISTS";
    if (error_code == MPI_ERR_FILE_IN_USE) return "MPI_ERR_FILE_IN_USE";
    if (error_code == MPI_ERR_FILE) return "MPI_ERR_FILE";
    if (error_code == MPI_ERR_INFO_KEY) return "MPI_ERR_INFO_KEY";
    if (error_code == MPI_ERR_INFO_NOKEY) return "MPI_ERR_INFO_NOKEY";
    if (error_code == MPI_ERR_INFO_VALUE) return "MPI_ERR_INFO_VALUE";
    if (error_code == MPI_ERR_INFO) return "MPI_ERR_INFO";
    if (error_code == MPI_ERR_IO) return "MPI_ERR_IO";
    if (error_code == MPI_ERR_KEYVAL) return "MPI_ERR_KEYVAL";
    if (error_code == MPI_ERR_LOCKTYPE) return "MPI_ERR_LOCKTYPE";
    if (error_code == MPI_ERR_NAME) return "MPI_ERR_NAME";
    if (error_code == MPI_ERR_NO_MEM) return "MPI_ERR_NO_MEM";
    if (error_code == MPI_ERR_NOT_SAME) return "MPI_ERR_NOT_SAME";
    if (error_code == MPI_ERR_NO_SPACE) return "MPI_ERR_NO_SPACE";
    if (error_code == MPI_ERR_NO_SUCH_FILE) return "MPI_ERR_NO_SUCH_FILE";
    if (error_code == MPI_ERR_PORT) return "MPI_ERR_PORT";
    if (error_code == MPI_ERR_QUOTA) return "MPI_ERR_QUOTA";
    if (error_code == MPI_ERR_READ_ONLY) return "MPI_ERR_READ_ONLY";
    if (error_code == MPI_ERR_RMA_CONFLICT) return "MPI_ERR_RMA_CONFLICT";
    if (error_code == MPI_ERR_RMA_SYNC) return "MPI_ERR_RMA_SYNC";
    if (error_code == MPI_ERR_SERVICE) return "MPI_ERR_SERVICE";
    if (error_code == MPI_ERR_SIZE) return "MPI_ERR_SIZE";
    if (error_code == MPI_ERR_SPAWN) return "MPI_ERR_SPAWN";
    if (error_code == MPI_ERR_UNSUPPORTED_DATAREP) return "MPI_ERR_UNSUPPORTED_DATAREP";
    if (error_code == MPI_ERR_UNSUPPORTED_OPERATION) return "MPI_ERR_UNSUPPORTED_OPERATION";
    if (error_code == MPI_ERR_WIN) return "MPI_ERR_WIN";
    if (error_code == MPI_ERR_PROC_FAILED) return "MPI_ERR_PROC_FAILED";
    if (error_code == MPI_ERR_PROC_FAIL_STOP) return "MPI_ERR_PROC_FAIL_STOP";
    if (error_code == MPI_ERR_LASTCODE) return "MPI_ERR_LASTCODE";
    return "Unknown MPI error";
}

const char* unimpi_error_string(int error_code) {
    switch (error_code) {
        case UNIMPI_OK:
            return "Success";
        case UNIMPI_ERR_NO_BACKEND:
            return "No MPI backend found";
        case UNIMPI_ERR_BACKEND_LOAD:
            return "Failed to load MPI backend library";
        case UNIMPI_ERR_ABI_MISMATCH:
            return "MPI ABI version mismatch";
        case UNIMPI_ERR_NOT_INITIALIZED:
            return "TFTK-MPI not initialized";
        case UNIMPI_ERR_ALREADY_INITIALIZED:
            return "TFTK-MPI already initialized";
        case UNIMPI_ERR_SYMBOL_NOT_FOUND:
            return "Required MPI symbol not found in backend";
        case UNIMPI_ERR_OUT_OF_MEMORY:
            return "Out of memory";
        case UNIMPI_ERR_INVALID_ARGUMENT:
            return "Invalid argument";
        case UNIMPI_ERR_BACKEND_NOT_SUPPORTED:
            return "Backend not supported on this platform";
        case UNIMPI_ERR_BACKEND_INIT_FAILED:
            return "Backend initialization failed";
        default:
            return "Unknown error";
    }
}

int unimpi_error_class(int error_code, int *error_class) {
    /* Map TFTK errors to MPI error classes */
    switch (error_code) {
        case UNIMPI_OK:
            *error_class = 0; /* MPI_SUCCESS */
            break;
        case UNIMPI_ERR_INVALID_ARGUMENT:
            *error_class = 2; /* MPI_ERR_ARG */
            break;
        case UNIMPI_ERR_NO_BACKEND:
        case UNIMPI_ERR_BACKEND_LOAD:
        case UNIMPI_ERR_BACKEND_INIT_FAILED:
            *error_class = 15; /* MPI_ERR_OTHER */
            break;
        default:
            *error_class = 15; /* MPI_ERR_OTHER */
            break;
    }
    return UNIMPI_OK;
}

void unimpi_set_debug_level(int level) {
    g_debug_level = level;
}

int unimpi_get_debug_level(void) {
    return g_debug_level;
}

static const char* debug_level_str(int level) {
    switch (level) {
        case UNIMPI_DBG_ERROR: return "ERROR";
        case UNIMPI_DBG_WARN:  return "WARN";
        case UNIMPI_DBG_INFO:  return "INFO";
        case UNIMPI_DBG_DEBUG: return "DEBUG";
        case UNIMPI_DBG_TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

void unimpi_debug_print(const char *fmt, ...) {
    va_list args;

    /* Check environment variable for debug level */
    const char *env_debug = getenv("UNIMPI_DEBUG");
    int level = UNIMPI_DBG_INFO;
    if (env_debug) {
        level = atoi(env_debug);
    } else {
        level = g_debug_level;
    }

    if (level < UNIMPI_DBG_INFO) {
        return;
    }

    fprintf(stderr, "[unimpi] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void unimpi_debug_print_level(int level, const char *fmt, ...) {
    va_list args;

    /* Check environment variable for debug level */
    const char *env_debug = getenv("UNIMPI_DEBUG");
    int current_level = g_debug_level;
    if (env_debug) {
        current_level = atoi(env_debug);
    }

    if (level > current_level) {
        return;
    }

    fprintf(stderr, "[unimpi:%s] ", debug_level_str(level));
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
