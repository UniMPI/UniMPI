/* tests/mpi/test_mpi30_core.c
 * MPI-3.0 core feature integration test: neighbor collectives, large-count
 * (_x) datatype query, dynamic windows, Comm_idup.
 *
 * Runs only when UniMPI's target is >= MPI-3.0 (see tests/CMakeLists.txt);
 * uses API that an MPI-2.2 target #if's out of the vtable and std macros.
 */

#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

/* On failure, record the offending expression and bail out of the enclosing
 * function; main() owns MPI_Finalize(). */
#define CHECK(expr) do { \
    if ((expr) != MPI_SUCCESS) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } } while (0)

static int test_neighbor_allgather(MPI_Comm comm) {
    int size, rank;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    /* Dist_graph_create wants an input communicator produced by Comm_dup,
     * and UniMPI does not expose the backend-specific MPI_UNWEIGHTED sentinel,
     * so use an explicit unit-weight array (see test_tierD_dist_graph.c). */
    MPI_Comm base;
    CHECK(MPI_Comm_dup(comm, &base));
    /* dist-graph: each node has one out-neighbor, rank -> (rank+1)%size */
    int src[1] = { rank };
    int dst[1] = { (rank + 1) % size };
    int deg[1] = { 1 };
    int weights[1] = { 1 };
    MPI_Comm dg;
    CHECK(MPI_Dist_graph_create(base, 1, src, deg, dst, weights,
                                MPI_INFO_NULL, 0, &dg));
    MPI_Comm_free(&base);
    int sendbuf = rank, recvbuf = -1;
    CHECK(MPI_Neighbor_allgather(&sendbuf, 1, MPI_INT, &recvbuf, 1, MPI_INT, dg));
    /* with one out-neighbor, we receive from (rank-1+size)%size */
    int expect = (rank - 1 + size) % size;
    if (recvbuf != expect) {
        fprintf(stderr, "FAIL rank %d Neighbor_allgather recv=%d expect=%d\n",
                rank, recvbuf, expect);
        return 1;
    }
    printf("  Neighbor_allgather OK (rank %d recv %d)\n", rank, recvbuf);
    MPI_Comm_free(&dg);
    return 0;
}

static int test_comm_idup(MPI_Comm comm) {
    MPI_Comm newcomm;
    MPI_Request req;
    /* Nonblocking comm duplicate: wait it out and confirm usable size/rank. */
    int newrank = -1;
    CHECK(MPI_Comm_idup(comm, &newcomm, &req));
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    CHECK(MPI_Comm_rank(newcomm, &newrank));
    if (newrank < 0) {
        fprintf(stderr, "FAIL rank %d Comm_idup newcomm rank=%d\n",
                newrank < 0 ? 0 : newrank, newrank);
        return 1;
    }
    printf("  Comm_idup OK\n");
    MPI_Comm_free(&newcomm);
    return 0;
}

static int test_large_count_and_type_x(MPI_Comm comm) {
    MPI_Count sz = 0, lb = 0, ext = 0, true_ext = 0;
    CHECK(MPI_Type_size_x(MPI_INT, &sz));
    CHECK(MPI_Type_get_extent_x(MPI_INT, &lb, &ext));
    CHECK(MPI_Type_get_true_extent_x(MPI_INT, &lb, &true_ext));
    if (sz <= 0 || ext <= 0 || ext != true_ext) {
        fprintf(stderr, "FAIL _x size=%llu ext=%llu true=%llu\n",
                (unsigned long long)sz, (unsigned long long)ext,
                (unsigned long long)true_ext);
        return 1;
    }
    /* Get_elements_x round-trips the element count out of a real status.
     * Each rank does a self-sendrecv with separate send/recv buffers (MPICH
     * asserts when send and recv buffers overlap). */
    int sendv = 1, recvv = -1;
    int rank;
    MPI_Comm_rank(comm, &rank);
    MPI_Status st;
    CHECK(MPI_Sendrecv(&sendv, 1, MPI_INT, rank, 0, &recvv, 1, MPI_INT, rank,
                       0, comm, &st));
    MPI_Count n = 0;
    CHECK(MPI_Get_elements_x(&st, MPI_INT, &n));
    if (n != 1) {
        fprintf(stderr, "FAIL Get_elements_x n=%llu\n", (unsigned long long)n);
        return 1;
    }
    /* Status_set_elements_x then read back. */
    MPI_Status st2 = st;
    CHECK(MPI_Status_set_elements_x(&st2, MPI_INT, (MPI_Count)3));
    CHECK(MPI_Get_elements_x(&st2, MPI_INT, &n));
    if (n != 3) {
        fprintf(stderr, "FAIL Status_set_elements_x readback n=%llu\n",
                (unsigned long long)n);
        return 1;
    }
    printf("  Type_size_x / Get_elements_x / Status_set_elements_x OK\n");
    return 0;
}

static int test_win_dynamic(void) {
    MPI_Errhandler prev;
    MPI_Comm_get_errhandler(MPI_COMM_WORLD, &prev);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Win win;
    int rc = MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, prev);
    if (rc != MPI_SUCCESS) {
        /* Some environments don't support dynamic windows (e.g. this OpenMPI
         * build reports MPI_ERR_WIN even natively); degrade to a skip rather
         * than failing the suite. */
        printf("  win_dynamic create unsupported (rc=%d); skip\n", rc);
        return 0;
    }
    int slot = 7;
    CHECK(MPI_Win_attach(win, &slot, sizeof(int)));
    CHECK(MPI_Win_detach(win, &slot));
    MPI_Win_free(&win);
    printf("  win_dynamic create/attach/detach OK\n");
    return 0;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    printf("=== MPI-3.0 core tests ===\n");
    if (test_neighbor_allgather(MPI_COMM_WORLD)) goto fail;
    if (test_comm_idup(MPI_COMM_WORLD)) goto fail;
    if (test_large_count_and_type_x(MPI_COMM_WORLD)) goto fail;
    if (test_win_dynamic()) goto fail;
    printf("=== All MPI-3.0 core tests passed ===\n");
    MPI_Finalize();
    return 0;
fail:
    MPI_Finalize();
    return 1;
}
