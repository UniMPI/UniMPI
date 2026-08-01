/*
 * Single-request / message adapters for MPICH, Intel MPI, and MS-MPI.
 *
 * Facade handles are intptr_t. Integer backends use native C types:
 *   int for Comm, Datatype, Op, Win, Request, Message
 *   struct ADIOI_FileD * for File
 *   intptr_t-sized Aint and long long Offset on supported 64-bit ABIs
 *
 * Every by-value handle is converted at the call boundary so dlsym-bound
 * function types match the real library, not the UniMPI facade aliases.
 * Array completion paths remain in request_array_wrappers.c.
 */
#include "request_handle_wrappers.h"
#include "request_array_wrappers.h"
#include "unimpi.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Integer-backend native ABI aliases (authoritative vendor mpi.h)            */
/* ------------------------------------------------------------------------- */

typedef int unimpi_ih_comm_t;
typedef int unimpi_ih_datatype_t;
typedef int unimpi_ih_op_t;
typedef int unimpi_ih_win_t;
typedef int unimpi_ih_request_t;
typedef int unimpi_ih_message_t;
struct ADIOI_FileD;
typedef struct ADIOI_FileD *unimpi_ih_file_t;
/* LP64: matches MPI_Aint as pointer-sized integer across the three backends. */
typedef intptr_t unimpi_ih_aint_t;
/* Matches MPI_Offset as 64-bit signed on the supported 64-bit ABIs. */
typedef long long unimpi_ih_offset_t;

/* ------------------------------------------------------------------------- */
/* Canonical request/message conversion                                      */
/* ------------------------------------------------------------------------- */

MPI_Request unimpi_request_from_native(int native) {
    return (MPI_Request)(intptr_t)native;
}

int unimpi_request_to_native(MPI_Request facade) {
    return (int)(intptr_t)facade;
}

MPI_Message unimpi_message_from_native(int native) {
    return (MPI_Message)(intptr_t)native;
}

int unimpi_message_to_native(MPI_Message facade) {
    return (int)(intptr_t)facade;
}

static unimpi_ih_comm_t ih_comm(MPI_Comm facade) {
    return (unimpi_ih_comm_t)(intptr_t)facade;
}

static unimpi_ih_datatype_t ih_datatype(MPI_Datatype facade) {
    return (unimpi_ih_datatype_t)(intptr_t)facade;
}

static unimpi_ih_op_t ih_op(MPI_Op facade) {
    return (unimpi_ih_op_t)(intptr_t)facade;
}

static unimpi_ih_win_t ih_win(MPI_Win facade) {
    return (unimpi_ih_win_t)(intptr_t)facade;
}

static unimpi_ih_file_t ih_file(MPI_File facade) {
    return (unimpi_ih_file_t)(intptr_t)facade;
}

static unimpi_ih_aint_t ih_aint(MPI_Aint facade) {
    return (unimpi_ih_aint_t)facade;
}

static unimpi_ih_offset_t ih_offset(MPI_Offset facade) {
    return (unimpi_ih_offset_t)facade;
}

static void store_facade_request(MPI_Request *request, int native) {
    if (request) {
        *request = unimpi_request_from_native(native);
    }
}

static void store_facade_message(MPI_Message *message, int native) {
    if (message) {
        *message = unimpi_message_from_native(native);
    }
}

static int request_arg_error(void) {
    return MPI_ERR_REQUEST != MPI_SUCCESS ? MPI_ERR_REQUEST : 19;
}

static int no_memory_error(void) {
    return MPI_ERR_NO_MEM != MPI_SUCCESS ? MPI_ERR_NO_MEM : 39;
}

static int call_succeeded(int result) {
    return result == MPI_SUCCESS;
}

static int status_is_ignored(const MPI_Status *status) {
    return status == NULL || status == UNIMPI_STATUSES_IGNORE;
}

/* Five-int native status stride; never sizeof incomplete struct MPI_Status. */
enum {
    UNIMPI_IH_STATUS_BYTES = sizeof(struct unimpi_status_legacy)
};

static struct MPI_Status *native_status_ignore(void) {
    return (struct MPI_Status *)UNIMPI_STATUSES_IGNORE;
}

static struct MPI_Status *as_native_status(void *cell) {
    return (struct MPI_Status *)cell;
}

static void *allocate_native_status_cell(void) {
    return calloc(1, UNIMPI_IH_STATUS_BYTES);
}

static void store_native_status_to_facade(const void *cell, MPI_Status *status) {
    if (!status || status_is_ignored(status) || !cell) {
        return;
    }
    memset(status, 0, sizeof(*status));
    memcpy(&status->legacy, cell, UNIMPI_IH_STATUS_BYTES);
}

/* ------------------------------------------------------------------------- */
/* Native function pointer types (integer-backend ABI)                       */
/* ------------------------------------------------------------------------- */

typedef int (UNIMPI_MPI_CALL *native_p2p_req_fn)(
    const void *, int, unimpi_ih_datatype_t, int, int, unimpi_ih_comm_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_recv_req_fn)(
    void *, int, unimpi_ih_datatype_t, int, int, unimpi_ih_comm_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_wait_fn)(
    unimpi_ih_request_t *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *native_test_fn)(
    unimpi_ih_request_t *, int *, struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *native_req_only_fn)(unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_imrecv_fn)(
    void *, int, unimpi_ih_datatype_t, unimpi_ih_message_t *,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_mprobe_fn)(
    int, int, unimpi_ih_comm_t, unimpi_ih_message_t *,
    struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *native_improbe_fn)(
    int, int, unimpi_ih_comm_t, int *, unimpi_ih_message_t *,
    struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *native_mrecv_fn)(
    void *, int, unimpi_ih_datatype_t, unimpi_ih_message_t *,
    struct MPI_Status *);
typedef int (UNIMPI_MPI_CALL *native_ibarrier_fn)(
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_ibcast_fn)(
    void *, int, unimpi_ih_datatype_t, int, unimpi_ih_comm_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_igather_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, int, unimpi_ih_datatype_t,
    int, unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_igatherv_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, const int *, const int *,
    unimpi_ih_datatype_t, int, unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_iscatter_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, int, unimpi_ih_datatype_t,
    int, unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_iscatterv_fn)(
    const void *, const int *, const int *, unimpi_ih_datatype_t, void *, int,
    unimpi_ih_datatype_t, int, unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_iallgather_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, int, unimpi_ih_datatype_t,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_iallgatherv_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, const int *, const int *,
    unimpi_ih_datatype_t, unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_ialltoall_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, int, unimpi_ih_datatype_t,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_ialltoallv_fn)(
    const void *, const int *, const int *, unimpi_ih_datatype_t, void *,
    const int *, const int *, unimpi_ih_datatype_t, unimpi_ih_comm_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_ireduce_fn)(
    const void *, void *, int, unimpi_ih_datatype_t, unimpi_ih_op_t, int,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_iallreduce_fn)(
    const void *, void *, int, unimpi_ih_datatype_t, unimpi_ih_op_t,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_ireduce_scatter_fn)(
    const void *, void *, const int *, unimpi_ih_datatype_t, unimpi_ih_op_t,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_ireduce_scatter_block_fn)(
    const void *, void *, int, unimpi_ih_datatype_t, unimpi_ih_op_t,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_iscan_fn)(
    const void *, void *, int, unimpi_ih_datatype_t, unimpi_ih_op_t,
    unimpi_ih_comm_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_rput_fn)(
    const void *, int, unimpi_ih_datatype_t, int, unimpi_ih_aint_t, int,
    unimpi_ih_datatype_t, unimpi_ih_win_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_rget_fn)(
    void *, int, unimpi_ih_datatype_t, int, unimpi_ih_aint_t, int,
    unimpi_ih_datatype_t, unimpi_ih_win_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_raccumulate_fn)(
    const void *, int, unimpi_ih_datatype_t, int, unimpi_ih_aint_t, int,
    unimpi_ih_datatype_t, unimpi_ih_op_t, unimpi_ih_win_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_rget_accumulate_fn)(
    const void *, int, unimpi_ih_datatype_t, void *, int, unimpi_ih_datatype_t,
    int, unimpi_ih_aint_t, int, unimpi_ih_datatype_t, unimpi_ih_op_t,
    unimpi_ih_win_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_file_iread_fn)(
    unimpi_ih_file_t, void *, int, unimpi_ih_datatype_t, unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_file_iwrite_fn)(
    unimpi_ih_file_t, const void *, int, unimpi_ih_datatype_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_file_iread_at_fn)(
    unimpi_ih_file_t, unimpi_ih_offset_t, void *, int, unimpi_ih_datatype_t,
    unimpi_ih_request_t *);
typedef int (UNIMPI_MPI_CALL *native_file_iwrite_at_fn)(
    unimpi_ih_file_t, unimpi_ih_offset_t, const void *, int,
    unimpi_ih_datatype_t, unimpi_ih_request_t *);

/* ------------------------------------------------------------------------- */
/* Native storage                                                            */
/* ------------------------------------------------------------------------- */

static native_p2p_req_fn real_isend;
static native_recv_req_fn real_irecv;
static native_wait_fn real_wait;
static native_test_fn real_test;
static native_p2p_req_fn real_ssend_init;
static native_p2p_req_fn real_bsend_init;
static native_p2p_req_fn real_rsend_init;
static native_p2p_req_fn real_send_init;
static native_recv_req_fn real_recv_init;
static native_req_only_fn real_start;
static native_req_only_fn real_request_free;
static native_req_only_fn real_cancel;
static native_imrecv_fn real_imrecv;
static native_mprobe_fn real_mprobe;
static native_improbe_fn real_improbe;
static native_mrecv_fn real_mrecv;
static native_ibarrier_fn real_ibarrier;
static native_ibcast_fn real_ibcast;
static native_igather_fn real_igather;
static native_igatherv_fn real_igatherv;
static native_iscatter_fn real_iscatter;
static native_iscatterv_fn real_iscatterv;
static native_iallgather_fn real_iallgather;
static native_iallgatherv_fn real_iallgatherv;
static native_ialltoall_fn real_ialltoall;
static native_ialltoallv_fn real_ialltoallv;
static native_ireduce_fn real_ireduce;
static native_iallreduce_fn real_iallreduce;
static native_ireduce_scatter_fn real_ireduce_scatter;
static native_ireduce_scatter_block_fn real_ireduce_scatter_block;
static native_iscan_fn real_iscan;
static native_iscan_fn real_iexscan;
static native_rput_fn real_rput;
static native_rget_fn real_rget;
static native_raccumulate_fn real_raccumulate;
static native_rget_accumulate_fn real_rget_accumulate;
static native_file_iread_fn real_file_iread;
static native_file_iwrite_fn real_file_iwrite;
static native_file_iread_at_fn real_file_iread_at;
static native_file_iwrite_at_fn real_file_iwrite_at;

/* ------------------------------------------------------------------------- */
/* Wrappers                                                                  */
/* ------------------------------------------------------------------------- */

int unimpi_wrap_isend(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_isend(
        buf, count, ih_datatype(datatype), dest, tag, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_irecv(void *buf, int count, MPI_Datatype datatype,
                      int source, int tag, MPI_Comm comm,
                      MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_irecv(
        buf, count, ih_datatype(datatype), source, tag, ih_comm(comm),
        &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_wait(MPI_Request *request, MPI_Status *status) {
    unimpi_ih_request_t native;
    int result;
    int ignore;
    void *native_status_cell = NULL;

    if (!request) {
        return request_arg_error();
    }
    native = (unimpi_ih_request_t)unimpi_request_to_native(*request);
    ignore = status_is_ignored(status);
    if (!ignore) {
        native_status_cell = allocate_native_status_cell();
        if (!native_status_cell) {
            return no_memory_error();
        }
    }
    result = real_wait(
        &native, ignore ? native_status_ignore() : as_native_status(native_status_cell));
    if (call_succeeded(result)) {
        store_facade_request(request, native);
        if (!ignore) {
            store_native_status_to_facade(native_status_cell, status);
        }
    }
    free(native_status_cell);
    return result;
}

int unimpi_wrap_test(MPI_Request *request, int *flag, MPI_Status *status) {
    unimpi_ih_request_t native;
    int result;
    int ignore;
    void *native_status_cell = NULL;

    if (!request) {
        return request_arg_error();
    }
    native = (unimpi_ih_request_t)unimpi_request_to_native(*request);
    ignore = status_is_ignored(status);
    if (!ignore) {
        native_status_cell = allocate_native_status_cell();
        if (!native_status_cell) {
            return no_memory_error();
        }
    }
    result = real_test(
        &native, flag, ignore ? native_status_ignore() : as_native_status(native_status_cell));
    if (call_succeeded(result)) {
        store_facade_request(request, native);
        /* Status is defined only when the request completed. */
        if (!ignore && flag && *flag) {
            store_native_status_to_facade(native_status_cell, status);
        }
    }
    free(native_status_cell);
    return result;
}

int unimpi_wrap_ssend_init(const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm,
                           MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ssend_init(
        buf, count, ih_datatype(datatype), dest, tag, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_bsend_init(const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm,
                           MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_bsend_init(
        buf, count, ih_datatype(datatype), dest, tag, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_rsend_init(const void *buf, int count, MPI_Datatype datatype,
                           int dest, int tag, MPI_Comm comm,
                           MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_rsend_init(
        buf, count, ih_datatype(datatype), dest, tag, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_send_init(const void *buf, int count, MPI_Datatype datatype,
                          int dest, int tag, MPI_Comm comm,
                          MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_send_init(
        buf, count, ih_datatype(datatype), dest, tag, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_recv_init(void *buf, int count, MPI_Datatype datatype,
                          int source, int tag, MPI_Comm comm,
                          MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_recv_init(
        buf, count, ih_datatype(datatype), source, tag, ih_comm(comm),
        &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_start(MPI_Request *request) {
    unimpi_ih_request_t native;
    int result;

    if (!request) {
        return request_arg_error();
    }
    native = (unimpi_ih_request_t)unimpi_request_to_native(*request);
    result = real_start(&native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_request_free(MPI_Request *request) {
    unimpi_ih_request_t native;
    int result;

    if (!request) {
        return request_arg_error();
    }
    native = (unimpi_ih_request_t)unimpi_request_to_native(*request);
    result = real_request_free(&native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_cancel(MPI_Request *request) {
    unimpi_ih_request_t native;
    int result;

    if (!request) {
        return request_arg_error();
    }
    native = (unimpi_ih_request_t)unimpi_request_to_native(*request);
    result = real_cancel(&native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_imrecv(void *buf, int count, MPI_Datatype datatype,
                       MPI_Message *message, MPI_Request *request) {
    unimpi_ih_message_t native_message;
    unimpi_ih_request_t native_request = 0;
    int result;

    if (!message || !request) {
        return request_arg_error();
    }
    native_message =
        (unimpi_ih_message_t)unimpi_message_to_native(*message);
    result = real_imrecv(
        buf, count, ih_datatype(datatype), &native_message, &native_request);
    if (call_succeeded(result)) {
        store_facade_message(message, native_message);
        store_facade_request(request, native_request);
    }
    return result;
}

int unimpi_wrap_mprobe(int source, int tag, MPI_Comm comm,
                       MPI_Message *message, MPI_Status *status) {
    unimpi_ih_message_t native_message = 0;
    int result;
    int ignore;
    void *native_status_cell = NULL;

    if (!message) {
        return request_arg_error();
    }
    ignore = status_is_ignored(status);
    if (!ignore) {
        native_status_cell = allocate_native_status_cell();
        if (!native_status_cell) {
            return no_memory_error();
        }
    }
    result = real_mprobe(
        source, tag, ih_comm(comm), &native_message,
        ignore ? native_status_ignore() : as_native_status(native_status_cell));
    if (call_succeeded(result)) {
        store_facade_message(message, native_message);
        if (!ignore) {
            store_native_status_to_facade(native_status_cell, status);
        }
    }
    free(native_status_cell);
    return result;
}

int unimpi_wrap_improbe(int source, int tag, MPI_Comm comm, int *flag,
                        MPI_Message *message, MPI_Status *status) {
    unimpi_ih_message_t native_message = 0;
    int result;
    int ignore;
    void *native_status_cell = NULL;

    if (!flag || !message) {
        return request_arg_error();
    }
    ignore = status_is_ignored(status);
    if (!ignore) {
        native_status_cell = allocate_native_status_cell();
        if (!native_status_cell) {
            return no_memory_error();
        }
    }
    result = real_improbe(
        source, tag, ih_comm(comm), flag, &native_message,
        ignore ? native_status_ignore() : as_native_status(native_status_cell));
    if (call_succeeded(result) && flag && *flag) {
        store_facade_message(message, native_message);
        if (!ignore) {
            store_native_status_to_facade(native_status_cell, status);
        }
    }
    /* flag false or failed call: leave caller message and status alone. */
    free(native_status_cell);
    return result;
}

int unimpi_wrap_mrecv(void *buf, int count, MPI_Datatype datatype,
                      MPI_Message *message, MPI_Status *status) {
    unimpi_ih_message_t native_message;
    int result;
    int ignore;
    void *native_status_cell = NULL;

    if (!message) {
        return request_arg_error();
    }
    native_message =
        (unimpi_ih_message_t)unimpi_message_to_native(*message);
    ignore = status_is_ignored(status);
    if (!ignore) {
        native_status_cell = allocate_native_status_cell();
        if (!native_status_cell) {
            return no_memory_error();
        }
    }
    result = real_mrecv(
        buf, count, ih_datatype(datatype), &native_message,
        ignore ? native_status_ignore() : as_native_status(native_status_cell));
    if (call_succeeded(result)) {
        store_facade_message(message, native_message);
        if (!ignore) {
            store_native_status_to_facade(native_status_cell, status);
        }
    }
    free(native_status_cell);
    return result;
}

int unimpi_wrap_ibarrier(MPI_Comm comm, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ibarrier(ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_ibcast(void *buffer, int count, MPI_Datatype datatype,
                       int root, MPI_Comm comm, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ibcast(
        buffer, count, ih_datatype(datatype), root, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_igather(const void *sendbuf, int sendcount,
                        MPI_Datatype sendtype, void *recvbuf, int recvcount,
                        MPI_Datatype recvtype, int root, MPI_Comm comm,
                        MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_igather(
        sendbuf, sendcount, ih_datatype(sendtype), recvbuf, recvcount,
        ih_datatype(recvtype), root, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_igatherv(const void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf,
                         const int *recvcounts, const int *displs,
                         MPI_Datatype recvtype, int root, MPI_Comm comm,
                         MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_igatherv(
        sendbuf, sendcount, ih_datatype(sendtype), recvbuf, recvcounts, displs,
        ih_datatype(recvtype), root, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iscatter(const void *sendbuf, int sendcount,
                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                         MPI_Datatype recvtype, int root, MPI_Comm comm,
                         MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iscatter(
        sendbuf, sendcount, ih_datatype(sendtype), recvbuf, recvcount,
        ih_datatype(recvtype), root, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iscatterv(const void *sendbuf, const int *sendcounts,
                          const int *displs, MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                          int root, MPI_Comm comm, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iscatterv(
        sendbuf, sendcounts, displs, ih_datatype(sendtype), recvbuf, recvcount,
        ih_datatype(recvtype), root, ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iallgather(const void *sendbuf, int sendcount,
                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                           MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iallgather(
        sendbuf, sendcount, ih_datatype(sendtype), recvbuf, recvcount,
        ih_datatype(recvtype), ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iallgatherv(const void *sendbuf, int sendcount,
                            MPI_Datatype sendtype, void *recvbuf,
                            const int *recvcounts, const int *displs,
                            MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iallgatherv(
        sendbuf, sendcount, ih_datatype(sendtype), recvbuf, recvcounts, displs,
        ih_datatype(recvtype), ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_ialltoall(const void *sendbuf, int sendcount,
                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                          MPI_Datatype recvtype, MPI_Comm comm,
                          MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ialltoall(
        sendbuf, sendcount, ih_datatype(sendtype), recvbuf, recvcount,
        ih_datatype(recvtype), ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_ialltoallv(const void *sendbuf, const int *sendcounts,
                           const int *sdispls, MPI_Datatype sendtype,
                           void *recvbuf, const int *recvcounts,
                           const int *rdispls, MPI_Datatype recvtype,
                           MPI_Comm comm, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ialltoallv(
        sendbuf, sendcounts, sdispls, ih_datatype(sendtype), recvbuf,
        recvcounts, rdispls, ih_datatype(recvtype), ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_ireduce(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, int root,
                        MPI_Comm comm, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ireduce(
        sendbuf, recvbuf, count, ih_datatype(datatype), ih_op(op), root,
        ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iallreduce(const void *sendbuf, void *recvbuf, int count,
                           MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                           MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iallreduce(
        sendbuf, recvbuf, count, ih_datatype(datatype), ih_op(op),
        ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                const int *recvcounts, MPI_Datatype datatype,
                                MPI_Op op, MPI_Comm comm,
                                MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ireduce_scatter(
        sendbuf, recvbuf, recvcounts, ih_datatype(datatype), ih_op(op),
        ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                      int recvcount, MPI_Datatype datatype,
                                      MPI_Op op, MPI_Comm comm,
                                      MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_ireduce_scatter_block(
        sendbuf, recvbuf, recvcount, ih_datatype(datatype), ih_op(op),
        ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iscan(const void *sendbuf, void *recvbuf, int count,
                      MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                      MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iscan(
        sendbuf, recvbuf, count, ih_datatype(datatype), ih_op(op),
        ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_iexscan(const void *sendbuf, void *recvbuf, int count,
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                        MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_iexscan(
        sendbuf, recvbuf, count, ih_datatype(datatype), ih_op(op),
        ih_comm(comm), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_rput(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Win win,
                     MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_rput(
        origin_addr, origin_count, ih_datatype(origin_datatype), target_rank,
        ih_aint(target_disp), target_count, ih_datatype(target_datatype),
        ih_win(win), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_rget(void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank,
                     MPI_Aint target_disp, int target_count,
                     MPI_Datatype target_datatype, MPI_Win win,
                     MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_rget(
        origin_addr, origin_count, ih_datatype(origin_datatype), target_rank,
        ih_aint(target_disp), target_count, ih_datatype(target_datatype),
        ih_win(win), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_raccumulate(const void *origin_addr, int origin_count,
                            MPI_Datatype origin_datatype, int target_rank,
                            MPI_Aint target_disp, int target_count,
                            MPI_Datatype target_datatype, MPI_Op op,
                            MPI_Win win, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_raccumulate(
        origin_addr, origin_count, ih_datatype(origin_datatype), target_rank,
        ih_aint(target_disp), target_count, ih_datatype(target_datatype),
        ih_op(op), ih_win(win), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_rget_accumulate(const void *origin_addr, int origin_count,
                                MPI_Datatype origin_datatype,
                                void *result_addr, int result_count,
                                MPI_Datatype result_datatype, int target_rank,
                                MPI_Aint target_disp, int target_count,
                                MPI_Datatype target_datatype, MPI_Op op,
                                MPI_Win win, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_rget_accumulate(
        origin_addr, origin_count, ih_datatype(origin_datatype), result_addr,
        result_count, ih_datatype(result_datatype), target_rank,
        ih_aint(target_disp), target_count, ih_datatype(target_datatype),
        ih_op(op), ih_win(win), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_file_iread(MPI_File fh, void *buf, int count,
                           MPI_Datatype datatype, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_file_iread(
        ih_file(fh), buf, count, ih_datatype(datatype), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_file_iwrite(MPI_File fh, const void *buf, int count,
                            MPI_Datatype datatype, MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_file_iwrite(
        ih_file(fh), buf, count, ih_datatype(datatype), &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_file_iread_at(MPI_File fh, MPI_Offset offset, void *buf,
                              int count, MPI_Datatype datatype,
                              MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_file_iread_at(
        ih_file(fh), ih_offset(offset), buf, count, ih_datatype(datatype),
        &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_file_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf,
                               int count, MPI_Datatype datatype,
                               MPI_Request *request) {
    unimpi_ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_file_iwrite_at(
        ih_file(fh), ih_offset(offset), buf, count, ih_datatype(datatype),
        &native);
    if (call_succeeded(result)) {
        store_facade_request(request, native);
    }
    return result;
}

/* ------------------------------------------------------------------------- */
/* Binding                                                                   */
/* ------------------------------------------------------------------------- */

#define BIND_OPTIONAL(field, wrap, native_type, symbol)                        \
    do {                                                                      \
        native_type fn =                                                      \
            (native_type)unimpi_platform_dlsym(handle, symbol);               \
        if (fn) {                                                             \
            real_##field = fn;                                                \
            unimpi.field = wrap;                                              \
        } else {                                                              \
            real_##field = NULL;                                              \
            unimpi.field = NULL;                                              \
        }                                                                     \
    } while (0)

#define BIND_ARRAY(field, wrap, set_fn, native_type, symbol)                   \
    do {                                                                      \
        native_type fn =                                                      \
            (native_type)unimpi_platform_dlsym(handle, symbol);               \
        if (fn) {                                                             \
            set_fn(fn);                                                       \
            unimpi.field = wrap;                                              \
        } else {                                                              \
            set_fn(NULL);                                                     \
            unimpi.field = NULL;                                              \
        }                                                                     \
    } while (0)

void unimpi_bind_integer_request_apis(unimpi_lib_handle_t handle) {
    BIND_OPTIONAL(isend, unimpi_wrap_isend, native_p2p_req_fn, "MPI_Isend");
    BIND_OPTIONAL(irecv, unimpi_wrap_irecv, native_recv_req_fn, "MPI_Irecv");
    BIND_OPTIONAL(wait, unimpi_wrap_wait, native_wait_fn, "MPI_Wait");
    BIND_OPTIONAL(test, unimpi_wrap_test, native_test_fn, "MPI_Test");

    BIND_ARRAY(waitall, unimpi_wrap_waitall, unimpi_wrapper_set_waitall,
               unimpi_native_legacy_waitall_fn, "MPI_Waitall");
    BIND_ARRAY(testany, unimpi_wrap_testany, unimpi_wrapper_set_testany,
               unimpi_native_legacy_any_fn, "MPI_Testany");
    BIND_ARRAY(testsome, unimpi_wrap_testsome, unimpi_wrapper_set_testsome,
               unimpi_native_legacy_some_fn, "MPI_Testsome");
    BIND_ARRAY(testall, unimpi_wrap_testall, unimpi_wrapper_set_testall,
               unimpi_native_legacy_testall_fn, "MPI_Testall");
    BIND_ARRAY(waitany, unimpi_wrap_waitany, unimpi_wrapper_set_waitany,
               unimpi_native_legacy_waitany_fn, "MPI_Waitany");
    BIND_ARRAY(waitsome, unimpi_wrap_waitsome, unimpi_wrapper_set_waitsome,
               unimpi_native_legacy_some_fn, "MPI_Waitsome");
    BIND_ARRAY(startall, unimpi_wrap_startall, unimpi_wrapper_set_startall,
               unimpi_native_legacy_startall_fn, "MPI_Startall");

    BIND_OPTIONAL(ssend_init, unimpi_wrap_ssend_init, native_p2p_req_fn,
                  "MPI_Ssend_init");
    BIND_OPTIONAL(bsend_init, unimpi_wrap_bsend_init, native_p2p_req_fn,
                  "MPI_Bsend_init");
    BIND_OPTIONAL(rsend_init, unimpi_wrap_rsend_init, native_p2p_req_fn,
                  "MPI_Rsend_init");
    BIND_OPTIONAL(send_init, unimpi_wrap_send_init, native_p2p_req_fn,
                  "MPI_Send_init");
    BIND_OPTIONAL(recv_init, unimpi_wrap_recv_init, native_recv_req_fn,
                  "MPI_Recv_init");
    BIND_OPTIONAL(start, unimpi_wrap_start, native_req_only_fn, "MPI_Start");
    BIND_OPTIONAL(request_free, unimpi_wrap_request_free, native_req_only_fn,
                  "MPI_Request_free");
    BIND_OPTIONAL(cancel, unimpi_wrap_cancel, native_req_only_fn,
                  "MPI_Cancel");
    BIND_OPTIONAL(imrecv, unimpi_wrap_imrecv, native_imrecv_fn, "MPI_Imrecv");
    BIND_OPTIONAL(mprobe, unimpi_wrap_mprobe, native_mprobe_fn, "MPI_Mprobe");
    BIND_OPTIONAL(improbe, unimpi_wrap_improbe, native_improbe_fn,
                  "MPI_Improbe");
    BIND_OPTIONAL(mrecv, unimpi_wrap_mrecv, native_mrecv_fn, "MPI_Mrecv");

    BIND_OPTIONAL(ibarrier, unimpi_wrap_ibarrier, native_ibarrier_fn,
                  "MPI_Ibarrier");
    BIND_OPTIONAL(ibcast, unimpi_wrap_ibcast, native_ibcast_fn, "MPI_Ibcast");
    BIND_OPTIONAL(igather, unimpi_wrap_igather, native_igather_fn,
                  "MPI_Igather");
    BIND_OPTIONAL(igatherv, unimpi_wrap_igatherv, native_igatherv_fn,
                  "MPI_Igatherv");
    BIND_OPTIONAL(iscatter, unimpi_wrap_iscatter, native_iscatter_fn,
                  "MPI_Iscatter");
    BIND_OPTIONAL(iscatterv, unimpi_wrap_iscatterv, native_iscatterv_fn,
                  "MPI_Iscatterv");
    BIND_OPTIONAL(iallgather, unimpi_wrap_iallgather, native_iallgather_fn,
                  "MPI_Iallgather");
    BIND_OPTIONAL(iallgatherv, unimpi_wrap_iallgatherv, native_iallgatherv_fn,
                  "MPI_Iallgatherv");
    BIND_OPTIONAL(ialltoall, unimpi_wrap_ialltoall, native_ialltoall_fn,
                  "MPI_Ialltoall");
    BIND_OPTIONAL(ialltoallv, unimpi_wrap_ialltoallv, native_ialltoallv_fn,
                  "MPI_Ialltoallv");
    unimpi.ialltoallw = NULL;
    BIND_OPTIONAL(ireduce, unimpi_wrap_ireduce, native_ireduce_fn,
                  "MPI_Ireduce");
    BIND_OPTIONAL(iallreduce, unimpi_wrap_iallreduce, native_iallreduce_fn,
                  "MPI_Iallreduce");
    BIND_OPTIONAL(ireduce_scatter, unimpi_wrap_ireduce_scatter,
                  native_ireduce_scatter_fn, "MPI_Ireduce_scatter");
    BIND_OPTIONAL(ireduce_scatter_block, unimpi_wrap_ireduce_scatter_block,
                  native_ireduce_scatter_block_fn,
                  "MPI_Ireduce_scatter_block");
    BIND_OPTIONAL(iscan, unimpi_wrap_iscan, native_iscan_fn, "MPI_Iscan");
    BIND_OPTIONAL(iexscan, unimpi_wrap_iexscan, native_iscan_fn, "MPI_Iexscan");

    BIND_OPTIONAL(rput, unimpi_wrap_rput, native_rput_fn, "MPI_Rput");
    BIND_OPTIONAL(rget, unimpi_wrap_rget, native_rget_fn, "MPI_Rget");
    BIND_OPTIONAL(raccumulate, unimpi_wrap_raccumulate, native_raccumulate_fn,
                  "MPI_Raccumulate");
    BIND_OPTIONAL(rget_accumulate, unimpi_wrap_rget_accumulate,
                  native_rget_accumulate_fn, "MPI_Rget_accumulate");

    BIND_OPTIONAL(file_iread, unimpi_wrap_file_iread, native_file_iread_fn,
                  "MPI_File_iread");
    BIND_OPTIONAL(file_iwrite, unimpi_wrap_file_iwrite, native_file_iwrite_fn,
                  "MPI_File_iwrite");
    BIND_OPTIONAL(file_iread_at, unimpi_wrap_file_iread_at,
                  native_file_iread_at_fn, "MPI_File_iread_at");
    BIND_OPTIONAL(file_iwrite_at, unimpi_wrap_file_iwrite_at,
                  native_file_iwrite_at_fn, "MPI_File_iwrite_at");
}

#undef BIND_OPTIONAL
#undef BIND_ARRAY
