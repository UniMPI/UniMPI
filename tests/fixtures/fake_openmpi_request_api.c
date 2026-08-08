/* tests/fixtures/fake_openmpi_request_api.c
 * Fake MPI API for testing OpenMPI pointer request ABI.
 *
 * OpenMPI uses pointer handles (ompi_request_t*) and has a different
 * status stride than the 128-byte facade. This fixture tests the
 * OpenMPI-specific status array adaptation.
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

/* OpenMPI uses opaque pointer types */
typedef struct ompi_communicator_t *ompi_comm_t;
typedef struct ompi_datatype_t *ompi_datatype_t;
typedef struct ompi_request_t *ompi_request_t;
typedef struct ompi_status_t {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int _cancelled;
    size_t _ucount;
    char _padding[80];  /* Total ~96-100 bytes */
} ompi_status_t;

/* Predefined communicators (pointers to dummy structures) */
static struct ompi_communicator_t ompi_comm_world_struct;
static struct ompi_communicator_t ompi_comm_self_struct;
static struct ompi_communicator_t ompi_comm_null_struct;

ompi_comm_t ompi_mpi_comm_world = &ompi_comm_world_struct;
ompi_comm_t ompi_mpi_comm_self = &ompi_comm_self_struct;
ompi_comm_t ompi_mpi_comm_null = &ompi_comm_null_struct;

/* Predefined datatypes */
static struct ompi_datatype_t ompi_datatype_int_struct;
static struct ompi_datatype_t ompi_datatype_char_struct;
static struct ompi_datatype_t ompi_datatype_null_struct;

ompi_datatype_t ompi_mpi_int = &ompi_datatype_int_struct;
ompi_datatype_t ompi_mpi_char = &ompi_datatype_char_struct;
ompi_datatype_t ompi_mpi_datatype_null = &ompi_datatype_null_struct;

/* Predefined request */
ompi_request_t ompi_request_null = NULL;

/* Error codes (OpenMPI specific) */
#define OMPI_SUCCESS        0
#define OMPI_ERR_COMM       5
#define OMPI_ERR_ARG        13
#define OMPI_ERR_REQUEST    7
#define OMPI_ERR_IN_STATUS  17
#define OMPI_ERR_PENDING    18

/* Request allocation */
static int request_counter = 0;
static ompi_request_t active_requests[1024];

/* ==================== Helper Functions ==================== */

static ompi_request_t allocate_request(void) {
    int i;
    for (i = 0; i < 1024; i++) {
        if (active_requests[i] == NULL) {
            /* Allocate a dummy request object */
            active_requests[i] = (ompi_request_t)(intptr_t)(0x1000 + i);
            return active_requests[i];
        }
    }
    return NULL;
}

static void free_request(ompi_request_t req) {
    int i;
    for (i = 0; i < 1024; i++) {
        if (active_requests[i] == req) {
            active_requests[i] = NULL;
            return;
        }
    }
}

/* ==================== Point-to-Point ==================== */

int FAKE_MPI_CALL MPI_Isend(const void *buf, int count, ompi_datatype_t datatype,
                            int dest, int tag, ompi_comm_t comm,
                            ompi_request_t **request) {
    (void)buf;
    (void)count;
    (void)dest;
    (void)tag;

    if (comm == ompi_mpi_comm_null) return OMPI_ERR_COMM;
    if (!request) return OMPI_ERR_ARG;

    /* OpenMPI: request is pointer to pointer */
    *request = allocate_request();
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Irecv(void *buf, int count, ompi_datatype_t datatype,
                            int source, int tag, ompi_comm_t comm,
                            ompi_request_t **request) {
    (void)buf;
    (void)count;
    (void)source;
    (void)tag;

    if (comm == ompi_mpi_comm_null) return OMPI_ERR_COMM;
    if (!request) return OMPI_ERR_ARG;

    *request = allocate_request();
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Wait(ompi_request_t **request, ompi_status_t *status) {
    if (!request) return OMPI_ERR_REQUEST;

    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 0;
        status->MPI_TAG = 0;
        status->MPI_ERROR = OMPI_SUCCESS;
    }

    if (*request) {
        free_request(*request);
        *request = ompi_request_null;
    }
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Test(ompi_request_t **request, int *flag,
                           ompi_status_t *status) {
    if (!request || !flag) return OMPI_ERR_ARG;

    *flag = 1;  /* Always complete in fake */

    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 0;
        status->MPI_TAG = 0;
        status->MPI_ERROR = OMPI_SUCCESS;
    }

    if (*request) {
        free_request(*request);
        *request = ompi_request_null;
    }
    return OMPI_SUCCESS;
}

/* ==================== Array Completion with OpenMPI Status ==================== */

/* OpenMPI's native waitall takes ompi_request_t** and ompi_status_t* */
int FAKE_MPI_CALL MPI_Waitall(int count, ompi_request_t **array_of_requests,
                               ompi_status_t *array_of_statuses) {
    int i;

    if (count < 0) return OMPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return OMPI_ERR_ARG;

    for (i = 0; i < count; i++) {
        if (array_of_requests[i]) {
            free_request(array_of_requests[i]);
            array_of_requests[i] = ompi_request_null;
        }
    }

    /* OpenMPI status stride is sizeof(ompi_status_t) ~ 96 bytes */
    if (array_of_statuses && count > 0) {
        for (i = 0; i < count; i++) {
            ompi_status_t *status = &array_of_statuses[i];
            memset(status, 0, sizeof(*status));
            status->MPI_SOURCE = 0;
            status->MPI_TAG = i;  /* Encode index for verification */
            status->MPI_ERROR = OMPI_SUCCESS;
        }
    }

    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Testall(int count, ompi_request_t **array_of_requests,
                               int *flag, ompi_status_t *array_of_statuses) {
    int i;

    if (count < 0 || !flag) return OMPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return OMPI_ERR_ARG;

    *flag = 1;  /* Always complete */

    for (i = 0; i < count; i++) {
        if (array_of_requests[i]) {
            free_request(array_of_requests[i]);
            array_of_requests[i] = ompi_request_null;
        }
    }

    if (array_of_statuses && count > 0) {
        for (i = 0; i < count; i++) {
            ompi_status_t *status = &array_of_statuses[i];
            memset(status, 0, sizeof(*status));
            status->MPI_SOURCE = 0;
            status->MPI_TAG = i;
            status->MPI_ERROR = OMPI_SUCCESS;
        }
    }

    return OMPI_SUCCESS;
}

/* Testany/Waitany with OpenMPI's larger status */
int FAKE_MPI_CALL MPI_Waitany(int count, ompi_request_t **array_of_requests,
                               int *index, ompi_status_t *status) {
    int i;

    if (count < 0 || !index) return OMPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return OMPI_ERR_ARG;

    if (count == 0) {
        *index = -1;
        return OMPI_SUCCESS;
    }

    for (i = 0; i < count; i++) {
        if (array_of_requests[i] != ompi_request_null) {
            *index = i;
            free_request(array_of_requests[i]);
            array_of_requests[i] = ompi_request_null;
            if (status) {
                memset(status, 0, sizeof(*status));
                status->MPI_SOURCE = 0;
                status->MPI_TAG = i;
                status->MPI_ERROR = OMPI_SUCCESS;
            }
            return OMPI_SUCCESS;
        }
    }

    *index = 0;
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Testany(int count, ompi_request_t **array_of_requests,
                               int *index, int *flag, ompi_status_t *status) {
    if (flag) *flag = 1;
    return MPI_Waitany(count, array_of_requests, index, status);
}

/* Waitsome/Testsome with OpenMPI status stride */
int FAKE_MPI_CALL MPI_Waitsome(int incount, ompi_request_t **array_of_requests,
                                int *outcount, int *array_of_indices,
                                ompi_status_t *array_of_statuses) {
    int i;
    int found = 0;

    if (incount < 0 || !outcount || !array_of_indices) return OMPI_ERR_ARG;
    if (incount > 0 && !array_of_requests) return OMPI_ERR_ARG;

    if (incount == 0) {
        *outcount = -1;
        return OMPI_SUCCESS;
    }

    for (i = 0; i < incount && found < incount; i++) {
        if (array_of_requests[i] != ompi_request_null) {
            array_of_indices[found] = i;
            free_request(array_of_requests[i]);
            array_of_requests[i] = ompi_request_null;
            if (array_of_statuses) {
                ompi_status_t *status = &array_of_statuses[found];
                memset(status, 0, sizeof(*status));
                status->MPI_SOURCE = 0;
                status->MPI_TAG = i;
                status->MPI_ERROR = OMPI_SUCCESS;
            }
            found++;
        }
    }

    *outcount = found;
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Testsome(int incount, ompi_request_t **array_of_requests,
                                int *outcount, int *array_of_indices,
                                ompi_status_t *array_of_statuses) {
    return MPI_Waitsome(incount, array_of_requests, outcount,
                         array_of_indices, array_of_statuses);
}

/* Startall with OpenMPI request pointers */
int FAKE_MPI_CALL MPI_Startall(int count, ompi_request_t **array_of_requests) {
    int i;

    if (count < 0) return OMPI_ERR_ARG;
    if (count > 0 && !array_of_requests) return OMPI_ERR_ARG;

    /* Just mark as started - handles remain valid */
    for (i = 0; i < count; i++) {
        (void)0;  /* No-op in fake */
    }
    return OMPI_SUCCESS;
}

/* Persistent requests */
int FAKE_MPI_CALL MPI_Send_init(const void *buf, int count, ompi_datatype_t datatype,
                                int dest, int tag, ompi_comm_t comm,
                                ompi_request_t **request) {
    return MPI_Isend(buf, count, datatype, dest, tag, comm, request);
}

int FAKE_MPI_CALL MPI_Recv_init(void *buf, int count, ompi_datatype_t datatype,
                                int source, int tag, ompi_comm_t comm,
                                ompi_request_t **request) {
    return MPI_Irecv(buf, count, datatype, source, tag, comm, request);
}

int FAKE_MPI_CALL MPI_Start(ompi_request_t **request) {
    (void)request;
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Request_free(ompi_request_t **request) {
    if (!request) return OMPI_ERR_ARG;
    if (*request) {
        free_request(*request);
        *request = ompi_request_null;
    }
    return OMPI_SUCCESS;
}

/* ==================== Status Helpers ==================== */

int FAKE_MPI_CALL MPI_Get_count(const ompi_status_t *status, ompi_datatype_t datatype,
                                 int *count) {
    if (!status || !count) return OMPI_ERR_ARG;
    *count = (int)status->_ucount;
    return OMPI_SUCCESS;
}

int FAKE_MPI_CALL MPI_Get_elements(const ompi_status_t *status, ompi_datatype_t datatype,
                                    int *elements) {
    if (!status || !elements) return OMPI_ERR_ARG;
    *elements = (int)status->_ucount / sizeof(int);
    return OMPI_SUCCESS;
}

