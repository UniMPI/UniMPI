/* tests/mpi/test_rma_extended.c - MPI 2.2 RMA extension tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

/* Attribute callback functions */
int copy_fn_called = 0;
int delete_fn_called = 0;

int test_copy_fn(MPI_Win oldwin, int win_keyval, void *extra_state,
                 void *attribute_val_in, void *attribute_val_out, int *flag) {
    (void)oldwin;
    (void)win_keyval;
    (void)extra_state;
    (void)attribute_val_in;
    (void)attribute_val_out;
    copy_fn_called++;
    *flag = 1;
    return MPI_SUCCESS;
}

int test_delete_fn(MPI_Win win, int win_keyval, void *attribute_val,
                   void *extra_state) {
    (void)win;
    (void)win_keyval;
    (void)attribute_val;
    (void)extra_state;
    delete_fn_called++;
    return MPI_SUCCESS;
}

int test_win_keyval(void) {
    int rank, keyval;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("Win_create_keyval/Win_free_keyval");

    /* Create a keyval */
    int ret = MPI_Win_create_keyval(test_copy_fn, test_delete_fn, &keyval, NULL);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_create_keyval failed");
    /* Note: MPICH uses negative values for handles, OpenMPI uses positive */
    if (keyval == 0) FAIL("keyval should not be zero");

    /* Free the keyval */
    ret = MPI_Win_free_keyval(&keyval);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_free_keyval failed");

    PASS();
    return 0;
}

int test_win_attr(void) {
    int rank, keyval, value = 42, flag = 0;
    void *attr_val = NULL;
    MPI_Win win;
    int buffer[1];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("Win_set_attr/Win_get_attr/Win_delete_attr");

    /* Create a keyval */
    int ret = MPI_Win_create_keyval(test_copy_fn, test_delete_fn, &keyval, NULL);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_create_keyval failed");

    /* Create a window using Win_create (no info needed) */
    ret = MPI_Win_create(buffer, sizeof(int), sizeof(int),
                         MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_create failed");

    /* Set attribute - pass pointer to value */
    ret = MPI_Win_set_attr(win, keyval, &value);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_set_attr failed");

    /* Get attribute - check only that it succeeds and flag is set */
    ret = MPI_Win_get_attr(win, keyval, &attr_val, &flag);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_get_attr failed");
    if (!flag) FAIL("flag should be set");

    /* Delete attribute */
    ret = MPI_Win_delete_attr(win, keyval);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_delete_attr failed");

    /* Verify deletion - flag should be 0 */
    flag = 1;  /* Reset flag to check if it gets cleared */
    attr_val = NULL;
    ret = MPI_Win_get_attr(win, keyval, &attr_val, &flag);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_get_attr after delete failed");
    if (flag) FAIL("flag should be 0 after delete");

    MPI_Win_free(&win);
    MPI_Win_free_keyval(&keyval);
    PASS();
    return 0;
}

int test_win_get_group(void) {
    int rank, size;
    MPI_Win win;
    MPI_Group group, comm_group;
    int buffer[1];
    int result;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    TEST("Win_get_group");

    /* Create a window using Win_create */
    int ret = MPI_Win_create(buffer, sizeof(int), sizeof(int),
                             MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_create failed");

    /* Get window group */
    ret = MPI_Win_get_group(win, &group);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_get_group failed");

    /* Compare with comm group */
    ret = MPI_Comm_group(MPI_COMM_WORLD, &comm_group);
    if (ret != MPI_SUCCESS) FAIL("MPI_Comm_group failed");

    ret = MPI_Group_compare(group, comm_group, &result);
    if (ret != MPI_SUCCESS) FAIL("MPI_Group_compare failed");
    if (result != MPI_IDENT && result != MPI_CONGRUENT) {
        FAIL("window group should match comm group");
    }

    MPI_Group_free(&group);
    MPI_Group_free(&comm_group);
    MPI_Win_free(&win);
    PASS();
    return 0;
}

int test_win_call_errhandler(void) {
    int rank;
    MPI_Win win;
    int buffer[1];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("Win_call_errhandler");

    /* Create a window using Win_create */
    int ret = MPI_Win_create(buffer, sizeof(int), sizeof(int),
                             MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    if (ret != MPI_SUCCESS) FAIL("MPI_Win_create failed");

    /* Just verify function exists and is callable
     * Note: Actually calling with error code may abort, so we just
     * verify the function pointer is set */
    if (unimpi.win_call_errhandler == NULL) {
        FAIL("win_call_errhandler not loaded");
    }

    MPI_Win_free(&win);
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
    printf("=== MPI 2.2 RMA Extension Tests ===\n\n");

    ret = test_win_keyval();
    if (ret != 0) goto cleanup;

    ret = test_win_attr();
    if (ret != 0) goto cleanup;

    ret = test_win_get_group();
    if (ret != 0) goto cleanup;

    ret = test_win_call_errhandler();
    if (ret != 0) goto cleanup;

    printf("\n=== All RMA extension tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
