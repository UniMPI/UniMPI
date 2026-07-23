#include "datatype_array_wrappers.h"
#include "unimpi.h"

#include <stdint.h>
#include <stdlib.h>

static unimpi_native_comm_query_fn real_comm_size;
static unimpi_native_comm_query_fn real_comm_test_inter;
static unimpi_native_comm_query_fn real_comm_remote_size;
static unimpi_native_alltoallw_fn real_alltoallw;
static unimpi_native_ialltoallw_fn real_ialltoallw;

void unimpi_datatype_array_adapter_init(
    unimpi_native_comm_query_fn comm_size,
    unimpi_native_comm_query_fn comm_test_inter,
    unimpi_native_comm_query_fn comm_remote_size,
    unimpi_native_alltoallw_fn alltoallw,
    unimpi_native_ialltoallw_fn ialltoallw) {
    real_comm_size = comm_size;
    real_comm_test_inter = comm_test_inter;
    real_comm_remote_size = comm_remote_size;
    real_alltoallw = alltoallw;
    real_ialltoallw = ialltoallw;
}

int unimpi_datatype_array_has_alltoallw(void) {
    return real_alltoallw && real_comm_size && real_comm_test_inter &&
           real_comm_remote_size;
}

int unimpi_datatype_array_has_ialltoallw(void) {
    return real_ialltoallw && real_comm_size && real_comm_test_inter &&
           real_comm_remote_size;
}

static int no_memory_error(void) {
    return MPI_ERR_NO_MEM != MPI_SUCCESS ? MPI_ERR_NO_MEM : 39;
}

static int peer_count(MPI_Comm comm, int *count) {
    int is_inter = 0;
    int result;

    result = real_comm_test_inter((int)(intptr_t)comm, &is_inter);
    if (result != MPI_SUCCESS) {
        return result;
    }
    return (is_inter ? real_comm_remote_size : real_comm_size)(
        (int)(intptr_t)comm, count);
}

static int *native_types_create(const MPI_Datatype *types, int count) {
    int *native_types;
    int i;

    if (count <= 0) {
        return NULL;
    }
    native_types = (int *)calloc((size_t)count, sizeof(*native_types));
    if (!native_types) {
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        native_types[i] = (int)(intptr_t)types[i];
    }
    return native_types;
}

static int prepare_types(
    MPI_Comm comm, const MPI_Datatype *sendtypes,
    const MPI_Datatype *recvtypes, int **native_sendtypes,
    int **native_recvtypes) {
    int count = 0;
    int result;

    if (!sendtypes || !recvtypes) {
        return MPI_ERR_TYPE;
    }
    result = peer_count(comm, &count);
    if (result != MPI_SUCCESS) {
        return result;
    }
    *native_sendtypes = native_types_create(sendtypes, count);
    *native_recvtypes = native_types_create(recvtypes, count);
    if (count > 0 && (!*native_sendtypes || !*native_recvtypes)) {
        free(*native_sendtypes);
        free(*native_recvtypes);
        *native_sendtypes = NULL;
        *native_recvtypes = NULL;
        return no_memory_error();
    }
    return MPI_SUCCESS;
}

int unimpi_wrap_alltoallw(
    const void *sendbuf, const int *sendcounts, const int *sdispls,
    const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts,
    const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm) {
    int *native_sendtypes = NULL;
    int *native_recvtypes = NULL;
    int result;

    result = prepare_types(
        comm, sendtypes, recvtypes, &native_sendtypes, &native_recvtypes);
    if (result == MPI_SUCCESS) {
        result = real_alltoallw(
            sendbuf, sendcounts, sdispls, native_sendtypes, recvbuf,
            recvcounts, rdispls, native_recvtypes, (int)(intptr_t)comm);
    }
    free(native_recvtypes);
    free(native_sendtypes);
    return result;
}

int unimpi_wrap_ialltoallw(
    const void *sendbuf, const int *sendcounts, const int *sdispls,
    const MPI_Datatype *sendtypes, void *recvbuf, const int *recvcounts,
    const int *rdispls, const MPI_Datatype *recvtypes, MPI_Comm comm,
    MPI_Request *request) {
    int *native_sendtypes = NULL;
    int *native_recvtypes = NULL;
    int native_request = 0;
    int result;

    if (!request) {
        return MPI_ERR_REQUEST;
    }
    result = prepare_types(
        comm, sendtypes, recvtypes, &native_sendtypes, &native_recvtypes);
    if (result == MPI_SUCCESS) {
        result = real_ialltoallw(
            sendbuf, sendcounts, sdispls, native_sendtypes, recvbuf,
            recvcounts, rdispls, native_recvtypes, (int)(intptr_t)comm,
            &native_request);
        if (result == MPI_SUCCESS) {
            *request = (MPI_Request)(intptr_t)native_request;
        }
    }
    free(native_recvtypes);
    free(native_sendtypes);
    return result;
}
