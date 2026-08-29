/* tests/mpi/test_tierD_dist_graph.c - runtime validation of the MPI-2.2
 * distributed-graph topology bindings (Dist_graph_create/create_adjacent/
 * neighbors_count/neighbors). Builds a ring and checks in/out degrees and
 * neighbor ranks, proving the function-pointer signatures are ABI-correct
 * against a real backend.
 */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

static int test_ring_dist_graph(void) {
    int rank, size;
    MPI_Comm base, dg;
    int sources[1], degrees[1], destinations[1], weights[1];

    TEST("Dist_graph ring");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size < 2) { printf("  SKIP (need >=2 ranks)\n"); return 0; }

    /* Dist_graph_create requires an input communicator created by Comm_dup. */
    if (MPI_Comm_dup(MPI_COMM_WORLD, &base) != MPI_SUCCESS)
        FAIL("MPI_Comm_dup failed");

    /* Ring: p -> (p+1) % size, unweighted (weights == MPI_UNWEIGHTED == NULL). */
    sources[0] = rank;
    degrees[0] = 1;
    destinations[0] = (rank + 1) % size;
    weights[0] = 1;
    /* NB: MPI_UNWEIGHTED is a backend-specific sentinel (not NULL) and is not
     * exposed by UniMPI, so a portable test uses explicit unit weights. */
    if (MPI_Dist_graph_create(base, 1, sources, degrees, destinations,
                              weights, MPI_INFO_NULL, 0, &dg) != MPI_SUCCESS)
        FAIL("MPI_Dist_graph_create failed");

    /* Each ring vertex has exactly one incoming and one outgoing edge. */
    {
        int indegree = -1, outdegree = -1, weighted = -1;
        if (MPI_Dist_graph_neighbors_count(dg, &indegree, &outdegree, &weighted) != MPI_SUCCESS)
            FAIL("MPI_Dist_graph_neighbors_count failed");
        if (indegree != 1 || outdegree != 1) {
            fprintf(stderr, "  FAIL: indegree=%d outdegree=%d want 1,1\n",
                    indegree, outdegree);
            return 1;
        }
        if (weighted != 1) {
            fprintf(stderr, "  FAIL: weighted=%d want 1 (explicit weights)\n", weighted);
            return 1;
        }
    }

    {
        int isrc[1] = {-1}, isrcw[1] = {0}, odest[1] = {-1}, odestw[1] = {0};
        int want_in = (rank - 1 + size) % size;
        int want_out = (rank + 1) % size;
        if (MPI_Dist_graph_neighbors(dg, 1, isrc, isrcw, 1, odest, odestw) != MPI_SUCCESS)
            FAIL("MPI_Dist_graph_neighbors failed");
        if (isrc[0] != want_in) {
            fprintf(stderr, "  FAIL: src=%d want %d\n", isrc[0], want_in);
            return 1;
        }
        if (odest[0] != want_out) {
            fprintf(stderr, "  FAIL: dest=%d want %d\n", odest[0], want_out);
            return 1;
        }
    }

    /* create_adjacent: rebuild the same ring from local in/out lists. */
    {
        int isrc[1] = { (rank - 1 + size) % size };
        int odev[1] = { (rank + 1) % size };
        int w1[1] = { 1 };
        MPI_Comm dg2;
        if (MPI_Dist_graph_create_adjacent(base, 1, isrc, w1, 1, odev, w1,
                                           MPI_INFO_NULL, 0, &dg2) != MPI_SUCCESS)
            FAIL("MPI_Dist_graph_create_adjacent failed");
        {
            int indegree = -1, outdegree = -1, weighted = -1;
            if (MPI_Dist_graph_neighbors_count(dg2, &indegree, &outdegree, &weighted) != MPI_SUCCESS)
                FAIL("MPI_Dist_graph_neighbors_count(adjacent) failed");
            if (indegree != 1 || outdegree != 1 || weighted != 1) {
                fprintf(stderr, "  FAIL: adjacent in=%d out=%d wt=%d want 1,1,1\n",
                        indegree, outdegree, weighted);
                return 1;
            }
        }
        MPI_Comm_free(&dg2);
    }

    MPI_Comm_free(&dg);
    MPI_Comm_free(&base);
    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) { fprintf(stderr, "MPI_Init failed\n"); return 1; }
    printf("=== MPI-2.2 Distributed-Graph Topology Tests ===\n\n");

    if ((ret = test_ring_dist_graph()) != 0) goto cleanup;

    printf("\n=== All distributed-graph tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
