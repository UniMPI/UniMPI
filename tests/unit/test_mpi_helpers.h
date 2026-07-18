/* test_mpi_helpers.h - Helper macros for MPI tests */
#ifndef TEST_MPI_HELPERS_H
#define TEST_MPI_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

/* Macro to check MPI function success and abort test on failure */
#define TEST_CHECK_SUCCESS(expr) do { \
    int _ret = (expr); \
    if (_ret != 0) { \
        fprintf(stderr, "Test failed: %s returned %d at %s:%d\n", \
                #expr, _ret, __FILE__, __LINE__); \
        MPI_Abort(MPI_COMM_WORLD, 1); \
        exit(1); \
    } \
} while (0)

#endif /* TEST_MPI_HELPERS_H */
