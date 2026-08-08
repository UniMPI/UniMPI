/* src/backends/request_handle_wrappers.h
 * Zero-allocation by-value adapters for single-request MPI operations on
 * integer-handle backends (MPICH, Intel MPI, MS-MPI).
 *
 * Facade handles are intptr_t; these backends use 32-bit native int handles.
 * Each wrapper converts by-value handles at the call boundary and uses a
 * local native int to receive an output request, storing a full-width facade
 * value only when the native call defines it. No heap allocation.
 *
 * This module grows incrementally; only the functions it currently wraps are
 * listed here. A backend installs a wrapper only when the native symbol
 * resolved (setter returns nonzero).
 */
#ifndef UNIMPI_REQUEST_HANDLE_WRAPPERS_H
#define UNIMPI_REQUEST_HANDLE_WRAPPERS_H

#include "unimpi_vtable.h"

/* Register native integer-backend entry points. Each setter returns nonzero
 * iff the pointer is usable; a backend installs the matching wrapper only
 * when it returns nonzero. */
int unimpi_ih_set_isend(int (*fn)(const void *, int, int, int, int, int, int *));
int unimpi_ih_set_irecv(int (*fn)(void *, int, int, int, int, int, int *));

/* Wrapper entry points — assign these to the vtable fields. */
int unimpi_wrap_isend(const void *buf, int count, MPI_Datatype datatype,
                      int dest, int tag, MPI_Comm comm, MPI_Request *request);
int unimpi_wrap_irecv(void *buf, int count, MPI_Datatype datatype,
                      int source, int tag, MPI_Comm comm,
                      MPI_Request *request);

#endif /* UNIMPI_REQUEST_HANDLE_WRAPPERS_H */
