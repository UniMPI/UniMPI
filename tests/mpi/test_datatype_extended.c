/* tests/mpi/test_datatype_extended.c - Extended datatype tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

#define TEST_TAG 300

void test_type_extent(void) {
    int rank;
    MPI_Datatype vector_type;
    MPI_Aint extent;
    int size;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    /* Create vector type: 3 doubles, stride 5 */
    TEST_CHECK_SUCCESS(MPI_Type_vector(3, 1, 5, MPI_DOUBLE, &vector_type));
    TEST_CHECK_SUCCESS(MPI_Type_commit(&vector_type));

    TEST_CHECK_SUCCESS(MPI_Type_extent(vector_type, &extent));
    TEST_CHECK_SUCCESS(MPI_Type_size(vector_type, &size));

    /* Size should be 3 * sizeof(double) = 24 */
    assert(size == 24);
    /* Extent should cover the stride */
    assert(extent >= 24);

    TEST_CHECK_SUCCESS(MPI_Type_free(&vector_type));

    if (rank == 0) printf("  Type_extent test passed\n");
}

void test_pack_unpack(void) {
    int rank;
    int in_buf[4] = {10, 20, 30, 40};
    int out_buf[4] = {0, 0, 0, 0};
    char pack_buf[100];
    int position = 0;
    int pack_size;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    /* Calculate pack size */
    TEST_CHECK_SUCCESS(MPI_Pack_size(4, MPI_INT, MPI_COMM_WORLD, &pack_size));
    assert(pack_size > 0);

    /* Pack data */
    TEST_CHECK_SUCCESS(MPI_Pack(in_buf, 4, MPI_INT, pack_buf, sizeof(pack_buf), &position, MPI_COMM_WORLD));

    /* Unpack data */
    position = 0;
    TEST_CHECK_SUCCESS(MPI_Unpack(pack_buf, sizeof(pack_buf), &position, out_buf, 4, MPI_INT, MPI_COMM_WORLD));

    assert(out_buf[0] == 10);
    assert(out_buf[1] == 20);
    assert(out_buf[2] == 30);
    assert(out_buf[3] == 40);

    if (rank == 0) printf("  Pack/unpack test passed\n");
}

void test_indexed_type(void) {
    int rank;
    MPI_Datatype indexed_type;
    int block_lengths[2] = {2, 3};
    int displacements[2] = {0, 5};
    int data[10] = {1, 2, 0, 0, 0, 3, 4, 5, 0, 0};
    int recv_buf[10] = {0};
    MPI_Status status;
    MPI_Request req;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    TEST_CHECK_SUCCESS(MPI_Type_indexed(2, block_lengths, displacements, MPI_INT, &indexed_type));
    TEST_CHECK_SUCCESS(MPI_Type_commit(&indexed_type));

    if (rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(recv_buf, 1, indexed_type, 0, TEST_TAG, MPI_COMM_WORLD, &req));
        TEST_CHECK_SUCCESS(MPI_Send(data, 1, indexed_type, 0, TEST_TAG, MPI_COMM_WORLD));
        TEST_CHECK_SUCCESS(MPI_Wait(&req, &status));

        assert(recv_buf[0] == 1);
        assert(recv_buf[1] == 2);
        assert(recv_buf[5] == 3);
        assert(recv_buf[6] == 4);
        assert(recv_buf[7] == 5);
    }

    TEST_CHECK_SUCCESS(MPI_Type_free(&indexed_type));
    TEST_CHECK_SUCCESS(MPI_Barrier(MPI_COMM_WORLD));

    if (rank == 0) printf("  Indexed type test passed\n");
}

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    const char *backend_name;

    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: error code %d\n", ret);
        return 1;
    }

    printf("Running extended datatype tests...\n");
    backend_name = unimpi_get_backend_name();
    assert(backend_name != NULL);
    printf("Using backend: %s\n", backend_name);

    test_type_extent();
    test_pack_unpack();
    test_indexed_type();

    printf("All extended datatype tests passed!\n");

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
