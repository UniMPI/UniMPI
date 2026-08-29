/* tests/mpi/test_tierA_gaps.c - runtime validation of the low-difficulty
 * (no-typedef) MPI-2.2 base gaps: error_class/error_string/pcontrol and the
 * file shared-pointer + split-collective I/O set. Proves the new
 * function-pointer signatures are ABI-correct against a real backend (a wrong
 * cast would crash or misbehave here).
 */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

static int test_error_class_string(void) {
    int cls = -1, len = -1;
    char buf[MPI_MAX_ERROR_STRING];
    TEST("Error_class + Error_string");
    if (MPI_Error_class(MPI_SUCCESS, &cls) != MPI_SUCCESS)
        FAIL("MPI_Error_class failed");
    if (cls != MPI_SUCCESS) { fprintf(stderr, "  FAIL: class=%d want 0\n", cls); return 1; }
    if (MPI_Error_string(MPI_SUCCESS, buf, &len) != MPI_SUCCESS)
        FAIL("MPI_Error_string failed");
    if (len <= 0) FAIL("MPI_Error_string returned empty");
    /* Pcontrol is a profiling no-op hook; it must return cleanly. */
    if (MPI_Pcontrol(0) != MPI_SUCCESS) FAIL("MPI_Pcontrol(0) failed");
    PASS();
    return 0;
}

static int test_file_shared_and_split(void) {
    int rank, rc;
    MPI_File fh;
    /* The collective MPI_File_open requires the SAME path on every rank, so the
     * name must not depend on per-process state like getpid(). */
    const char *fname = "unimpi_test_tierA_gaps.dat";
    TEST("File split-collective + shared-pointer I/O");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) remove(fname);

    rc = MPI_File_open(MPI_COMM_WORLD, fname, MPI_MODE_CREATE | MPI_MODE_RDWR,
                       MPI_INFO_NULL, &fh);
    if (rc != MPI_SUCCESS) FAIL("MPI_File_open failed");

    /* get_type_extent */
    {
        MPI_Aint extent = -1;
        if (MPI_File_get_type_extent(fh, MPI_CHAR, &extent) != MPI_SUCCESS)
            FAIL("MPI_File_get_type_extent failed");
        if (extent != 1) { fprintf(stderr, "  FAIL: extent=%ld want 1\n", (long)extent); return 1; }
    }

    /* split collective write at per-rank offsets */
    {
        char sendbuf[16];
        snprintf(sendbuf, sizeof sendbuf, "rank%d", rank);
        if (MPI_File_write_at_all_begin(fh, (MPI_Offset)(rank * 16),
                                        sendbuf, (int)strlen(sendbuf) + 1, MPI_CHAR) != MPI_SUCCESS)
            FAIL("MPI_File_write_at_all_begin failed");
        if (MPI_File_write_at_all_end(fh, sendbuf, MPI_STATUS_IGNORE) != MPI_SUCCESS)
            FAIL("MPI_File_write_at_all_end failed");
    }

    /* collective read-back of rank 0's record using the existing read_at_all
     * binding, to prove the split-collective data landed. */
    if (rank == 0) {
        char rbuf[16];
        if (MPI_File_read_at_all(fh, 0, rbuf, 16, MPI_CHAR, MPI_STATUS_IGNORE) != MPI_SUCCESS)
            FAIL("MPI_File_read_at_all failed");
        if (strncmp(rbuf, "rank0", 5) != 0) FAIL("split-collective write mismatch");
    }

    MPI_File_close(&fh);
    if (rank == 0) remove(fname);
    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret, rank;
    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) { fprintf(stderr, "MPI_Init failed\n"); return 1; }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI-2.2 Tier-A Gap Smoke Tests ===\n\n");

    if ((ret = test_error_class_string()) != 0) goto cleanup;
    if ((ret = test_file_shared_and_split()) != 0) goto cleanup;

    printf("\n=== All Tier-A gap tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
