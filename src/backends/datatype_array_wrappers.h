/* src/backends/datatype_array_wrappers.h
 * Wrapper functions for MPI functions that take arrays of MPI_Datatype.
 *
 * Open MPI uses pointer-based datatypes that fit UnimPI's 8-byte
 * `intptr_t MPI_Datatype`. MPICH, Intel MPI, and MS-MPI use 32-bit integer
 * datatype handles, so an array of UnimPI `MPI_Datatype` (8-byte stride) does
 * not match the native 4-byte stride the backend expects.
 *
 * The collectives below pass the *count* of datatype entries implicitly
 * (always `MPI_Comm_size` entries), so a wrapper must query the communicator
 * size in order to shrink the arrays.
 *
 * Following the same zero-allocation policy as the request-array wrappers,
 * the datatype arrays (read-only inputs) are compressed in place, the native
 * call runs, and the arrays are restored in place by sign-extension before
 * returning. This adds two linear passes over the arrays and one `Comm_size`
 * query — no heap allocation.
 */

#ifndef UNIMPI_DATATYPE_ARRAY_WRAPPERS_H
#define UNIMPI_DATATYPE_ARRAY_WRAPPERS_H

#include "unimpi_vtable.h"

/*
 * Register the native backend function pointers. Each setter returns nonzero
 * iff the pointer is usable; a backend should only install the matching
 * wrapper when it returns nonzero. A NULL native pointer is left untouched.
 *
 * `comm_size` is a back-reference so a wrapper can reproduce the entry count
 * when shrinking a datatype array.
 */
int unimpi_dt_wrapper_set_comm_size(int (*fn)(MPI_Comm, int *));
int unimpi_dt_wrapper_set_alltoallw(int (*fn)(const void *, const int *,
    const int *, const int *, void *, const int *, const int *, const int *,
    MPI_Comm));
int unimpi_dt_wrapper_set_ialltoallw(int (*fn)(const void *, const int *,
    const int *, const int *, void *, const int *, const int *, const int *,
    MPI_Comm, MPI_Request *));

/* Wrapper entry points — assign these to the vtable fields. */
int unimpi_wrap_alltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm);
int unimpi_wrap_ialltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm, MPI_Request *request);

#endif /* UNIMPI_DATATYPE_ARRAY_WRAPPERS_H */
