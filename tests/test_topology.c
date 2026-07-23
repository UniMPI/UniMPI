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
    int coords_roundtrip[2];
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
    if (new_rank != rank) {
        FAIL("MPI_Cart_rank did not round-trip the calling rank");
    }

    /* Test MPI_Cart_coords */
    ret = MPI_Cart_coords(cart_comm, new_rank, 2, coords_roundtrip);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_coords failed");
    }
    if (coords_roundtrip[0] != coords[0] ||
        coords_roundtrip[1] != coords[1]) {
        FAIL("MPI_Cart_coords did not round-trip coordinates");
    }

    /* Test MPI_Cart_shift */
    ret = MPI_Cart_shift(cart_comm, 0, 1, &rank_source, &rank_dest);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_shift failed");
    }

    /* Test MPI_Cart_map */
    int mapped_rank = MPI_UNDEFINED;
    ret = MPI_Cart_map(MPI_COMM_WORLD, 2, dims, periods, &mapped_rank);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_map failed");
    }
    if (mapped_rank < 0 || mapped_rank >= size) {
        FAIL("MPI_Cart_map returned an invalid rank");
    }

    /* Test MPI_Cart_sub by retaining the first dimension. */
    int remain_dims[2] = {1, 0};
    MPI_Comm sub_comm;
    int sub_size = 0;
    ret = MPI_Cart_sub(cart_comm, remain_dims, &sub_comm);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Cart_sub failed");
    }
    ret = MPI_Comm_size(sub_comm, &sub_size);
    if (ret != MPI_SUCCESS || sub_size != dims[0]) {
        FAIL("MPI_Cart_sub returned the wrong subgrid size");
    }
    MPI_Comm_free(&sub_comm);

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

    if (size < 3) {
        printf("  SKIP: Need at least 3 processes\n");
        return 0;
    }

    /* Ring topology: each node is connected to its predecessor and successor. */
    int *index = (int *)malloc((size_t)size * sizeof(*index));
    int *edges = (int *)malloc((size_t)size * 2 * sizeof(*edges));
    int *get_index = (int *)malloc((size_t)size * sizeof(*get_index));
    int *get_edges = (int *)malloc((size_t)size * 2 * sizeof(*get_edges));
    if (!index || !edges || !get_index || !get_edges) {
        free(index);
        free(edges);
        free(get_index);
        free(get_edges);
        FAIL("Failed to allocate graph arrays");
    }
    for (int i = 0; i < size; i++) {
        index[i] = 2 * (i + 1);
        edges[2 * i] = (i + size - 1) % size;
        edges[2 * i + 1] = (i + 1) % size;
    }

    int ret = MPI_Graph_create(
        MPI_COMM_WORLD, size, index, edges, 0, &graph_comm);
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
    if (nnodes != size || nedges != 2 * size) {
        FAIL("MPI_Graphdims_get returned wrong values");
    }

    /* Test MPI_Graph_get */
    ret = MPI_Graph_get(
        graph_comm, size, 2 * size, get_index, get_edges);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Graph_get failed");
    }
    for (int i = 0; i < size; i++) {
        if (get_index[i] != index[i] ||
            get_edges[2 * i] != edges[2 * i] ||
            get_edges[2 * i + 1] != edges[2 * i + 1]) {
            FAIL("MPI_Graph_get returned the wrong topology");
        }
    }

    /* Test neighbor queries for the calling rank. */
    int neighbor_count = 0;
    int neighbors[2] = {MPI_UNDEFINED, MPI_UNDEFINED};
    ret = MPI_Graph_neighbors_count(graph_comm, rank, &neighbor_count);
    if (ret != MPI_SUCCESS || neighbor_count != 2) {
        FAIL("MPI_Graph_neighbors_count failed");
    }
    ret = MPI_Graph_neighbors(graph_comm, rank, 2, neighbors);
    if (ret != MPI_SUCCESS) {
        FAIL("MPI_Graph_neighbors failed");
    }
    if (neighbors[0] != (rank + size - 1) % size ||
        neighbors[1] != (rank + 1) % size) {
        FAIL("MPI_Graph_neighbors returned the wrong ranks");
    }

    /* Test MPI_Graph_map */
    int mapped_rank = MPI_UNDEFINED;
    ret = MPI_Graph_map(
        MPI_COMM_WORLD, size, index, edges, &mapped_rank);
    if (ret != MPI_SUCCESS || mapped_rank < 0 || mapped_rank >= size) {
        FAIL("MPI_Graph_map failed");
    }

    /* Cleanup */
    MPI_Comm_free(&graph_comm);
    free(index);
    free(edges);
    free(get_index);
    free(get_edges);

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
