/* tests/internal/test_platform.c - Platform abstraction tests */
#include <stdio.h>
#include <assert.h>
#include "unimpi.h"
#include "unimpi_platform.h"

void test_dlopen_invalid(void) {
    unimpi_lib_handle_t handle;

    /* Try to open non-existent library */
    handle = unimpi_platform_dlopen("/nonexistent/library.so");
    assert(handle == NULL);

    printf("  Invalid dlopen test passed\n");
}

void test_dlsym_null_handle(void) {
    void *sym;

    /* Try to get symbol with NULL handle */
    sym = unimpi_platform_dlsym(NULL, "MPI_Init");
    assert(sym == NULL);

    printf("  Null handle dlsym test passed\n");
}

void test_error_strings(void) {
    const char *str;

    /* Test valid error codes */
    str = unimpi_error_string(UNIMPI_OK);
    assert(str != NULL);
    printf("  UNIMPI_OK: %s\n", str);

    str = unimpi_error_string(UNIMPI_ERR_NO_BACKEND);
    assert(str != NULL);
    printf("  UNIMPI_ERR_NO_BACKEND: %s\n", str);

    str = unimpi_error_string(UNIMPI_ERR_BACKEND_LOAD);
    assert(str != NULL);
    printf("  UNIMPI_ERR_BACKEND_LOAD: %s\n", str);

    /* Test unknown error code */
    str = unimpi_error_string(9999);
    assert(str != NULL);

    printf("  Error strings test passed\n");
}

void test_mpi_abi_types(void) {
    assert(sizeof(MPI_Aint) == sizeof(intptr_t));
    assert(sizeof(MPI_Aint) == sizeof(void *));

    printf("  MPI_Aint ABI width test passed\n");
}

int main(void) {
    printf("Running platform tests...\n");

    test_dlopen_invalid();
    test_dlsym_null_handle();
    test_error_strings();
    test_mpi_abi_types();

    printf("All platform tests passed!\n");
    return 0;
}
