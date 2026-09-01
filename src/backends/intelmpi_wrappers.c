/* src/backends/intelmpi_wrappers.c
 * INTELMPI-specific wrapper implementations for array operations.
 */

#include "intelmpi_wrappers.h"
#include <stdint.h>
#include <string.h>

/* ---- Real backend function pointers ---- */
int (*intelmpi_waitall)(int, int*, MPI_Status*);
int (*intelmpi_testany)(int, int*, int*, int*, MPI_Status*);
int (*intelmpi_testsome)(int, int*, int*, int*, MPI_Status*);
int (*intelmpi_testall)(int, int*, int*, MPI_Status*);
int (*intelmpi_waitany)(int, int*, int*, MPI_Status*);
int (*intelmpi_waitsome)(int, int*, int*, int*, MPI_Status*);
int (*intelmpi_startall)(int, int*);
int (*intelmpi_alltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm);
int (*intelmpi_ialltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm,
    MPI_Request *);
int (*intelmpi_type_create_struct)(int, const int *, const MPI_Aint *,
    const int *, MPI_Datatype *);
int (*intelmpi_type_struct)(int, const int *, const MPI_Aint *,
    const int *, MPI_Datatype *);
int (*intelmpi_type_get_contents)(MPI_Datatype, int, int, int,
    int *, MPI_Aint *, MPI_Datatype *);
int (*intelmpi_comm_spawn_multiple)(int, char *[], char **[],
    const int[], const int[], int, MPI_Comm, MPI_Comm *, int[]);

/* ---- Request array helpers ---- */

/* Compress 8-byte MPI_Request array to 4-byte int array in-place.
 * Low-32-bit value truncation; correct on supported (little-endian) targets,
 * where the i-th 4-byte slot aliases reqs[i]'s low half and element 0 is
 * already laid out as its low 32 bits. Forward scan: read lags write. */
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

int intelmpi_wrap_waitall(int count, MPI_Request *array_of_requests,
                        MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = intelmpi_waitall(count, (int*)array_of_requests, array_of_statuses);
    reqs_expand_inplace(array_of_requests, count);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, count);
    }
    return ret;
}

int intelmpi_wrap_testany(int count, MPI_Request *array_of_requests,
                        int *index, int *flag, MPI_Status *status) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = intelmpi_testany(count, (int*)array_of_requests, index, flag, status);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int intelmpi_wrap_testsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, incount);
    int ret = intelmpi_testsome(incount, (int*)array_of_requests, outcount,
                           array_of_indices, array_of_statuses);
    reqs_expand_inplace(array_of_requests, incount);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, *outcount);
    }
    return ret;
}

int intelmpi_wrap_testall(int count, MPI_Request *array_of_requests,
                        int *flag, MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = intelmpi_testall(count, (int*)array_of_requests, flag, array_of_statuses);
    reqs_expand_inplace(array_of_requests, count);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, count);
    }
    return ret;
}

int intelmpi_wrap_waitany(int count, MPI_Request *array_of_requests,
                        int *index, MPI_Status *status) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = intelmpi_waitany(count, (int*)array_of_requests, index, status);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

int intelmpi_wrap_waitsome(int incount, MPI_Request *array_of_requests,
                         int *outcount, int *array_of_indices,
                         MPI_Status *array_of_statuses) {
    reqs_compress_inplace(array_of_requests, incount);
    int ret = intelmpi_waitsome(incount, (int*)array_of_requests, outcount,
                           array_of_indices, array_of_statuses);
    reqs_expand_inplace(array_of_requests, incount);
    if (statuses_writable(array_of_statuses)) {
        statuses_expand_inplace(array_of_statuses, *outcount);
    }
    return ret;
}

int intelmpi_wrap_startall(int count, MPI_Request *array_of_requests) {
    reqs_compress_inplace(array_of_requests, count);
    int ret = intelmpi_startall(count, (int*)array_of_requests);
    reqs_expand_inplace(array_of_requests, count);
    return ret;
}

/* ---- Datatype array helpers ---- */

/* Compress MPI_Datatype array to 4-byte int array in-place (see
 * reqs_compress_inplace: same little-endian layout assumption). */
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

int intelmpi_wrap_alltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm) {
    int n = 0;
    unimpi.comm_size(comm, &n);
    dtypes_compress_inplace(sendtypes, n);
    dtypes_compress_inplace(recvtypes, n);
    int ret = intelmpi_alltoallw(sendbuf, sendcounts, sdispls,
        (const int *)sendtypes, recvbuf, recvcounts, rdispls,
        (const int *)recvtypes, comm);
    dtypes_restore_inplace((MPI_Datatype *)sendtypes, n);
    dtypes_restore_inplace((MPI_Datatype *)recvtypes, n);
    return ret;
}

int intelmpi_wrap_ialltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm, MPI_Request *request) {
    int n = 0;
    unimpi.comm_size(comm, &n);
    dtypes_compress_inplace(sendtypes, n);
    dtypes_compress_inplace(recvtypes, n);
    int ret = intelmpi_ialltoallw(sendbuf, sendcounts, sdispls,
        (const int *)sendtypes, recvbuf, recvcounts, rdispls,
        (const int *)recvtypes, comm, request);
    dtypes_restore_inplace((MPI_Datatype *)sendtypes, n);
    dtypes_restore_inplace((MPI_Datatype *)recvtypes, n);
    return ret;
}

int intelmpi_wrap_type_create_struct(int count,
    const int *array_of_blocklengths, const MPI_Aint *array_of_displacements,
    const MPI_Datatype *array_of_types, MPI_Datatype *newtype) {
    dtypes_compress_inplace(array_of_types, count);
    int ret = intelmpi_type_create_struct(count, array_of_blocklengths,
        array_of_displacements, (const int *)array_of_types, newtype);
    dtypes_restore_inplace((MPI_Datatype *)array_of_types, count);
    return ret;
}

int intelmpi_wrap_type_struct(int count,
    const int *array_of_blocklengths, const MPI_Aint *array_of_displacements,
    const MPI_Datatype *array_of_types, MPI_Datatype *newtype) {
    dtypes_compress_inplace(array_of_types, count);
    int ret = intelmpi_type_struct(count, array_of_blocklengths,
        array_of_displacements, (const int *)array_of_types, newtype);
    dtypes_restore_inplace((MPI_Datatype *)array_of_types, count);
    return ret;
}

int intelmpi_wrap_type_get_contents(MPI_Datatype datatype, int max_integers,
    int max_addresses, int max_datatypes, int *array_of_integers,
    MPI_Aint *array_of_addresses, MPI_Datatype *array_of_datatypes) {
    int ret = intelmpi_type_get_contents(datatype, max_integers, max_addresses,
        max_datatypes, array_of_integers, array_of_addresses,
        array_of_datatypes);
    /* Output array_of_datatypes: backend wrote 4-byte int handles over the
     * 8-byte slots; sign-extend them back into intptr_t MPI_Datatype values. */
    dtypes_restore_inplace(array_of_datatypes, max_datatypes);
    return ret;
}

int intelmpi_wrap_comm_spawn_multiple(int count, char *array_of_commands[],
    char **array_of_argv[], const int array_of_maxprocs[],
    const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm,
    int array_of_errcodes[]) {
    /* array_of_info is an input array of MPI_Info handles (8-byte in UnimPI,
     * 4-byte int on INTELMPI); compress before the call, restore after. */
    dtypes_compress_inplace(array_of_info, count);
    int ret = intelmpi_comm_spawn_multiple(count, array_of_commands,
        array_of_argv, array_of_maxprocs, (const int *)array_of_info, root,
        comm, intercomm, array_of_errcodes);
    dtypes_restore_inplace((MPI_Datatype *)array_of_info, count);
    return ret;
}
