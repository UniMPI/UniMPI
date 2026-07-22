/* tests/mpi/test_datatype_extended2.c - MPI-2.2 extended datatype tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

int test_create_resized(void) {
    int rank;
    MPI_Datatype resized;
    MPI_Aint lb, extent;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("Type_create_resized");

    /* Resize MPI_INT to have double the extent */
    int ret = MPI_Type_create_resized(MPI_INT, 0, 2 * sizeof(int), &resized);
    if (ret != MPI_SUCCESS) FAIL("MPI_Type_create_resized failed");

    ret = MPI_Type_get_extent(resized, &lb, &extent);
    if (ret != MPI_SUCCESS) FAIL("MPI_Type_get_extent on resized failed");
    if (extent != 2 * sizeof(int)) {
        fprintf(stderr, "  FAIL: extent=%ld (expected %ld)\n",
                (long)extent, (long)(2 * sizeof(int)));
        return 1;
    }
    if (lb != 0) {
        fprintf(stderr, "  FAIL: lb=%ld (expected 0)\n", (long)lb);
        return 1;
    }

    MPI_Type_free(&resized);
    PASS();
    return 0;
}

int test_get_envelope(void) {
    int rank;
    int num_integers, num_addresses, num_datatypes, combiner;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("Type_get_envelope (contiguous)");

    MPI_Datatype contig;
    MPI_Type_contiguous(5, MPI_INT, &contig);

    int ret = MPI_Type_get_envelope(contig, &num_integers, &num_addresses,
                                    &num_datatypes, &combiner);
    if (ret != MPI_SUCCESS) FAIL("MPI_Type_get_envelope failed");
    if (num_integers <= 0) FAIL("expected positive num_integers");
    if (num_datatypes <= 0) FAIL("expected positive num_datatypes");

    MPI_Type_free(&contig);
    PASS();
    return 0;
}

int test_get_contents(void) {
    int rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("Type_get_contents");

    MPI_Datatype contig;
    MPI_Type_contiguous(5, MPI_INT, &contig);

    int num_integers, num_addresses, num_datatypes, combiner;
    MPI_Type_get_envelope(contig, &num_integers, &num_addresses,
                          &num_datatypes, &combiner);

    int *ints = (int*)calloc(num_integers > 0 ? num_integers : 1, sizeof(int));
    MPI_Aint *addrs = (MPI_Aint*)calloc(num_addresses > 0 ? num_addresses : 1, sizeof(MPI_Aint));
    MPI_Datatype *types = (MPI_Datatype*)calloc(num_datatypes > 0 ? num_datatypes : 1, sizeof(MPI_Datatype));

    int ret = MPI_Type_get_contents(contig, num_integers, num_addresses,
                                    num_datatypes, ints, addrs, types);
    if (ret != MPI_SUCCESS) FAIL("MPI_Type_get_contents failed");
    if (num_integers > 0 && ints[0] != 5) {
        fprintf(stderr, "  FAIL: count=%d (expected 5)\n", ints[0]);
        return 1;
    }

    free(ints);
    free(addrs);
    free(types);
    MPI_Type_free(&contig);
    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret, rank;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed\n");
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI Extended Datatype Tests ===\n\n");

    ret = test_create_resized();
    if (ret != 0) goto cleanup;

    ret = test_get_envelope();
    if (ret != 0) goto cleanup;

    ret = test_get_contents();
    if (ret != 0) goto cleanup;

    printf("\n=== All tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
