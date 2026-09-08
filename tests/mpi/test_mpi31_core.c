#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unimpi.h"

/* MPI-3.1 additions integration suite (Annex B.1.2).
 *
 * Exercises the six MPI-3.1 additions through the standard names:
 *   - MPI_Aint_add / MPI_Aint_diff (address arithmetic, pure scalar pass-through)
 *   - MPI_File_iread_all / MPI_File_iwrite_all / MPI_File_iread_at_all /
 *     MPI_File_iwrite_at_all (nonblocking collective file I/O)
 *
 * Address arithmetic is self-consistent (no backend value assumed), so it holds
 * across MPICH-family, Intel-MPI, MS-MPI and OpenMPI regardless of their
 * differing handle encodings. The file I/O case degrades to a skip when a
 * backend has not bound the *_all slots (mirroring test_mpi30_core's
 * degrade-skip for unsupported windows) -- but on a standard MPI-3.1 backend it
 * executes a real write/read round-trip through every new routine.
 */

#define CHECK(e) do {                                          \
    int _rc = (e);                                             \
    if (_rc != 0) {                                            \
        fprintf(stderr, "FAIL %s:%d: %s -> %d\n",              \
                __FILE__, __LINE__, #e, _rc);                  \
        return 1;                                              \
    }                                                          \
} while (0)

/* Address arithmetic is pure pass-through; self-consistency asserts the fields
 * are bound and correct without depending on any backend-specific value. */
static int test_aint_add_diff(void) {
    MPI_Aint base = (MPI_Aint)1000;
    MPI_Aint disp = (MPI_Aint)234;
    MPI_Aint sum = MPI_Aint_add(base, disp);
    if (sum != base + disp) {
        fprintf(stderr, "FAIL aint_add: %ld + %ld = %ld (got %ld)\n",
                (long)base, (long)disp, (long)(base + disp), (long)sum);
        return 1;
    }
    MPI_Aint addr1 = (MPI_Aint)2000;
    MPI_Aint addr2 = (MPI_Aint)1500;
    MPI_Aint diff = MPI_Aint_diff(addr1, addr2);
    if (diff != addr1 - addr2) {
        fprintf(stderr, "FAIL aint_diff: %ld - %ld = %ld (got %ld)\n",
                (long)addr1, (long)addr2, (long)(addr1 - addr2), (long)diff);
        return 1;
    }
    /* Round-trip: add(base,disp) then diff back to base. */
    if (MPI_Aint_diff(MPI_Aint_add(base, disp), disp) != base) {
        fprintf(stderr, "FAIL aint round-trip\n");
        return 1;
    }
    printf("  aint_add/aint_diff arithmetic OK\n");
    return 0;
}

#define M31_IO_FILE "unimpi_m31_io.bin"

/* Nonblocking collective file I/O. Not all backends bind the *_all slots;
 * degrade to a skip when absent. A rank-0 local round-trip exercises all four
 * routines (iwrite_all, iwrite_at_all, iread_all, iread_at_all) for real. */
static int test_file_nonblocking_all(int rank) {
    if (unimpi.file_iwrite_all == NULL || unimpi.file_iread_all == NULL ||
        unimpi.file_iwrite_at_all == NULL || unimpi.file_iread_at_all == NULL) {
        printf("  nonblocking_io_all not bound by this backend (skip)\n");
        return 0;
    }
    MPI_File fh;
    MPI_Status st;
    MPI_Request req;

    /* Write 3 ints at offsets 0/1/2, then overwrite slot 1 with a distinct
     * value via the at-offset variant; read everything back and verify. */
    int data[3] = {11, 22, 33};
    if (rank == 0) {
        CHECK(MPI_File_open(MPI_COMM_SELF, M31_IO_FILE,
                            MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh));
        CHECK(MPI_File_iwrite_all(fh, data, 3, MPI_INT, &req));
        CHECK(MPI_Wait(&req, &st));
        int slot = 99;
        MPI_Offset slot_off = (MPI_Offset)sizeof(int);
        CHECK(MPI_File_iwrite_at_all(fh, slot_off, &slot, 1, MPI_INT, &req));
        CHECK(MPI_Wait(&req, &st));
        CHECK(MPI_File_close(&fh));

        int back[3] = {0, 0, 0};
        CHECK(MPI_File_open(MPI_COMM_SELF, M31_IO_FILE,
                            MPI_MODE_RDONLY, MPI_INFO_NULL, &fh));
        CHECK(MPI_File_iread_all(fh, back, 3, MPI_INT, &req));
        CHECK(MPI_Wait(&req, &st));
        /* Re-read the overwritten slot via iread_at_all. */
        int slot2 = 0;
        CHECK(MPI_File_iread_at_all(fh, slot_off, &slot2, 1, MPI_INT, &req));
        CHECK(MPI_Wait(&req, &st));
        CHECK(MPI_File_close(&fh));

        if (back[0] != 11 || back[2] != 33) {
            fprintf(stderr, "FAIL file round-trip data [%d,%d,%d]\n",
                    back[0], back[1], back[2]);
            return 1;
        }
        if (slot2 != 99) {
            fprintf(stderr, "FAIL file at_all readback %d\n", slot2);
            return 1;
        }
        remove(M31_IO_FILE);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0)
        printf("  file *_all nonblocking I/O round-trip OK\n");
    return 0;
}

int main(int argc, char **argv) {
    int rank = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
        printf("=== MPI-3.1 additions tests ===\n");
    if (rank == 0 && test_aint_add_diff()) goto fail;
    if (test_file_nonblocking_all(rank)) goto fail;
    if (rank == 0)
        printf("=== All MPI-3.1 additions tests passed ===\n");
    MPI_Finalize();
    return 0;
fail:
    MPI_Finalize();
    return 1;
}
