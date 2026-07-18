/* test_mpi_helpers.h - Helper macros for MPI tests */
#ifndef TEST_MPI_HELPERS_H
#define TEST_MPI_HELPERS_H

#include <stdio.h>
#include <stdlib.h>

static void test_check_success(int result, const char *expression,
                               const char *file, int line) {
    if (result != 0) {
        fprintf(stderr, "%s:%d: %s returned error code %d\n",
                file, line, expression, result);
        abort();
    }
}

#define TEST_CHECK_SUCCESS(expression) \
    test_check_success((expression), #expression, __FILE__, __LINE__)

#endif /* TEST_MPI_HELPERS_H */
