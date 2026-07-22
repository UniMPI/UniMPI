/* tests/mpi/test_intercomm.c - MPI-2.2 intercommunicator tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

int test_intercomm(void) {
    int rank, size, flag, remote_size;
    MPI_Comm intercomm, merged, color_comm, remote_group;
    int ret;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("  SKIP: need >=2 processes\n");
        return 0;
    }

    TEST("Comm_test_inter (intracommunicator)");
    ret = MPI_Comm_test_inter(MPI_COMM_WORLD, &flag);
    if (ret != MPI_SUCCESS || flag) FAIL("intracommunicator should not be inter");
    PASS();

    /* Split into two groups to create an intercommunicator */
    int half = size / 2;
    int color = rank < half ? 0 : 1;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &color_comm);

    /*
     * MPI_Intercomm_create: local_leader is rank within local_comm,
     * remote_leader is rank within peer_comm (MPI_COMM_WORLD).
     * For color=0 group: remote leader = rank 'half' in COMM_WORLD
     * For color=1 group: remote leader = rank 0 in COMM_WORLD
     */
    int remote_leader = (color == 0) ? half : 0;

    TEST("Intercomm_create");
    ret = MPI_Intercomm_create(color_comm, 0, MPI_COMM_WORLD,
                               remote_leader, 99, &intercomm);
    if (ret != MPI_SUCCESS) FAIL("MPI_Intercomm_create failed");
    PASS();

    TEST("Comm_test_inter (intercommunicator)");
    ret = MPI_Comm_test_inter(intercomm, &flag);
    if (ret != MPI_SUCCESS || !flag) FAIL("intercommunicator should be inter");
    PASS();

    TEST("Comm_remote_size");
    ret = MPI_Comm_remote_size(intercomm, &remote_size);
    if (ret != MPI_SUCCESS) FAIL("MPI_Comm_remote_size failed");
    if (remote_size != half) {
        fprintf(stderr, "  FAIL: remote_size=%d (expected %d)\n", remote_size, half);
        return 1;
    }
    PASS();

    TEST("Comm_remote_group");
    ret = MPI_Comm_remote_group(intercomm, &remote_group);
    if (ret != MPI_SUCCESS) FAIL("MPI_Comm_remote_group failed");
    {
        int remote_grp_size;
        MPI_Group_size(remote_group, &remote_grp_size);
        if (remote_grp_size != half) {
            fprintf(stderr, "  FAIL: remote group size=%d (expected %d)\n", remote_grp_size, half);
            return 1;
        }
    }
    MPI_Group_free(&remote_group);
    PASS();

    TEST("Intercomm_merge");
    ret = MPI_Intercomm_merge(intercomm, 0, &merged);
    if (ret != MPI_SUCCESS) FAIL("MPI_Intercomm_merge failed");
    {
        int merged_size;
        MPI_Comm_size(merged, &merged_size);
        if (merged_size != size) {
            fprintf(stderr, "  FAIL: merged size=%d (expected %d)\n", merged_size, size);
            return 1;
        }
    }
    MPI_Comm_free(&merged);
    PASS();

    MPI_Comm_free(&intercomm);
    MPI_Comm_free(&color_comm);
    return 0;
}

int main(int argc, char **argv) {
    int ret;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed\n");
        return 1;
    }
    printf("=== MPI Intercommunicator Tests ===\n\n");
    ret = test_intercomm();
    if (ret != 0) goto cleanup;

    printf("\n=== All tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
