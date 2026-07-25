/* Datatype-array ABI adapters for integer-handle MPI backends. */
#ifndef UNIMPI_DATATYPE_ARRAY_WRAPPERS_H
#define UNIMPI_DATATYPE_ARRAY_WRAPPERS_H

#include "unimpi_vtable.h"

typedef int (UNIMPI_MPI_CALL *unimpi_native_comm_query_fn)(int, int *);
typedef int (UNIMPI_MPI_CALL *unimpi_native_alltoallw_fn)(
    const void *, const int *, const int *, const int *,
    void *, const int *, const int *, const int *, int);

void unimpi_datatype_array_adapter_init(
    unimpi_native_comm_query_fn comm_size,
    unimpi_native_comm_query_fn comm_test_inter,
    unimpi_native_comm_query_fn comm_remote_size,
    unimpi_native_alltoallw_fn alltoallw);
int unimpi_datatype_array_has_alltoallw(void);

int unimpi_wrap_alltoallw(
    const void *sendbuf, const int *sendcounts, const int *sdispls,
    const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts,
    const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm);

#endif
