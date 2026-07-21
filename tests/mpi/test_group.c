/* tests/mpi/test_group.c - Group operation tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <assert.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

void test_group_basic(void) {
    int rank, size;
    MPI_Group world_group;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    /* Get world group */
    TEST_CHECK_SUCCESS(MPI_Comm_group(MPI_COMM_WORLD, &world_group));

    int group_size;
    TEST_CHECK_SUCCESS(MPI_Group_size(world_group, &group_size));
    assert(group_size == size);

    int group_rank;
    TEST_CHECK_SUCCESS(MPI_Group_rank(world_group, &group_rank));
    assert(group_rank == rank);

    TEST_CHECK_SUCCESS(MPI_Group_free(&world_group));

    if (rank == 0) printf("  Group_basic test passed\n");
}

void test_group_incl_excl(void) {
    int rank, size;
    MPI_Group world_group, incl_group, excl_group;
    int ranks[2];

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 4) {
        printf("  Skipping group incl/excl test (need 4+ processes)\n");
        return;
    }

    TEST_CHECK_SUCCESS(MPI_Comm_group(MPI_COMM_WORLD, &world_group));

    /* Include first 2 ranks */
    ranks[0] = 0;
    ranks[1] = 1;
    TEST_CHECK_SUCCESS(MPI_Group_incl(world_group, 2, ranks, &incl_group));

    int incl_size;
    TEST_CHECK_SUCCESS(MPI_Group_size(incl_group, &incl_size));
    assert(incl_size == 2);

    /* Exclude rank 0 */
    int excl_rank = 0;
    TEST_CHECK_SUCCESS(MPI_Group_excl(world_group, 1, &excl_rank, &excl_group));

    int excl_size;
    TEST_CHECK_SUCCESS(MPI_Group_size(excl_group, &excl_size));
    assert(excl_size == size - 1);

    TEST_CHECK_SUCCESS(MPI_Group_free(&world_group));
    TEST_CHECK_SUCCESS(MPI_Group_free(&incl_group));
    TEST_CHECK_SUCCESS(MPI_Group_free(&excl_group));

    if (rank == 0) printf("  Group_incl_excl test passed\n");
}

void test_group_compare(void) {
    int rank;
    MPI_Group world_group, dup_group;
    int result;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));

    TEST_CHECK_SUCCESS(MPI_Comm_group(MPI_COMM_WORLD, &world_group));
    TEST_CHECK_SUCCESS(MPI_Comm_group(MPI_COMM_WORLD, &dup_group));

    TEST_CHECK_SUCCESS(MPI_Group_compare(world_group, dup_group, &result));
    assert(result == MPI_IDENT || result == MPI_CONGRUENT);

    TEST_CHECK_SUCCESS(MPI_Group_free(&world_group));
    TEST_CHECK_SUCCESS(MPI_Group_free(&dup_group));

    if (rank == 0) printf("  Group_compare test passed\n");
}

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    const char *backend_name;

    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: error code %d\n", ret);
        return 1;
    }

    printf("Running group tests...\n");
    backend_name = unimpi_get_backend_name();
    assert(backend_name != NULL);
    printf("Using backend: %s\n", backend_name);

    test_group_basic();
    test_group_incl_excl();
    test_group_compare();

    printf("All group tests passed!\n");

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
