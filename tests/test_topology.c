/* tests/test_topology.c - Test MPI topology functions */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { printf("  FAIL: %s\n", msg); return 1; } while(0)

int test_cartesian_topology(void) {
    int rank, size;
    MPI_Comm cart_comm;
    int dims[2] = {0, 0};
    int periods[2] = {1, 0};
    int coords[2];
    int rank_source, rank_dest;
    int topo_type;
    int ndims;

    TEST("Cartesian topology creation");

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 4) {
        printf("  SKIP: Need at least 4 processes\n");
        return 0;
    }

    /* Test MPI_Dims_create */
    int ret = MPI_Dims_create(size, 2, dims);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Dims_create failed");
    }
    if (dims[0] * dims[1] != size) {
        FAIL("MPI_Dims_create gave wrong dimensions");
    }

    /* Test MPI_Cart_create */
    ret = MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_create failed");
    }

    /* Test MPI_Topo_test */
    ret = MPI_Topo_test(cart_comm, &topo_type);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Topo_test failed");
    }
    if (topo_type != MPI_CART) {
        FAIL("MPI_Topo_test did not return MPI_CART");
    }

    /* Test MPI_Cartdim_get */
    ret = MPI_Cartdim_get(cart_comm, &ndims);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cartdim_get failed");
    }
    if (ndims != 2) {
        FAIL("MPI_Cartdim_get returned wrong dimension count");
    }

    /* Test MPI_Cart_get */
    int get_dims[2], get_periods[2];
    ret = MPI_Cart_get(cart_comm, 2, get_dims, get_periods, coords);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_get failed");
    }

    /* Test MPI_Cart_rank */
    int new_rank;
    ret = MPI_Cart_rank(cart_comm, coords, &new_rank);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_rank failed");
    }

    /* Test MPI_Cart_shift */
    ret = MPI_Cart_shift(cart_comm, 0, 1, &rank_source, &rank_dest);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_shift failed");
    }

    /* Cleanup */
    MPI_Comm_free(&cart_comm);

    PASS();
    return 0;
}

int test_graph_topology(void) {
    int rank, size;
    MPI_Comm graph_comm;
    int topo_type;

    TEST("Graph topology");

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 4) {
        printf("  SKIP: Need at least 4 processes\n");
        return 0;
    }

    /* Simple ring topology: each node connected to next */
    int index[4] = {2, 4, 6, 8};
    int edges[8] = {1, 3, 0, 2, 1, 3, 0, 2};

    int ret = MPI_Graph_create(MPI_COMM_WORLD, 4, index, edges, 0, &graph_comm);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Graph_create failed");
    }

    /* Test MPI_Topo_test */
    ret = MPI_Topo_test(graph_comm, &topo_type);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Topo_test failed");
    }
    if (topo_type != MPI_GRAPH) {
        FAIL("MPI_Topo_test did not return MPI_GRAPH");
    }

    /* Test MPI_Graphdims_get */
    int nnodes, nedges;
    ret = MPI_Graphdims_get(graph_comm, &nnodes, &nedges);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Graphdims_get failed");
    }
    if (nnodes != 4 || nedges != 8) {
        FAIL("MPI_Graphdims_get returned wrong values");
    }

    /* Cleanup */
    MPI_Comm_free(&graph_comm);

    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed\n");
        return 1;
    }

    printf("=== MPI Topology Tests ===\n\n");

    ret = test_cartesian_topology();
    if (ret != 0) goto cleanup;

    ret = test_graph_topology();
    if (ret != 0) goto cleanup;

    printf("\n=== All tests passed ===\n");

cleanup:
    MPI_Finalize();
    return ret;
}
