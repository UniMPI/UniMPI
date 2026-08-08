/* src/backends/request_handle_wrappers.c
 * Zero-allocation by-value adapters for single-request MPI operations on
 * integer-handle backends.
 *
 * MPICH, Intel MPI, and MS-MPI store MPI_Request as 32-bit native int, while
 * UniMPI uses 8-byte intptr_t. For by-value and single output handles this
 * matters at two boundary points: arguments are narrowed to the native type
 * at the call site, and an output request is received into a local native int
 * before being widened to a full facade value (so the high bytes are never
 * left as stale stack garbage).
 *
 * No heap allocation: conversions are scalar, using only local native temps.
 */

#include "request_handle_wrappers.h"
#include "unimpi_errors.h"

#include <stdint.h>

/* ---- Integer-backend native ABI aliases ---- */
typedef int32_t ih_datatype_t;   /* native MPI_Datatype */
typedef int32_t ih_comm_t;       /* native MPI_Comm */
typedef int32_t ih_request_t;    /* native MPI_Request */

/* ---- Real backend entry points (set during backend init) ---- */
static int (*real_isend)(const void *, int, int, int, int, int, int *);
static int (*real_irecv)(void *, int, int, int, int, int, int *);

/* ---- Setters ---- */
int unimpi_ih_set_isend(int (*fn)(const void *, int, int, int, int, int,
                                  int *)) {
    if (fn)
        real_isend = fn;
    return fn != NULL;
}
int unimpi_ih_set_irecv(int (*fn)(void *, int, int, int, int, int, int *)) {
    if (fn)
        real_irecv = fn;
    return fn != NULL;
}

/* ---- Canonical conversion helpers (matching C narrowing of intptr_t) ---- */
static ih_datatype_t ih_datatype(MPI_Datatype facade) {
    return (ih_datatype_t)(intptr_t)facade;
}
static ih_comm_t ih_comm(MPI_Comm facade) {
    return (ih_comm_t)(intptr_t)facade;
}
static void store_facade_request(MPI_Request *request, int native) {
    if (request) {
        *request = (MPI_Request)(intptr_t)native;
    }
}
static int request_arg_error(void) {
    return MPI_ERR_REQUEST != MPI_SUCCESS ? MPI_ERR_REQUEST : 19;
}

/* ---- Wrapper implementations ---- */

int unimpi_wrap_isend(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_isend(
        buf, count, ih_datatype(datatype), dest, tag, ih_comm(comm), &native);
    if (result == MPI_SUCCESS) {
        store_facade_request(request, native);
    }
    return result;
}

int unimpi_wrap_irecv(void *buf, int count, MPI_Datatype datatype,
                      int source, int tag, MPI_Comm comm,
                      MPI_Request *request) {
    ih_request_t native = 0;
    int result;

    if (!request) {
        return request_arg_error();
    }
    result = real_irecv(
        buf, count, ih_datatype(datatype), source, tag, ih_comm(comm),
        &native);
    if (result == MPI_SUCCESS) {
        store_facade_request(request, native);
    }
    return result;
}
