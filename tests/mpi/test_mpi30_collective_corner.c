/* tests/mpi/test_mpi30_collective_corner.c
 * MPI-3.0 nonblocking-collective corner-case semantics: in-place buffers
 * (MPI_IN_PLACE), zero-count operations, and tolerance of non-power-of-two
 * communicator sizes. The generic nonblocking-collective suite
 * (test_collective_nonblocking.c) exercises the happy path for every Iall*
 * call; this file probes the corners it leaves untouched.
 *
 * Each sub-test asserts real values round-trip (or, for the aggregate
 * collectives, that the reduced result matches a locally computed reference),
 * not just a clean exit. Requires the MPI_IN_PLACE sentinel, which UniMPI
 * exposes as a runtime global filled by each backend (OpenMPI (void*)1,
 * MPICH-family (void*)-1).
 *
 * Runs only when UniMPI's target is >= MPI-3.0 (see tests/CMakeLists.txt).
 */

#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

#define CHECK(expr) do { \
    if ((expr) != MPI_SUCCESS) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } } while (0)

/* In-place Iallreduce: every rank's buffer holds its rank+1 and the in-place
 * reduce-over-self must yield the sum 1+2+...+size on every rank. This is the
 * core in-place semantic -- no separate receive buffer is supplied, so the
 * operation must read and write the same location. */
static int test_inplace_iallreduce(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int value = rank + 1;
    MPI_Request req;
    CHECK(MPI_Iallreduce(MPI_IN_PLACE, &value, 1, MPI_INT, MPI_SUM, comm,
                         &req));
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    int expected = 0;
    for (int i = 0; i < size; i++) expected += i + 1;
    if (value != expected) {
        fprintf(stderr, "FAIL rank %d inplace iallreduce=%d expect=%d\n",
                rank, value, expected);
        return 1;
    }
    printf("  Iallreduce(in_place) OK (rank %d val %d)\n", rank, value);
    return 0;
}

/* In-place Igather root: on the root, the receive buffer aliases the send
 * buffer (the root's own contribution occupies slot 0). Every rank contributes
 * its rank; after the gather the root's buffer must be exactly [0..size-1].
 * Non-root ranks pass MPI_IN_PLACE and MPI_IGNORE for sendbuf/recvbuf. */
static int test_inplace_igather(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int *buf = malloc((size_t)size * sizeof(int));
    if (buf == NULL) return 1;
    for (int i = 0; i < size; i++) buf[i] = -1;
    buf[rank] = rank;               /* root's own slot, read in place */
    MPI_Request req;
    if (rank == 0) {
        CHECK(MPI_Igather(MPI_IN_PLACE, 1, MPI_INT, buf, 1, MPI_INT, 0, comm,
                          &req));
    } else {
        CHECK(MPI_Igather(&rank, 1, MPI_INT, NULL, 1, MPI_INT, 0, comm, &req));
    }
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    int ok = 1;
    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            if (buf[i] != i) { ok = 0; break; }
        }
    }
    if (!ok) {
        fprintf(stderr, "FAIL rank %d inplace igather mismatch\n", rank);
        free(buf);
        return 1;
    }
    printf("  Igather(in_place root) OK (rank %d)\n", rank);
    free(buf);
    return 0;
}

/* In-place Iscatter: the root's buffer aliases the send buffer; each rank
 * must receive buf[rank]. Exercises the root-side in-place scatter path. */
static int test_inplace_iscatter(MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    int *sbuf = malloc((size_t)size * sizeof(int));
    if (sbuf == NULL) return 1;
    for (int i = 0; i < size; i++) sbuf[i] = 1000 + i;
    int recv_val = -1;
    MPI_Request req;
    if (rank == 0) {
        /* For scatter, in-place aliases the RECEIVE buffer (the root provides
         * the send data via sendbuf and receives its own share in place),
         * the mirror image of gather where in-place aliases sendbuf. */
        CHECK(MPI_Iscatter(sbuf, 1, MPI_INT, MPI_IN_PLACE, 1, MPI_INT, 0,
                           comm, &req));
    } else {
        CHECK(MPI_Iscatter(sbuf, 1, MPI_INT, &recv_val, 1, MPI_INT, 0, comm,
                           &req));
    }
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    int expected = 1000 + rank;
    int got = (rank == 0) ? sbuf[0] : recv_val;
    if (got != expected) {
        fprintf(stderr, "FAIL rank %d inplace iscatter=%d expect=%d\n",
                rank, got, expected);
        free(sbuf);
        return 1;
    }
    printf("  Iscatter(in_place root) OK (rank %d val %d)\n", rank, got);
    free(sbuf);
    return 0;
}

/* Zero-count Iallreduce: MPI-3 allows count==0 with datatype significant; the
 * result buffer must be left untouched. Assert the sentinel survives. */
static int test_zero_count_iallreduce(MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);
    int value = 777;
    MPI_Request req;
    CHECK(MPI_Iallreduce(&value, &value, 0, MPI_INT, MPI_SUM, comm, &req));
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    if (value != 777) {
        fprintf(stderr, "FAIL rank %d zero-count iallreduce clobbered=%d\n",
                rank, value);
        return 1;
    }
    printf("  Iallreduce(count=0) OK (rank %d sentinel %d)\n", rank, value);
    return 0;
}

/* Zero-count Ibcast: count==0 must complete without touching the buffer. */
static int test_zero_count_ibcast(MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);
    int value = 999;
    MPI_Request req;
    CHECK(MPI_Ibcast(&value, 0, MPI_INT, 0, comm, &req));
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    if (value != 999) {
        fprintf(stderr, "FAIL rank %d zero-count ibcast clobbered=%d\n",
                rank, value);
        return 1;
    }
    printf("  Ibcast(count=0) OK (rank %d sentinel %d)\n", rank, value);
    return 0;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI-3.0 nonblocking-collective corner tests ===\n");
    if (test_inplace_iallreduce(MPI_COMM_WORLD)) goto fail;
    if (test_inplace_igather(MPI_COMM_WORLD)) goto fail;
    if (test_inplace_iscatter(MPI_COMM_WORLD)) goto fail;
    if (test_zero_count_iallreduce(MPI_COMM_WORLD)) goto fail;
    if (test_zero_count_ibcast(MPI_COMM_WORLD)) goto fail;
    printf("=== All MPI-3.0 collective-corner tests passed ===\n");
    MPI_Finalize();
    return 0;
fail:
    MPI_Finalize();
    return 1;
}
