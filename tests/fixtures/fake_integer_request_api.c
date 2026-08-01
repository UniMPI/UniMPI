/* Fake integer-handle MPI DSO for production binder regressions.
 *
 * Exports the real MPICH / Intel MPI / MS-MPI native C signatures
 * (int handles, struct ADIOI_FileD * for MPI_File, legacy status layout),
 * not UniMPI facade intptr_t aliases, so UBSan exercises the production
 * contract rather than a mirrored facade typedef.
 */
#include <stdint.h>
#include <string.h>

#include "unimpi_platform.h"

/* Match production integer-backend status layout without facade aliases. */
struct unimpi_status_legacy {
    int count_lo;
    int count_hi_and_cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
};

struct ADIOI_FileD;

enum {
    FAKE_NATIVE_REQUEST = (int)0xac000001,
    FAKE_NATIVE_MESSAGE = (int)0xad000002,
    FAKE_NATIVE_NULL = (int)0x2c000000,
    FAKE_ERR_IN_STATUS_CODE = 0x10000 | 17,
    FAKE_ERR_OTHER = 15
};

/* Same sentinel value as UNIMPI_STATUSES_IGNORE ((MPI_Status *)1). */
#define FAKE_STATUSES_IGNORE ((struct unimpi_status_legacy *)(intptr_t)1)

static int g_started;
static int g_message_live;
static int g_fail_next_wait;
static int g_fail_next_mprobe;
static int g_fail_next_improbe;
static int g_fail_next_mrecv;

static int status_is_ignored(const struct unimpi_status_legacy *status) {
    return status == NULL || status == FAKE_STATUSES_IGNORE;
}

/* Test-visible failure injection (called via weak optional symbols below). */
int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_wait(int enable) {
    g_fail_next_wait = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_mprobe(int enable) {
    g_fail_next_mprobe = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_improbe(int enable) {
    g_fail_next_improbe = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_mrecv(int enable) {
    g_fail_next_mrecv = enable ? 1 : 0;
    return 0;
}

/* --- Core symbols required by vtable validation --- */

int UNIMPI_MPI_CALL MPI_Init(int *argc, char ***argv) {
    (void)argc;
    (void)argv;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Finalize(void) {
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_size(int comm, int *size) {
    (void)comm;
    if (size) {
        *size = 1;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_rank(int comm, int *rank) {
    (void)comm;
    if (rank) {
        *rank = 0;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Error_class(int errorcode, int *error_class) {
    if (!error_class) {
        return 13;
    }
    *error_class = errorcode & 0xff;
    return 0;
}

/* --- Request producers / completion (native int handles) --- */

int UNIMPI_MPI_CALL MPI_Irecv(void *buf, int count, int datatype, int source,
                              int tag, int comm, int *request) {
    (void)buf;
    (void)count;
    (void)datatype;
    (void)source;
    (void)tag;
    (void)comm;
    if (!request) {
        return 19;
    }
    *request = FAKE_NATIVE_REQUEST;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Send_init(const void *buf, int count, int datatype,
                                  int dest, int tag, int comm, int *request) {
    (void)buf;
    (void)count;
    (void)datatype;
    (void)dest;
    (void)tag;
    (void)comm;
    if (!request) {
        return 19;
    }
    *request = FAKE_NATIVE_REQUEST;
    g_started = 0;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Startall(int count, int *requests) {
    if (count <= 0) {
        return 0;
    }
    if (!requests || requests[0] != FAKE_NATIVE_REQUEST) {
        return 19;
    }
    g_started = 1;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Request_free(int *request) {
    if (!request || *request != FAKE_NATIVE_REQUEST || !g_started) {
        return 19;
    }
    *request = FAKE_NATIVE_NULL;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Wait(int *request,
                             struct unimpi_status_legacy *status) {
    if (!request || *request != FAKE_NATIVE_REQUEST) {
        return 19;
    }
    if (g_fail_next_wait) {
        g_fail_next_wait = 0;
        /* Leave request and status undefined on failure. */
        *request = (int)0xdeadbeef;
        if (!status_is_ignored(status)) {
            status->MPI_SOURCE = 99;
            status->MPI_TAG = 99;
            status->MPI_ERROR = 99;
            status->count_lo = 99;
        }
        return FAKE_ERR_OTHER;
    }
    if (!status_is_ignored(status)) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 3;
        status->MPI_TAG = 7;
        status->MPI_ERROR = 0;
        status->count_lo = 1;
    }
    *request = FAKE_NATIVE_NULL;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Testall(int count, int *requests, int *flag,
                                struct unimpi_status_legacy *statuses) {
    int i;
    int ignore;

    if (!flag) {
        return 13;
    }
    ignore = status_is_ignored(statuses);

    if (count == 2 && requests && !ignore) {
        *flag = 0;
        for (i = 0; i < count; ++i) {
            requests[i] = FAKE_NATIVE_NULL;
            memset(&statuses[i], 0, sizeof(statuses[i]));
            statuses[i].count_lo = 40 + i;
            statuses[i].MPI_SOURCE = 10 + i;
            statuses[i].MPI_TAG = 20 + i;
            statuses[i].MPI_ERROR = 17;
        }
        return FAKE_ERR_IN_STATUS_CODE;
    }

    if (count == 1 && requests) {
        if (requests[0] != FAKE_NATIVE_REQUEST) {
            return 19;
        }
        *flag = 0;
        return 0;
    }

    if (count <= 0) {
        *flag = 1;
        return 0;
    }
    return 13;
}

/* --- Representative NBC / RMA / I/O producers --- */

int UNIMPI_MPI_CALL MPI_Ibarrier(int comm, int *request) {
    (void)comm;
    if (!request) {
        return 19;
    }
    *request = FAKE_NATIVE_REQUEST;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Rput(const void *origin_addr, int origin_count,
                             int origin_datatype, int target_rank,
                             intptr_t target_disp, int target_count,
                             int target_datatype, int win, int *request) {
    (void)origin_addr;
    (void)origin_count;
    (void)origin_datatype;
    (void)target_rank;
    (void)target_disp;
    (void)target_count;
    (void)target_datatype;
    (void)win;
    if (!request) {
        return 19;
    }
    *request = FAKE_NATIVE_REQUEST;
    return 0;
}

int UNIMPI_MPI_CALL MPI_File_iread(struct ADIOI_FileD *fh, void *buf, int count,
                                   int datatype, int *request) {
    (void)fh;
    (void)buf;
    (void)count;
    (void)datatype;
    if (!request) {
        return 19;
    }
    *request = FAKE_NATIVE_REQUEST;
    return 0;
}

/* --- Matched message path --- */

int UNIMPI_MPI_CALL MPI_Mprobe(int source, int tag, int comm, int *message,
                               struct unimpi_status_legacy *status) {
    (void)comm;
    if (!message) {
        return 19;
    }
    if (g_fail_next_mprobe) {
        g_fail_next_mprobe = 0;
        *message = (int)0xdeadbeef;
        if (!status_is_ignored(status)) {
            status->MPI_SOURCE = 88;
            status->MPI_TAG = 88;
            status->count_lo = 88;
        }
        return FAKE_ERR_OTHER;
    }
    *message = FAKE_NATIVE_MESSAGE;
    g_message_live = 1;
    if (!status_is_ignored(status)) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = source >= 0 ? source : 0;
        status->MPI_TAG = tag >= 0 ? tag : 0;
        status->count_lo = 4;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Improbe(int source, int tag, int comm, int *flag,
                                int *message,
                                struct unimpi_status_legacy *status) {
    (void)source;
    (void)tag;
    (void)comm;
    if (!flag || !message) {
        return 13;
    }
    if (g_fail_next_improbe) {
        g_fail_next_improbe = 0;
        *flag = 1;
        *message = (int)0xdeadbeef;
        if (!status_is_ignored(status)) {
            status->MPI_SOURCE = 77;
            status->MPI_TAG = 77;
            status->count_lo = 77;
        }
        return FAKE_ERR_OTHER;
    }
    /* No match: leave *message and status untouched. */
    *flag = 0;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Mrecv(void *buf, int count, int datatype, int *message,
                              struct unimpi_status_legacy *status) {
    (void)buf;
    (void)datatype;
    if (!message || *message != FAKE_NATIVE_MESSAGE || !g_message_live) {
        return 19;
    }
    if (g_fail_next_mrecv) {
        g_fail_next_mrecv = 0;
        *message = (int)0xdeadbeef;
        if (!status_is_ignored(status)) {
            status->MPI_SOURCE = 66;
            status->MPI_TAG = 66;
            status->count_lo = 66;
        }
        return FAKE_ERR_OTHER;
    }
    if (!status_is_ignored(status)) {
        memset(status, 0, sizeof(*status));
        status->MPI_SOURCE = 1;
        status->MPI_TAG = 2;
        status->count_lo = count;
    }
    *message = FAKE_NATIVE_NULL;
    g_message_live = 0;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Imrecv(void *buf, int count, int datatype, int *message,
                               int *request) {
    (void)buf;
    (void)count;
    (void)datatype;
    if (!message || !request || *message != FAKE_NATIVE_MESSAGE ||
        !g_message_live) {
        return 19;
    }
    *message = FAKE_NATIVE_NULL;
    *request = FAKE_NATIVE_REQUEST;
    g_message_live = 0;
    return 0;
}
