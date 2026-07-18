#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"

void test_error_codes(void) {
    /* Test error code to string conversion */
    assert(strcmp(unimpi_error_string(UNIMPI_OK), "Success") == 0);
    assert(strcmp(unimpi_error_string(UNIMPI_ERR_NO_BACKEND),
                  "No MPI backend found") == 0);
    assert(strcmp(unimpi_error_string(UNIMPI_ERR_NOT_INITIALIZED),
                  "TFTK-MPI not initialized") == 0);

    printf("  Error code tests passed\n");
}

void test_error_unknown(void) {
    /* Test unknown error code */
    const char *msg = unimpi_error_string(-999);
    assert(strcmp(msg, "Unknown error") == 0);

    printf("  Unknown error test passed\n");
}

void test_error_class_rejects_null_output(void) {
    assert(unimpi_error_class(UNIMPI_OK, NULL) == UNIMPI_ERR_INVALID_ARGUMENT);

    printf("  Error class output validation passed\n");
}

int main(void) {
    printf("Running error handling tests...\n");

    test_error_codes();
    test_error_unknown();
    test_error_class_rejects_null_output();

    printf("All error tests passed!\n");
    return 0;
}
