/* src/backends/msmpi_wrappers.c
 * MSMPI-specific wrapper implementations for array operations.
 */

#include "msmpi_wrappers.h"
#include <stdint.h>
#include <string.h>

/* ---- Real backend function pointers ---- */
int (*msmpi_waitall)(int, int*, MPI_Status*);
int (*msmpi_testany)(int, int*, int*, int*, MPI_Status*);
int (*msmpi_testsome)(int, int*, int*, int*, MPI_Status*);
int (*msmpi_testall)(int, int*, int*, MPI_Status*);
int (*msmpi_waitany)(int, int*, int*, MPI_Status*);
int (*msmpi_waitsome)(int, int*, int*, int*, MPI_Status*);
int (*msmpi_startall)(int, int*);
int (*msmpi_alltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm);
int (*msmpi_ialltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm,
    MPI_Request *);

/* ---- Setters ---- */

/* ---- Request array helpers ---- */

static inline void reqs_compress_inplace(MPI_Request *reqs, int n) {
    int32_t *p = (int32_t*)reqs;
    for (int i = 1; i < n; i++) {
        p[i] = (int32_t)(reqs[i] & 0xFFFFFFFF);
    }
}

static inline void reqs_expand_inplace(MPI_Request *reqs, int n) {
    for (int i = n - 1; i >= 0; i--) {
        int32_t val = ((int32_t*)reqs)[i];
        reqs[i] = (MPI_Request)(intptr_t)val;
    }
}

static inline void statuses_expand_inplace(MPI_Status *statuses, int n) {
    for (int i = n - 1; i >= 0; i--) {
        memcpy(&((char *)statuses)[i * (int)sizeof(union MPI_Status)],
               &((char *)statuses)[i * (int)sizeof(struct unimpi_status_legacy)],
               sizeof(struct unimpi_status_legacy));
    }
}

/* True when the status array is a real array to materialize, rather than
 * the MPI_STATUSES_IGNORE sentinel (which the backend never writes to). */
static inline int statuses_writable(MPI_Status *statuses) {
    return statuses != NULL && statuses != UNIMPI_STATUSES_IGNORE;
}

/* ---- Request array wrappers ---- */

int msmpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                        MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = msmpi_waitall(count, (int*)array_of_requests, array_of_statuses);
    reqs_expand_inplace(array_of_requests, count);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, count);
    }
    return ret;
}

int msmpi_wrap_testany(int count, MPI_Request *array_of_requests,
                        int *index, int *flag, MPI_Status *status) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = msmpi_testany(count, (int*)array_of_requests, index, flag, status);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int msmpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, incount);
    int ret = msmpi_testsome(incount, (int*)array_of_requests, outcount,
                           array_of_indices, array_of_statuses);
    reqs_expand_inplace(array_of_requests, incount);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, *outcount);
    }
    return ret;
}

int msmpi_wrap_testall(int count, MPI_Request *array_of_requests,
                        int *flag, MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = msmpi_testall(count, (int*)array_of_requests, flag, array_of_statuses);
    reqs_expand_inplace(array_of_requests, count);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, count);
    }
    return ret;
}

int msmpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                        int *index, MPI_Status *status) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = msmpi_waitany(count, (int*)array_of_requests, index, status);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int msmpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, incount);
    int ret = msmpi_waitsome(incount, (int*)array_of_requests, outcount,
                           array_of_indices, array_of_statuses);
    reqs_expand_inplace(array_of_requests, incount);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, *outcount);
    }
    return ret;
}

int msmpi_wrap_startall(int count, MPI_Request *array_of_requests) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = msmpi_startall(count, (int*)array_of_requests);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

/* ---- Datatype array helpers ---- */

static inline void dtypes_compress_inplace(const MPI_Datatype *types, int n) {
    int32_t *p = (int32_t *)types;
    for (int i = 1; i < n; i++) {
        p[i] = (int32_t)(types[i] & 0xFFFFFFFF);
    }
}

static inline void dtypes_restore_inplace(MPI_Datatype *types, int n) {
    for (int i = n - 1; i >= 0; i--) {
        int32_t val = ((int32_t *)types)[i];
        types[i] = (MPI_Datatype)(intptr_t)val;
    }
}

/* ---- Datatype array wrappers ---- */

int msmpi_wrap_alltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm) {
    int n = 0;
    unimpi.comm_size(comm, &n);
    dtypes_compress_inplace(sendtypes, n);
    dtypes_compress_inplace(recvtypes, n);
    int ret = msmpi_alltoallw(sendbuf, sendcounts, sdispls,
        (const int *)sendtypes, recvbuf, recvcounts, rdispls,
        (const int *)recvtypes, comm);
    dtypes_restore_inplace((MPI_Datatype *)sendtypes, n);
    dtypes_restore_inplace((MPI_Datatype *)recvtypes, n);
    return ret;
}

int msmpi_wrap_ialltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm, MPI_Request *request) {
    int n = 0;
    unimpi.comm_size(comm, &n);
    dtypes_compress_inplace(sendtypes, n);
    dtypes_compress_inplace(recvtypes, n);
    int ret = msmpi_ialltoallw(sendbuf, sendcounts, sdispls,
        (const int *)sendtypes, recvbuf, recvcounts, rdispls,
        (const int *)recvtypes, comm, request);
    dtypes_restore_inplace((MPI_Datatype *)sendtypes, n);
    dtypes_restore_inplace((MPI_Datatype *)recvtypes, n);
    return ret;
}
