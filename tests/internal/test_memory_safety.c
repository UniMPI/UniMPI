/* tests/internal/test_memory_safety.c - Memory safety tests */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"
#include "unimpi_vtable.h"

void test_multiple_init_finalize(void) {
    int ret;

    /* First init/finalize cycle */
    ret = unimpi_init(NULL, NULL);
    assert(ret == UNIMPI_OK);

    const char *backend = unimpi_get_backend_name();
    assert(backend != NULL);

    ret = unimpi_finalize();
    assert(ret == UNIMPI_OK);

    /* After finalize, cannot re-init */
    ret = unimpi_init(NULL, NULL);
    assert(ret == UNIMPI_ERR_FINALIZED);

    printf("  Multiple init/finalize test passed\n");
}

void test_null_pointer_handling(void) {
    int ret;
    int flag;

    /* Test MPI_Initialized with NULL */
    ret = unimpi_mpi_initialized(NULL);
    assert(ret == UNIMPI_ERR_INVALID_ARGUMENT);

    /* Test MPI_Finalized with NULL */
    ret = unimpi_mpi_finalized(NULL);
    assert(ret == UNIMPI_ERR_INVALID_ARGUMENT);

    /* Test with valid pointer before init */
    ret = unimpi_mpi_initialized(&flag);
    assert(ret == UNIMPI_OK);
    assert(flag == 0);

    printf("  Null pointer handling test passed\n");
}

void test_version_info(void) {
    int version, subversion;
    int ret;

    ret = unimpi_mpi_get_version(&version, &subversion);
    assert(ret == UNIMPI_OK);
    assert(version == UNIMPI_MPI_VERSION);
    assert(subversion == UNIMPI_MPI_SUBVERSION);

    printf("  Version info test passed\n");
}

int main(void) {
    printf("Running memory safety tests...\n");

    test_null_pointer_handling();
    test_version_info();
    test_multiple_init_finalize();

    printf("All memory safety tests passed!\n");
    return 0;
}
