/* Fake integer-handle MPI DSO for production binder regressions.
 *
 * Exports UNIMPI_MPI_CALL natives with signatures that match the integer
 * backend adapter typedefs exactly (including status pointer types) so
 * dlsym-bound calls pass C strict function-type checks under UBSan.
 *
 * High-bit request/message handles ensure stale facade high bytes fail
 * equality. Core lifecycle symbols satisfy backend validate_core.
 */
#include <stdint.h>
#include <string.h>

#include "unimpi_platform.h"
#include "unimpi_vtable.h"

enum {
    FAKE_NATIVE_REQUEST = (int)0xac000001,
    FAKE_NATIVE_MESSAGE = (int)0xad000002,
    FAKE_NATIVE_NULL = (int)0x2c000000,
    FAKE_ERR_IN_STATUS_CODE = 0x10000 | 17
};

static int g_started;
static int g_message_live;

static int status_is_ignored(const struct unimpi_status_legacy *status) {
    return status == NULL ||
        (const void *)status == (const void *)UNIMPI_STATUSES_IGNORE;
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

int UNIMPI_MPI_CALL MPI_Comm_size(MPI_Comm comm, int *size) {
    (void)comm;
    if (size) {
        *size = 1;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_rank(MPI_Comm comm, int *rank) {
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

/* --- Request producers / completion --- */

int UNIMPI_MPI_CALL MPI_Irecv(void *buf, int count, MPI_Datatype datatype,
                              int source, int tag, MPI_Comm comm,
                              int *request) {
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

int UNIMPI_MPI_CALL MPI_Send_init(const void *buf, int count,
                                  MPI_Datatype datatype, int dest, int tag,
                                  MPI_Comm comm, int *request) {
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

/* Signature must match unimpi_native_legacy_testall_fn exactly. */
int UNIMPI_MPI_CALL MPI_Testall(int count, int *requests, int *flag,
                                struct unimpi_status_legacy *statuses) {
    int i;
    int ignore;

    if (!flag) {
        return 13;
    }
    ignore = status_is_ignored(statuses);

    /* Two-request path: post-init ERR_IN_STATUS status copyback. */
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

    /* Single incomplete request: preserve high-bit handle. */
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

int UNIMPI_MPI_CALL MPI_Ibarrier(MPI_Comm comm, int *request) {
    (void)comm;
    if (!request) {
        return 19;
    }
    *request = FAKE_NATIVE_REQUEST;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Rput(const void *origin_addr, int origin_count,
                             MPI_Datatype origin_datatype, int target_rank,
                             MPI_Aint target_disp, int target_count,
                             MPI_Datatype target_datatype, MPI_Win win,
                             int *request) {
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

int UNIMPI_MPI_CALL MPI_File_iread(MPI_File fh, void *buf, int count,
                                   MPI_Datatype datatype, int *request) {
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

int UNIMPI_MPI_CALL MPI_Mprobe(int source, int tag, MPI_Comm comm,
                               int *message,
                               struct unimpi_status_legacy *status) {
    (void)comm;
    if (!message) {
        return 19;
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

int UNIMPI_MPI_CALL MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag,
                                int *message,
                                struct unimpi_status_legacy *status) {
    (void)source;
    (void)tag;
    (void)comm;
    (void)status;
    if (!flag || !message) {
        return 13;
    }
    /* No match: leave *message untouched. */
    *flag = 0;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Mrecv(void *buf, int count, MPI_Datatype datatype,
                              int *message,
                              struct unimpi_status_legacy *status) {
    (void)buf;
    (void)datatype;
    if (!message || *message != FAKE_NATIVE_MESSAGE || !g_message_live) {
        return 19;
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

int UNIMPI_MPI_CALL MPI_Imrecv(void *buf, int count, MPI_Datatype datatype,
                               int *message, int *request) {
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
