/* Fake Open MPI DSO for production array-wrapper regressions.
 *
 * Exports real Open MPI native tags:
 *   struct ompi_request_t ** request arrays
 *   struct ompi_status_public_t * status arrays
 * plus identity symbols so unimpi_vtable_init_openmpi can bind.
 */
#include <stdint.h>
#include <string.h>

#include "unimpi_platform.h"

/* MSVC does not export data symbols via WINDOWS_EXPORT_ALL_SYMBOLS alone;
 * match fake_mpi_identity.c so GetProcAddress can resolve Open MPI globals.
 */
#ifdef _WIN32
#define FAKE_MPI_EXPORT __declspec(dllexport)
#else
#define FAKE_MPI_EXPORT __attribute__((visibility("default")))
#endif

struct ompi_request_t {
    int id;
};

/* Match Open MPI mpi.h public status layout and tag. */
struct ompi_status_public_t {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int _cancelled;
    size_t _ucount;
};

enum {
    FAKE_OMPI_REQ_A = 0x51,
    FAKE_OMPI_REQ_B = 0x52,
    /* Open MPI MPI_ERR_IN_STATUS is 18; low byte must match for Error_class. */
    FAKE_ERR_IN_STATUS_CODE = 0x10000 | 18
};

/* Identity symbols for Open MPI backend detection / comm binding. */
FAKE_MPI_EXPORT int ompi_mpi_comm_world;
FAKE_MPI_EXPORT int ompi_mpi_comm_self;

int UNIMPI_MPI_CALL MPI_Init(int *argc, char ***argv) {
    (void)argc;
    (void)argv;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Finalize(void) {
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_size(void *comm, int *size) {
    (void)comm;
    if (size) {
        *size = 1;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_rank(void *comm, int *rank) {
    (void)comm;
    if (rank) {
        *rank = 0;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Error_class(int errorcode, int *error_class) {
    if (!error_class) {
        return 14;
    }
    *error_class = errorcode & 0xff;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Waitall(int count, struct ompi_request_t **requests,
                                struct ompi_status_public_t *statuses) {
    int i;

    if (count <= 0) {
        return 0;
    }
    if (!requests) {
        return 7;
    }
    for (i = 0; i < count; ++i) {
        requests[i] = NULL;
        if (statuses) {
            memset(&statuses[i], 0, sizeof(statuses[i]));
            statuses[i].MPI_SOURCE = 10 + i;
            statuses[i].MPI_TAG = 20 + i;
            statuses[i]._ucount = (size_t)(30 + i);
        }
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Testall(int count, struct ompi_request_t **requests,
                                int *flag,
                                struct ompi_status_public_t *statuses) {
    int i;

    if (!flag) {
        return 13;
    }
    if (count <= 0) {
        *flag = 1;
        return 0;
    }
    if (!requests) {
        return 7;
    }

    /* Two-request path: MPI_ERR_IN_STATUS style copyback. */
    if (count == 2 && statuses) {
        *flag = 0;
        for (i = 0; i < count; ++i) {
            requests[i] = NULL;
            memset(&statuses[i], 0, sizeof(statuses[i]));
            statuses[i].MPI_SOURCE = 40 + i;
            statuses[i].MPI_TAG = 50 + i;
            statuses[i].MPI_ERROR = 18;
            statuses[i]._ucount = (size_t)(60 + i);
        }
        return FAKE_ERR_IN_STATUS_CODE;
    }

    *flag = 1;
    for (i = 0; i < count; ++i) {
        requests[i] = NULL;
        if (statuses) {
            memset(&statuses[i], 0, sizeof(statuses[i]));
            statuses[i]._ucount = (size_t)(i + 1);
        }
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Testsome(int incount, struct ompi_request_t **requests,
                                 int *outcount, int *indices,
                                 struct ompi_status_public_t *statuses) {
    if (!outcount || !indices) {
        return 13;
    }
    if (incount <= 0) {
        *outcount = -1; /* MPI_UNDEFINED-like */
        return 0;
    }
    if (!requests) {
        return 7;
    }
    requests[0] = NULL;
    *outcount = 1;
    indices[0] = 0;
    if (statuses) {
        memset(&statuses[0], 0, sizeof(statuses[0]));
        statuses[0].MPI_SOURCE = 70;
        statuses[0]._ucount = 71;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Waitsome(int incount, struct ompi_request_t **requests,
                                 int *outcount, int *indices,
                                 struct ompi_status_public_t *statuses) {
    return MPI_Testsome(incount, requests, outcount, indices, statuses);
}

int UNIMPI_MPI_CALL MPI_Testany(int count, struct ompi_request_t **requests,
                                int *index, int *flag,
                                struct ompi_status_public_t *status) {
    if (!index || !flag) {
        return 13;
    }
    if (count <= 0) {
        *flag = 1;
        *index = -32766; /* MPI_UNDEFINED */
        return 0;
    }
    if (!requests) {
        return 7;
    }
    /* Complete request 1 when present. */
    *flag = 1;
    *index = count > 1 ? 1 : 0;
    requests[*index] = NULL;
    if (status) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 80;
        status->MPI_TAG = 81;
        status->_ucount = 82;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Waitany(int count, struct ompi_request_t **requests,
                                int *index,
                                struct ompi_status_public_t *status) {
    int flag = 0;

    return MPI_Testany(count, requests, index, &flag, status);
}

int UNIMPI_MPI_CALL MPI_Startall(int count, struct ompi_request_t **requests) {
    int i;

    /* Match Open MPI: NULL array is MPI_ERR_REQUEST even when count == 0. */
    if (!requests) {
        return 7;
    }
    if (count <= 0) {
        return 0;
    }
    /* Persistent start leaves handles live; bump id to prove round-trip. */
    for (i = 0; i < count; ++i) {
        if (requests[i]) {
            requests[i] = (struct ompi_request_t *)(intptr_t)(
                (int)(intptr_t)requests[i] | 0x100);
        }
    }
    return 0;
}
