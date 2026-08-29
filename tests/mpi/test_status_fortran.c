/* tests/mpi/test_status_fortran.c - runtime validation of the MPI-2
 * Fortran-status conversion bindings (Status_f2c / Status_c2f).
 *
 * A real receive fills an MPI_Status; we c2f it into an MPI_Fint array, f2c it
 * back, and verify (a) MPI_Get_count reports the same count before and after,
 * proving the converted C status is a valid backend-native status (a wrong
 * cast would crash or misbehave), and (b) f2c(c2f(s)) reproduces the same
 * Fortran array (round-trip identity). No MPI_Status internal layout is
 * assumed, keeping the test portable across OpenMPI/MPICH/Intel/MS-MPI.
 */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

static int test_status_roundtrip(void) {
    int rank, nprocs, i;
    const int tag = 77;
    MPI_Status status, status2;
    int count_before = -1, count_after = -1;
    MPI_Fint fs[MPI_STATUS_SIZE];

    TEST("Recv + Status_c2f/f2c round trip");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (nprocs < 2) { printf("  SKIP (need >=2 ranks)\n"); return 0; }

    if (rank == 0) {
        int val = 42;
        if (MPI_Send(&val, 1, MPI_INT, 1, tag, MPI_COMM_WORLD) != MPI_SUCCESS)
            FAIL("MPI_Send failed");
    } else {
        int val = 0;
        if (MPI_Recv(&val, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status) != MPI_SUCCESS)
            FAIL("MPI_Recv failed");
        if (MPI_Get_count(&status, MPI_INT, &count_before) != MPI_SUCCESS)
            FAIL("MPI_Get_count(before) failed");

        /* C status -> Fortran INTEGER array, then back to a C status. */
        if (MPI_Status_c2f(&status, fs) != MPI_SUCCESS)
            FAIL("MPI_Status_c2f failed");
        if (MPI_Status_f2c(fs, &status2) != MPI_SUCCESS)
            FAIL("MPI_Status_f2c failed");

        if (MPI_Get_count(&status2, MPI_INT, &count_after) != MPI_SUCCESS)
            FAIL("MPI_Get_count(after) failed");
        if (count_before != 1 || count_after != 1) {
            fprintf(stderr, "  FAIL: count before=%d after=%d want both 1\n",
                    count_before, count_after);
            return 1;
        }

        /* Round-trip identity: c2f(f2c(c2f(s))) == c2f(s), backend-agnostic. */
        {
            MPI_Fint fs2[MPI_STATUS_SIZE];
            if (MPI_Status_c2f(&status2, fs2) != MPI_SUCCESS)
                FAIL("MPI_Status_c2f(re) failed");
            for (i = 0; i < MPI_STATUS_SIZE; i++) {
                if (fs[i] != fs2[i]) {
                    fprintf(stderr, "  FAIL: fs[%d]=%d want %d\n", i, fs[i], fs2[i]);
                    return 1;
                }
            }
        }
    }

    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret, rank;
    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) { fprintf(stderr, "MPI_Init failed\n"); return 1; }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI-2 Fortran-Status Conversion Tests ===\n\n");

    if ((ret = test_status_roundtrip()) != 0) goto cleanup;

    printf("\n=== All Fortran-status tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
