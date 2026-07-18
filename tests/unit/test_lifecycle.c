/* tests/unit/test_lifecycle.c - Lifecycle state machine tests
 * These tests use the real MPI backend and test the lifecycle state machine.
 * Each test is independent and calls init/finalize as needed.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    int rank = -1;
    (void)argv;

    printf("Running lifecycle tests...\n");

    /* Test 1: Cannot finalize before init */
    printf("  Test: finalize before init...\n");
    assert(unimpi_finalize() == UNIMPI_ERR_NOT_INITIALIZED);
    printf("    PASSED: returns UNIMPI_ERR_NOT_INITIALIZED\n");

    /* Test 2: Normal init/finalize cycle */
    printf("  Test: normal init/finalize...\n");
    assert(unimpi_init(&argc, NULL) == UNIMPI_OK);
    assert(unimpi_is_initialized());
    assert(unimpi_get_backend_name() != NULL);
    printf("    Backend: %s\n", unimpi_get_backend_name());
    assert(unimpi_finalize() == UNIMPI_OK);
    assert(!unimpi_is_initialized());
    assert(unimpi_get_backend_name() == NULL);
    printf("    PASSED\n");

    /* Test 3: After finalize, CANNOT re-initialize (MPI standard compliance) */
    printf("  Test: re-init after finalize...\n");
    assert(unimpi_init(NULL, NULL) == UNIMPI_ERR_FINALIZED);
    printf("    PASSED: returns UNIMPI_ERR_FINALIZED\n");

    /* Test 4: Invalid thread level */
    printf("  Test: invalid thread level...\n");
    int provided;
    assert(unimpi_init_thread(NULL, NULL, 999, &provided) == UNIMPI_ERR_INVALID_ARGUMENT);
    assert(!unimpi_is_initialized());
    printf("    PASSED: returns UNIMPI_ERR_INVALID_ARGUMENT\n");

    /* Test 5: Query functions work */
    printf("  Test: query functions...\n");
    int flag;
    assert(unimpi_mpi_initialized(&flag) == UNIMPI_OK);
    assert(flag == 1);  /* Was initialized (MPI_Initialized returns true after init) */
    assert(unimpi_mpi_finalized(&flag) == UNIMPI_OK);
    assert(flag == 1);  /* Was finalized */
    printf("    PASSED\n");

    printf("All lifecycle tests passed!\n");
    return 0;
}
