/* tests/fixtures/fake_integer_request_api.c
 * Fake MPI API for testing integer request handle ABI adapters.
 *
 * Simulates MPICH/Intel-MPI/MS-MPI 4-byte request handles.
 * UniMPI facade uses intptr_t (8 bytes), requiring conversion at boundary.
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

/* Native types */
typedef int fake_int_comm;
typedef int fake_int_datatype;
typedef int fake_int_request;
typedef int fake_int_status;

/* Predefined handles */
#define FAKE_COMM_WORLD    0x44000000
#define FAKE_COMM_SELF     0x44000001
#define FAKE_COMM_NULL     0x04000000
#define FAKE_DATATYPE_NULL 0x0c000000
#define FAKE_INT           0x0c000001
#define FAKE_REQUEST_NULL  0x2c000000

/* Error codes */
#define FAKE_MPI_SUCCESS        0
#define FAKE_MPI_ERR_COMM       5
#define FAKE_MPI_ERR_ARG        13
#define FAKE_MPI_ERR_COUNT      2
#define FAKE_MPI_ERR_TYPE       3
#define FAKE_MPI_ERR_TAG        4
#define FAKE_MPI_ERR_RANK       6
#define FAKE_MPI_ERR_REQUEST    19
#define FAKE_MPI_ERR_IN_STATUS  17
#define FAKE_MPI_ERR_PENDING    18
#define FAKE_MPI_ERR_BUFFER     1

/* Status layout (compact 20 bytes for integer backends) */
typedef struct {
    int count_lo;
    int count_hi_and_cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
} fake_status;

/* Request allocation */
static int next_request = FAKE_REQUEST_NULL + 0x100;

/* ==================== Point-to-Point ==================== */

int FAKE_MPI_CALL MPI_Isend(const void *buf, int count, fake_int_datatype datatype,
                            int dest, int tag, fake_int_comm comm,
                            fake_int_request *request) {
    (void)buf;
    (void)count;

    assert((intptr_t)datatype == (int)datatype);
    assert((intptr_t)comm == (int)comm);

    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;
    if (dest < 0) return FAKE_MPI_ERR_RANK;
    if (tag < 0) return FAKE_MPI_ERR_TAG;
    if (!request) return FAKE_MPI_ERR_ARG;

    /* Return new request handle (4-byte native) */
    *request = next_request++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Irecv(void *buf, int count, fake_int_datatype datatype,
                            int source, int tag, fake_int_comm comm,
                            fake_int_request *request) {
    (void)buf;
    (void)count;

    assert((intptr_t)datatype == (int)datatype);
    assert((intptr_t)comm == (int)comm);

    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;
    if (tag < 0) return FAKE_MPI_ERR_TAG;
    if (!request) return FAKE_MPI_ERR_ARG;

    *request = next_request++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Wait(fake_int_request *request, fake_status *status) {
    if (!request) return FAKE_MPI_ERR_REQUEST;

    /* Mark as completed and return to null */
    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 0;
        status->MPI_TAG = 0;
        status->MPI_ERROR = FAKE_MPI_SUCCESS;
    }

    *request = FAKE_REQUEST_NULL;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Test(fake_int_request *request, int *flag, fake_status *status) {
    if (!request || !flag) return FAKE_MPI_ERR_ARG;

    /* Always complete immediately in fake */
    *flag = 1;

    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 0;
        status->MPI_TAG = 0;
        status->MPI_ERROR = FAKE_MPI_SUCCESS;
    }

    *request = FAKE_REQUEST_NULL;
    return FAKE_MPI_SUCCESS;
}

/* ==================== Persistent Requests ==================== */

int FAKE_MPI_CALL MPI_Send_init(const void *buf, int count, fake_int_datatype datatype,
                                int dest, int tag, fake_int_comm comm,
                                fake_int_request *request) {
    (void)buf;
    (void)count;

    assert((intptr_t)datatype == (int)datatype);
    assert((intptr_t)comm == (int)comm);

    if (!request) return FAKE_MPI_ERR_ARG;
    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;

    *request = next_request++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Recv_init(void *buf, int count, fake_int_datatype datatype,
                                int source, int tag, fake_int_comm comm,
                                fake_int_request *request) {
    (void)buf;
    (void)count;

    assert((intptr_t)datatype == (int)datatype);
    assert((intptr_t)comm == (int)comm);

    if (!request) return FAKE_MPI_ERR_ARG;
    if (comm == FAKE_COMM_NULL) return FAKE_MPI_ERR_COMM;

    *request = next_request++;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Start(fake_int_request *request) {
    if (!request) return FAKE_MPI_ERR_ARG;
    /* Just mark as started - still valid handle */
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Request_free(fake_int_request *request) {
    if (!request) return FAKE_MPI_ERR_ARG;
    *request = FAKE_REQUEST_NULL;
    return FAKE_MPI_SUCCESS;
}

/* ==================== Array Completion (native signatures) ==================== */

/* Native waitall takes int* for requests, compact status array */
int FAKE_MPI_CALL MPI_Waitall(int count, int *array_of_requests,
                               fake_status *array_of_statuses) {
    int i;

    if (count < 0) return FAKE_MPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return FAKE_MPI_ERR_ARG;

    for (i = 0; i < count; i++) {
        array_of_requests[i] = FAKE_REQUEST_NULL;
    }

    if (array_of_statuses && count > 0) {
        for (i = 0; i < count; i++) {
            memset(&array_of_statuses[i], 0, sizeof(array_of_statuses[i]));
            array_of_statuses[i].MPI_SOURCE = 0;
            array_of_statuses[i].MPI_TAG = 0;
            array_of_statuses[i].MPI_ERROR = FAKE_MPI_SUCCESS;
        }
    }

    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Testall(int count, int *array_of_requests, int *flag,
                               fake_status *array_of_statuses) {
    int i;

    if (count < 0 || !flag) return FAKE_MPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return FAKE_MPI_ERR_ARG;

    *flag = 1;  /* Always complete in fake */

    for (i = 0; i < count; i++) {
        array_of_requests[i] = FAKE_REQUEST_NULL;
    }

    if (array_of_statuses && count > 0) {
        for (i = 0; i < count; i++) {
            memset(&array_of_statuses[i], 0, sizeof(array_of_statuses[i]));
            array_of_statuses[i].MPI_SOURCE = 0;
            array_of_statuses[i].MPI_TAG = 0;
            array_of_statuses[i].MPI_ERROR = FAKE_MPI_SUCCESS;
        }
    }

    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Waitany(int count, int *array_of_requests, int *index,
                               fake_status *status) {
    int i;

    if (count < 0 || !index) return FAKE_MPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return FAKE_MPI_ERR_ARG;

    if (count == 0) {
        *index = -1;  /* MPI_UNDEFINED */
        return FAKE_MPI_SUCCESS;
    }

    /* Return first non-null request */
    for (i = 0; i < count; i++) {
        if (array_of_requests[i] != FAKE_REQUEST_NULL) {
            *index = i;
            array_of_requests[i] = FAKE_REQUEST_NULL;
            if (status) {
                memset(status, 0, sizeof(*status));
                status->MPI_SOURCE = 0;
                status->MPI_TAG = i;  /* Encode index in tag for verification */
                status->MPI_ERROR = FAKE_MPI_SUCCESS;
            }
            return FAKE_MPI_SUCCESS;
        }
    }

    /* All null - return first index */
    *index = 0;
    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_ERROR = FAKE_MPI_SUCCESS;
    }
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Testany(int count, int *array_of_requests, int *index,
                               int *flag, fake_status *status) {
    return MPI_Waitany(count, array_of_requests, index, status);
    /* Always complete, so flag always set */
    if (flag) *flag = 1;
}

int FAKE_MPI_CALL MPI_Waitsome(int incount, int *array_of_requests, int *outcount,
                                int *array_of_indices, fake_status *array_of_statuses) {
    int i;
    int found = 0;

    if (incount < 0 || !outcount || !array_of_indices) return FAKE_MPI_ERR_ARG;
    if (incount > 0 && !array_of_requests) return FAKE_MPI_ERR_ARG;

    if (incount == 0) {
        *outcount = -1;  /* MPI_UNDEFINED */
        return FAKE_MPI_SUCCESS;
    }

    for (i = 0; i < incount && found < incount; i++) {
        if (array_of_requests[i] != FAKE_REQUEST_NULL) {
            array_of_indices[found] = i;
            array_of_requests[i] = FAKE_REQUEST_NULL;
            if (array_of_statuses) {
                memset(&array_of_statuses[found], 0, sizeof(array_of_statuses[found]));
                array_of_statuses[found].MPI_SOURCE = 0;
                array_of_statuses[found].MPI_TAG = i;
                array_of_statuses[found].MPI_ERROR = FAKE_MPI_SUCCESS;
            }
            found++;
        }
    }

    *outcount = found;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Testsome(int incount, int *array_of_requests, int *outcount,
                                int *array_of_indices, fake_status *array_of_statuses) {
    return MPI_Waitsome(incount, array_of_requests, outcount,
                        array_of_indices, array_of_statuses);
}

int FAKE_MPI_CALL MPI_Startall(int count, int *array_of_requests) {
    int i;

    if (count < 0) return FAKE_MPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return FAKE_MPI_ERR_ARG;

    /* Just mark all as started */
    (void)i;
    return FAKE_MPI_SUCCESS;
}

/* ==================== Cancel ==================== */

int FAKE_MPI_CALL MPI_Cancel(fake_int_request *request) {
    if (!request) return FAKE_MPI_ERR_ARG;
    return FAKE_MPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Test_cancelled(const fake_status *status, int *flag) {
    if (!status || !flag) return FAKE_MPI_ERR_ARG;
    *flag = 0;  /* Never cancelled in fake */
    return FAKE_MPI_SUCCESS;
}

