#include "unimpi.h"
#include <stdio.h>

/* AT_LEAST truth table, relative to the compiled-in target
 * (UNIMPI_MPI_TARGET_VERSION / UNIMPI_MPI_TARGET_SUBVERSION). Each assertion is
 * universally true at ANY target, so this positive gate stays meaningful whether
 * the branch is configured for the default (3.1) or a 2.2 target. */
#if !UNIMPI_MPI_AT_LEAST(UNIMPI_MPI_TARGET_VERSION, UNIMPI_MPI_TARGET_SUBVERSION)
#error "target must satisfy AT_LEAST(target)"
#endif
#if !UNIMPI_MPI_AT_LEAST(UNIMPI_MPI_TARGET_VERSION, 0)
#error "target must satisfy AT_LEAST(target, 0)"
#endif
#if !UNIMPI_MPI_AT_LEAST(UNIMPI_MPI_TARGET_VERSION - 1, 0)
#error "target must satisfy AT_LEAST(target - 1, 0)"
#endif
#if UNIMPI_MPI_AT_LEAST(UNIMPI_MPI_TARGET_VERSION + 1, 0)
#error "target must NOT satisfy AT_LEAST(target + 1, 0)"
#endif
#if UNIMPI_MPI_AT_LEAST(UNIMPI_MPI_TARGET_VERSION, UNIMPI_MPI_TARGET_SUBVERSION + 1)
#error "target must NOT satisfy AT_LEAST(target, subversion + 1)"
#endif

int main(void) {
    /* MPI-3.0 macros are present only when the target satisfies >= 3.0. */
#ifdef MPI_Ibcast
    printf("IBCAST_PRESENT\n");
#else
    printf("IBCAST_ABSENT\n");
#endif
    return 0;
}
