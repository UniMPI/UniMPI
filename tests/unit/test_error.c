#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "tftk_mpi.h"

void test_error_codes(void) {
    /* Test error code to string conversion */
    assert(strcmp(tftk_mpi_error_string(TFTK_MPI_OK), "Success") == 0);
    assert(strcmp(tftk_mpi_error_string(TFTK_MPI_ERR_NO_BACKEND),
                  "No MPI backend found") == 0);
    assert(strcmp(tftk_mpi_error_string(TFTK_MPI_ERR_NOT_INITIALIZED),
                  "TFTK-MPI not initialized") == 0);

    printf("  Error code tests passed\n");
}

void test_error_unknown(void) {
    /* Test unknown error code */
    const char *msg = tftk_mpi_error_string(-999);
    assert(strcmp(msg, "Unknown error") == 0);

    printf("  Unknown error test passed\n");
}

int main(void) {
    printf("Running error handling tests...\n");

    test_error_codes();
    test_error_unknown();

    printf("All error tests passed!\n");
    return 0;
}
