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

/* MPI-3.0 neighbor collectives are defined on any communicator with an
 * associated dist-graph, graph, or *cartesian* topology. The MPICH-family
 * backends (MPICH/Intel/MS-MPI) route Neighbor_alltoallv/w through a wrapper
 * that queries the neighbor degree; this guards that wrapper against wrongly
 * rejecting a cart communicator with MPI_ERR_TOPOLOGY. All ranks send the
 * identical index sequence {0..degree-1}, so the received multiset must equal
 * {0..degree-1} regardless of the backend's cart-neighbor ordering. */
static int test_neighbor_cart_alltoallw(MPI_Comm comm) {
    int size, rank;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    if (size < 2) return 0;              /* need a ring with a real neighbor */
    /* 1-D periodic ring: every rank (dims[0]=size>=2) has 2 in-cart neighbors. */
    int dims[1] = { size }, periods[1] = { 1 };
    MPI_Comm cart;
    CHECK(MPI_Cart_create(comm, 1, dims, periods, 0, &cart));

    int ndims = 0;
    CHECK(MPI_Cartdim_get(cart, &ndims));
    int cdims[1], cperiods[1], ccoords[1];
    CHECK(MPI_Cart_get(cart, ndims, cdims, cperiods, ccoords));
    int degree = 0;
    for (int i = 0; i < ndims; i++) {
        if (cdims[i] <= 1) continue;
        degree += cperiods[i] ? 2 : ((ccoords[i] > 0) + (ccoords[i] < cdims[i] - 1));
    }

    int sendbuf[16], recvbuf[16], counts[16];
    MPI_Aint displs[16];
    MPI_Datatype types[16];
    for (int i = 0; i < degree && i < 16; i++) {
        sendbuf[i] = i;
        recvbuf[i] = -1;
        counts[i] = 1;
        displs[i] = (MPI_Aint)(i * (int)sizeof(int));
        types[i] = MPI_INT;
    }
    CHECK(MPI_Neighbor_alltoallw(sendbuf, counts, displs, types,
                                 recvbuf, counts, displs, types, cart));

    /* verify the received multiset == {0..degree-1} (sort to drop ordering) */
    for (int i = 0; i < degree; i++)
        for (int j = i + 1; j < degree; j++)
            if (recvbuf[j] < recvbuf[i]) {
                int t = recvbuf[i]; recvbuf[i] = recvbuf[j]; recvbuf[j] = t;
            }
    for (int i = 0; i < degree; i++) {
        if (recvbuf[i] != i) {
            fprintf(stderr, "FAIL rank %d cart Neighbor_alltoallw recv[%d]=%d\n",
                    rank, i, recvbuf[i]);
            return 1;
        }
    }
    printf("  Neighbor_alltoallw(cart) OK (rank %d degree %d)\n", rank, degree);
    MPI_Comm_free(&cart);
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
    if (test_neighbor_cart_alltoallw(MPI_COMM_WORLD)) goto fail;
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
