/* tests/mpi/test_communicator.c - Communicator operation tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <assert.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

void test_comm_dup(void) {
    int rank;
    MPI_Comm new_comm;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    TEST_CHECK_SUCCESS(MPI_Comm_dup(MPI_COMM_WORLD, &new_comm));

    int new_rank;
    TEST_CHECK_SUCCESS(MPI_Comm_rank(new_comm, &new_rank));
    assert(new_rank == rank);

    TEST_CHECK_SUCCESS(MPI_Comm_free(&new_comm));

    if (rank == 0) printf("  Comm_dup test passed\n");
}

void test_comm_compare(void) {
    int rank, result;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    /* Compare COMM_WORLD with itself */
    TEST_CHECK_SUCCESS(MPI_Comm_compare(MPI_COMM_WORLD, MPI_COMM_WORLD, &result));
    assert(result == MPI_IDENT);

    if (rank == 0) printf("  Comm_compare test passed\n");
}

void test_comm_split(void) {
    int rank, size;
    MPI_Comm new_comm;
    int new_rank, new_size;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 4) {
        printf("  Skipping comm_split test (need 4+ processes)\n");
        return;
    }

    /* Split into even and odd groups */
    int color = rank % 2;
    int key = rank;

    TEST_CHECK_SUCCESS(MPI_Comm_split(MPI_COMM_WORLD, color, key, &new_comm));

    TEST_CHECK_SUCCESS(MPI_Comm_rank(new_comm, &new_rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(new_comm, &new_size));

    /* Verify split correctness */
    if (color == 0) {
        assert(new_size == (size + 1) / 2);
    } else {
        assert(new_size == size / 2);
    }

    TEST_CHECK_SUCCESS(MPI_Comm_free(&new_comm));

    if (rank == 0) printf("  Comm_split test passed\n");
}

void test_cart_create(void) {
    int rank;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    /* Note: Cartesian topology functions not yet implemented in unimpi
     * This test is a placeholder for future implementation
     */

    if (rank == 0) printf("  Cart_create test (placeholder - not implemented)\n");
}

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    const char *backend_name;

    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: error code %d\n", ret);
        return 1;
    }

    printf("Running communicator tests...\n");
    backend_name = unimpi_get_backend_name();
    assert(backend_name != NULL);
    printf("Using backend: %s\n", backend_name);

    test_comm_dup();
    test_comm_compare();
    test_comm_split();
    test_cart_create();

    printf("All communicator tests passed!\n");

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
