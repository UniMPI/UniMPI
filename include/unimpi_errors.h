/* include/unimpi_errors.h - MPI Error Codes
 * Runtime-initialized error codes - values set by backend during init
 */
#ifndef UNIMPI_ERRORS_H
#define UNIMPI_ERRORS_H

/* Standard MPI error codes - initialized at runtime based on backend */
extern int MPI_SUCCESS;
extern int MPI_ERR_BUFFER;
extern int MPI_ERR_COUNT;
extern int MPI_ERR_TYPE;
extern int MPI_ERR_TAG;
extern int MPI_ERR_COMM;
extern int MPI_ERR_RANK;
extern int MPI_ERR_REQUEST;
extern int MPI_ERR_ROOT;
extern int MPI_ERR_GROUP;
extern int MPI_ERR_OP;
extern int MPI_ERR_TOPOLOGY;
extern int MPI_ERR_DIMS;
extern int MPI_ERR_ARG;
extern int MPI_ERR_UNKNOWN;
extern int MPI_ERR_TRUNCATE;
extern int MPI_ERR_OTHER;
extern int MPI_ERR_INTERN;
extern int MPI_ERR_IN_STATUS;
extern int MPI_ERR_PENDING;
extern int MPI_ERR_ACCESS;
extern int MPI_ERR_AMODE;
extern int MPI_ERR_ASSERT;
extern int MPI_ERR_BAD_FILE;
extern int MPI_ERR_BASE;
extern int MPI_ERR_CONVERSION;
extern int MPI_ERR_DISP;
extern int MPI_ERR_DUP_DATAREP;
extern int MPI_ERR_FILE_EXISTS;
extern int MPI_ERR_FILE_IN_USE;
extern int MPI_ERR_FILE;
extern int MPI_ERR_INFO_KEY;
extern int MPI_ERR_INFO_NOKEY;
extern int MPI_ERR_INFO_VALUE;
extern int MPI_ERR_INFO;
extern int MPI_ERR_IO;
extern int MPI_ERR_KEYVAL;
extern int MPI_ERR_LOCKTYPE;
extern int MPI_ERR_NAME;
extern int MPI_ERR_NO_MEM;
extern int MPI_ERR_NOT_SAME;
extern int MPI_ERR_NO_SPACE;
extern int MPI_ERR_NO_SUCH_FILE;
extern int MPI_ERR_PORT;
extern int MPI_ERR_QUOTA;
extern int MPI_ERR_READ_ONLY;
extern int MPI_ERR_RMA_CONFLICT;
extern int MPI_ERR_RMA_SYNC;
extern int MPI_ERR_RMA_RANGE;
extern int MPI_ERR_SERVICE;
extern int MPI_ERR_SIZE;
extern int MPI_ERR_SPAWN;
extern int MPI_ERR_UNSUPPORTED_DATAREP;
extern int MPI_ERR_UNSUPPORTED_OPERATION;
extern int MPI_ERR_WIN;
extern int MPI_ERR_RMA_ATTACH;
extern int MPI_ERR_RMA_SHARED;
extern int MPI_ERR_RMA_FLAVOR;
extern int MPI_ERR_SESSION;
extern int MPI_ERR_PROC_ABORTED;
extern int MPI_ERR_VALUE_TOO_LARGE;
extern int MPI_ERR_ERRHANDLER;
extern int MPI_ERR_ABI;
extern int MPI_ERR_LASTCODE;

/* Thread levels */
extern int MPI_THREAD_SINGLE;
extern int MPI_THREAD_FUNNELED;
extern int MPI_THREAD_SERIALIZED;
extern int MPI_THREAD_MULTIPLE;

/* File operation constants */
extern int MPI_MODE_RDONLY;
extern int MPI_MODE_RDWR;
extern int MPI_MODE_WRONLY;
extern int MPI_MODE_CREATE;
extern int MPI_MODE_EXCL;
extern int MPI_MODE_DELETE_ON_CLOSE;
extern int MPI_MODE_UNIQUE_OPEN;
extern int MPI_MODE_APPEND;
extern int MPI_MODE_SEQUENTIAL;

/* Seek constants */
extern int MPI_SEEK_SET;
extern int MPI_SEEK_CUR;
extern int MPI_SEEK_END;

/* Order constants */
extern int MPI_ORDER_C;
extern int MPI_ORDER_FORTRAN;

/* Distribution constants */
extern int MPI_DISTRIBUTE_BLOCK;
extern int MPI_DISTRIBUTE_CYCLIC;
extern int MPI_DISTRIBUTE_NONE;
extern int MPI_DISTRIBUTE_DFLT_DARG;

/* Window constants */
extern int MPI_WIN_FLAVOR_CREATE;
extern int MPI_WIN_FLAVOR_ALLOCATE;
extern int MPI_WIN_FLAVOR_DYNAMIC;
extern int MPI_WIN_FLAVOR_SHARED;
extern int MPI_WIN_SEPARATE;
extern int MPI_WIN_UNIFIED;

/* Lock types */
extern int MPI_LOCK_EXCLUSIVE;
extern int MPI_LOCK_SHARED;

/* Compare results */
extern int MPI_IDENT;
extern int MPI_CONGRUENT;
extern int MPI_SIMILAR;
extern int MPI_UNEQUAL;

/* Results of test/wait */
extern int MPI_TEST_CANCELLED_TRUE;
extern int MPI_TEST_CANCELLED_FALSE;

/* Special constants */
extern int MPI_ANY_SOURCE;
extern int MPI_ANY_TAG;
extern int MPI_UNDEFINED;
extern int MPI_TAG_UB;
extern int MPI_HOST;
extern int MPI_IO;
extern int MPI_WTIME_IS_GLOBAL;
extern int MPI_BSEND_OVERHEAD;
extern int MPI_PROC_NULL;
extern int MPI_ROOT;

/* MPI_IN_PLACE buffer sentinel for in-place collectives. Backend-specific
 * pointer value (OpenMPI (void*)1, MPICH-family (void*)-1); set by each
 * backend at init so in-place calls pass the loaded implementation's own
 * sentinel. */
extern void *MPI_IN_PLACE;

/* Fortran status size - this is the size of MPI_Status in integers */
extern int MPI_STATUS_SIZE;

/* Note: MPI_SOURCE, MPI_TAG, MPI_ERROR are fields within MPI_Status struct,
 * not separate variables. Access as status.MPI_SOURCE etc. */

/* Communicator split types */
extern int MPI_COMM_TYPE_SHARED;
extern int MPI_COMM_TYPE_HW_UNGUIDED;
extern int MPI_COMM_TYPE_HW_THREAD;

/* Maximum values */
extern int MPI_MAX_PROCESSOR_NAME;
extern int MPI_MAX_LIBRARY_VERSION_STRING;
extern int MPI_MAX_ERROR_STRING;
extern int MPI_MAX_INFO_KEY;
extern int MPI_MAX_INFO_VAL;
extern int MPI_MAX_OBJECT_NAME;
extern int MPI_MAX_PORT_NAME;
extern int MPI_MAX_DATAREP_STRING;

#endif /* UNIMPI_ERRORS_H */
