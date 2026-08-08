/* src/backends/datatype_array_wrappers.c
 * Zero-allocation in-place wrappers for MPI collectives that take arrays of
 * MPI_Datatype.
 *
 * MPICH, Intel MPI, and MS-MPI use `typedef int MPI_Datatype` (4 bytes),
 * while UnimPI uses `typedef intptr_t MPI_Datatype` (8 bytes) to also cover
 * Open MPI's pointer-based datatypes. `MPI_Alltoallw`/`MPI_Ialltoallw` pass
 * datatype arrays whose element count is implicit (the communicator size), so
 * the 8-byte stride does not match the native 4-byte stride and the backend
 * would misinterpret every entry after the first.
 *
 * The datatype arrays are read-only inputs. Following the request-array
 * wrappers, we compress them in place (8->4 bytes), run the native call, then
 * restore them in place with sign-extension (4->8 bytes). Datatype handles on
 * these backends are non-negative 32-bit integers, so sign-extension is an
 * exact inverse. This costs two linear passes and one `Comm_size` query and
 * performs no heap allocation.
 */

#include "datatype_array_wrappers.h"
#include <stddef.h>
#include <stdint.h>

/* ---- Real backend function pointers (set during backend init) ---- */
static int (*real_comm_size)(MPI_Comm, int *);
static int (*real_alltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm);
static int (*real_ialltoallw)(const void *, const int *, const int *,
    const int *, void *, const int *, const int *, const int *, MPI_Comm,
    MPI_Request *);

/* ---- Setters ---- */
int unimpi_dt_wrapper_set_comm_size(int (*fn)(MPI_Comm, int *)) {
    if (fn) real_comm_size = fn;
    return fn != NULL;
}
int unimpi_dt_wrapper_set_alltoallw(int (*fn)(const void *, const int *,
    const int *, const int *, void *, const int *, const int *, const int *,
    MPI_Comm)) {
    if (fn) real_alltoallw = fn;
    return fn != NULL;
}
int unimpi_dt_wrapper_set_ialltoallw(int (*fn)(const void *, const int *,
    const int *, const int *, void *, const int *, const int *, const int *,
    MPI_Comm, MPI_Request *)) {
    if (fn) real_ialltoallw = fn;
    return fn != NULL;
}

/* ---- In-place conversion helpers (mirror the request-array wrappers) ---- */

/* Shrink an 8-byte `MPI_Datatype` array to a 4-byte native handle array in
 * place. Forward scan: for each i read the low 4 bytes of element i (at
 * int position 2*i) and write to int position i. Source > destination, so no
 * read-after-write hazard. */
static inline void dtypes_compress_inplace(const MPI_Datatype *types, int n) {
    int32_t *p = (int32_t *)types;
    for (int i = 1; i < n; i++) {
        p[i] = p[i * 2];
    }
}

/* Restore the 8-byte representation by sign-extending each native handle.
 * Backward scan: writing 8 bytes at offset i never overlaps reading 4 bytes
 * at offset i. */
static inline void dtypes_restore_inplace(MPI_Datatype *types, int n) {
    for (int i = n - 1; i >= 0; i--) {
        int32_t val = ((int32_t *)types)[i];
        types[i] = (MPI_Datatype)(intptr_t)val;
    }
}

/* ---- Wrapper implementations ---- */

int unimpi_wrap_alltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm) {
    int n = 0;
    if (real_comm_size) {
        real_comm_size(comm, &n);
    }
    if (n <= 0 || !sendtypes || !recvtypes) {
        return real_alltoallw(sendbuf, sendcounts, sdispls,
            (const int *)sendtypes, recvbuf, recvcounts, rdispls,
            (const int *)recvtypes, comm);
    }
    dtypes_compress_inplace(sendtypes, n);
    dtypes_compress_inplace(recvtypes, n);
    int ret = real_alltoallw(sendbuf, sendcounts, sdispls,
        (const int *)sendtypes, recvbuf, recvcounts, rdispls,
        (const int *)recvtypes, comm);
    dtypes_restore_inplace((MPI_Datatype *)sendtypes, n);
    dtypes_restore_inplace((MPI_Datatype *)recvtypes, n);
    return ret;
}

int unimpi_wrap_ialltoallw(const void *sendbuf, const int *sendcounts,
    const int *sdispls, const MPI_Datatype *sendtypes, void *recvbuf,
    const int *recvcounts, const int *rdispls, const MPI_Datatype *recvtypes,
    MPI_Comm comm, MPI_Request *request) {
    int n = 0;
    if (real_comm_size) {
        real_comm_size(comm, &n);
    }
    if (n <= 0 || !sendtypes || !recvtypes) {
        return real_ialltoallw(sendbuf, sendcounts, sdispls,
            (const int *)sendtypes, recvbuf, recvcounts, rdispls,
            (const int *)recvtypes, comm, request);
    }
    dtypes_compress_inplace(sendtypes, n);
    dtypes_compress_inplace(recvtypes, n);
    int ret = real_ialltoallw(sendbuf, sendcounts, sdispls,
        (const int *)sendtypes, recvbuf, recvcounts, rdispls,
        (const int *)recvtypes, comm, request);
    dtypes_restore_inplace((MPI_Datatype *)sendtypes, n);
    dtypes_restore_inplace((MPI_Datatype *)recvtypes, n);
    return ret;
}
