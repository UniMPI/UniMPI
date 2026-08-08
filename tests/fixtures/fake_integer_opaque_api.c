/* tests/fixtures/fake_integer_opaque_api.c
 * Fake MPI API for testing integer opaque handle ABI adapters.
 *
 * Simulates MPICH/Intel-MPI/MS-MPI integer-handle backends where:
 * - MPI_Comm, MPI_Datatype, MPI_Group, MPI_Win, MPI_Op, MPI_Info are native int
 * - UniMPI facade uses intptr_t (8 bytes on x86_64)
 * - This fixture provides 4-byte native handles for testing conversion
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#define FAKE_MPI_CALL __stdcall
#else
#define FAKE_MPI_CALL
#endif

/* Native integer handle types (4 bytes) */
typedef int fake_int_comm;
typedef int fake_int_datatype;
typedef int fake_int_group;
typedef int fake_int_win;
typedef int fake_int_op;
typedef int fake_int_info;
typedef int fake_int_request;

/* Predefined communicator handles */
#define FAKE_COMM_WORLD  0x44000000
#define FAKE_COMM_SELF   0x44000001
#define FAKE_COMM_NULL   0x04000000

/* Predefined datatype handles */
#define FAKE_DATATYPE_NULL 0x0c000000
#define FAKE_INT           0x0c000001
#define FAKE_CHAR          0x0c000002
#define FAKE_DOUBLE        0x0c000003

/* Predefined group handles */
#define FAKE_GROUP_NULL    0x08000000
#define FAKE_GROUP_EMPTY   0x08000001

/* Predefined request */
#define FAKE_REQUEST_NULL  0x2c000000

/* Status structure (compact 20-byte layout) */
typedef struct {
    int count_lo;
    int count_hi_and_cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
} fake_int_status;

/* Error codes */
#define FAKE_MPI_SUCCESS        0
#define FAKE_MPI_ERR_COMM       5
#define FAKE_MPI_ERR_ARG        13
#define FAKE_MPI_ERR_TYPE       3
#define FAKE_MPI_ERR_GROUP      9
#define FAKE_MPI_ERR_WIN        53
#define FAKE_MPI_ERR_OP         10
#define FAKE_MPI_ERR_INFO       34
#define FAKE_MPI_ERR_REQUEST    19
#define FAKE_MPI_ERR_IN_STATUS  17

/* Handle allocation tracking for memory leak detection */
static int next_comm_handle = FAKE_COMM_WORLD + 0x100;
static int next_datatype_handle = FAKE_DATATYPE_NULL + 0x100;
static int next_group_handle = FAKE_GROUP_NULL + 0x100;
static int next_win_handle = 0x20000000;
static int next_info_handle = 0x1c000001;

/* ==================== Communicator Operations ==================== */

int FAKE_MPI_CALL MPI_Comm_size(fake_int_comm comm, int *size) {
    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;
    *size = 4;  /* Fake 4-process world */
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Comm_rank(fake_int_comm comm, int *rank) {
    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;
    *rank = 0;  /* Always rank 0 in fake */
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Comm_dup(fake_int_comm comm, fake_int_comm *newcomm) {
    /* Verify native handle is 4-byte value */
    assert((intptr_t)comm == (int)comm);  /* Should fit in 32 bits */

    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;

    *newcomm = next_comm_handle++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Comm_free(fake_int_comm *comm) {
    if (!comm) return FAKE_MPI_ERR_ARG;
    *comm = FAKE_COMM_NULL;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Comm_test_inter(fake_int_comm comm, int *flag) {
    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;
    *flag = 0;  /* Always intra-communicator */
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Comm_remote_size(fake_int_comm comm, int *size) {
    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;
    *size = 0;  /* No remote processes in intra-comm */
    return FAKE_MPI_SUCCESS;
}

/* ==================== Datatype Operations ==================== */

int FAKE_MPI_CALL MPI_Type_dup(fake_int_datatype oldtype, fake_int_datatype *newtype) {
    assert((intptr_t)oldtype == (int)oldtype);

    if (oldtype == FAKE_DATATYPE_NULL) return FAKE_MPI_ERR_TYPE;

    *newtype = next_datatype_handle++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Type_free(fake_int_datatype *datatype) {
    if (!datatype) return FAKE_MPI_ERR_ARG;
    *datatype = FAKE_DATATYPE_NULL;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Type_contiguous(int count, fake_int_datatype oldtype,
                                       fake_int_datatype *newtype) {
    if (count < 0) return FAKE_MPI_ERR_ARG;
    if (oldtype == FAKE_DATATYPE_NULL) return FAKE_MPI_ERR_TYPE;

    *newtype = next_datatype_handle++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Type_commit(fake_int_datatype *datatype) {
    if (!datatype || *datatype == FAKE_DATATYPE_NULL) return FAKE_MPI_ERR_TYPE;
    return FAKE_MPI_SUCCESS;
}

/* ==================== Group Operations ==================== */

int FAKE_MPI_CALL MPI_Group_size(fake_int_group group, int *size) {
    if (group == FAKE_GROUP_NULL) return FAKE_MPI_ERR_GROUP;
    *size = 4;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Group_rank(fake_int_group group, int *rank) {
    if (group == FAKE_GROUP_NULL) return FAKE_MPI_ERR_GROUP;
    *rank = 0;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Group_incl(fake_int_group group, int n, const int *ranks,
                                  fake_int_group *newgroup) {
    assert((intptr_t)group == (int)group);

    if (group == FAKE_GROUP_NULL) return FAKE_MPI_ERR_GROUP;
    if (n < 0) return FAKE_MPI_ERR_ARG;

    *newgroup = next_group_handle++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Group_free(fake_int_group *group) {
    if (!group) return FAKE_MPI_ERR_ARG;
    *group = FAKE_GROUP_NULL;
    return FAKE_MPI_SUCCESS;
}

/* ==================== Info Operations ==================== */

int FAKE_MPI_CALL MPI_Info_create(fake_int_info *info) {
    *info = next_info_handle++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Info_free(fake_int_info *info) {
    if (!info) return FAKE_MPI_ERR_ARG;
    *info = 0x1c000000;  /* FAKE_INFO_NULL */
    return FAKE_MPI_SUCCESS;
}

/* ==================== Window Operations ==================== */

int FAKE_MPI_CALL MPI_Win_create(void *base, intptr_t size, int disp_unit,
                                  fake_int_info info, fake_int_comm comm,
                                  fake_int_win *win) {
    assert((intptr_t)comm == (int)comm);

    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;

    *win = next_win_handle++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Win_free(fake_int_win *win) {
    if (!win) return FAKE_MPI_ERR_ARG;
    *win = 0x20000000;  /* FAKE_WIN_NULL */
    return FAKE_MPI_SUCCESS;
}

/* ==================== Operation Operations ==================== */

int FAKE_MPI_CALL MPI_Op_free(fake_int_op *op) {
    if (!op) return FAKE_MPI_ERR_ARG;
    *op = 0x18000000;  /* FAKE_OP_NULL */
    return FAKE_MPI_SUCCESS;
}

/* ==================== Request Operations (minimal) ==================== */

int FAKE_MPI_CALL MPI_Wait(fake_int_request *request, fake_int_status *status) {
    if (!request) return FAKE_MPI_ERR_REQUEST;
    *request = FAKE_REQUEST_NULL;
    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_ERROR = FAKE_MPI_SUCCESS;
    }
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Test(fake_int_request *request, int *flag, fake_int_status *status) {
    if (!request || !flag) return FAKE_MPI_ERR_ARG;
    *flag = 1;  /* Always complete immediately in fake */
    *request = FAKE_REQUEST_NULL;
    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_ERROR = FAKE_MPI_SUCCESS;
    }
    return FAKE_MPI_SUCCESS;
}

/* ==================== Global Symbol Exports ==================== */

/* Export symbols for backend detection */
int MPIR_Dup_fn;
int MPIR_Err_create_code;

/* Intel MPI specific symbol */
int __I_MPI___cpu_core_type;

/* MS-MPI specific */
int MSMPI_Get_version;

