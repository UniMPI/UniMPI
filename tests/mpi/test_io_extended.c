/* tests/mpi/test_io_extended.c - MPI 2.2 I/O extension tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

/* Get temp directory for current platform */
static const char* get_temp_dir(void) {
#ifdef _WIN32
    const char* tmp = getenv("TEMP");
    return tmp ? tmp : ".";
#else
    return "/tmp";
#endif
}

int test_file_atomicity(void) {
    int rank;
    MPI_File fh;
    int flag;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("File_set_atomicity/File_get_atomicity");

    /* Open a file for testing */
    char path1[256];
    snprintf(path1, sizeof(path1), "%s/test_atomicity.txt", get_temp_dir());
    int ret = MPI_File_open(MPI_COMM_WORLD, path1,
                            MPI_MODE_CREATE | MPI_MODE_RDWR,
                            MPI_INFO_NULL, &fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_open failed");

    /* Set atomicity mode */
    ret = MPI_File_set_atomicity(fh, 1);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_set_atomicity failed");

    /* Get atomicity mode */
    flag = 0;
    ret = MPI_File_get_atomicity(fh, &flag);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_get_atomicity failed");
    if (!flag) FAIL("atomicity flag should be 1");

    /* Restore non-atomic */
    ret = MPI_File_set_atomicity(fh, 0);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_set_atomicity(0) failed");

    MPI_File_close(&fh);
    PASS();
    return 0;
}

int test_file_sync(void) {
    int rank;
    MPI_File fh;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("File_sync");

    char path2[256];
    snprintf(path2, sizeof(path2), "%s/test_sync.txt", get_temp_dir());
    int ret = MPI_File_open(MPI_COMM_WORLD, path2,
                            MPI_MODE_CREATE | MPI_MODE_RDWR,
                            MPI_INFO_NULL, &fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_open failed");

    /* Write some data then sync */
    MPI_Status status;
    char buf[32] = "sync test data";
    ret = MPI_File_write(fh, buf, 16, MPI_BYTE, &status);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_write failed");

    ret = MPI_File_sync(fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_sync failed");

    MPI_File_close(&fh);
    PASS();
    return 0;
}

int test_file_errhandler(void) {
    int rank;
    MPI_File fh;
    MPI_Errhandler errhandler;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("File_errhandler");

    char path3[256];
    snprintf(path3, sizeof(path3), "%s/test_errhandler.txt", get_temp_dir());
    int ret = MPI_File_open(MPI_COMM_WORLD, path3,
                            MPI_MODE_CREATE | MPI_MODE_RDWR,
                            MPI_INFO_NULL, &fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_open failed");

    /* Test get/set errhandler */
    ret = MPI_File_get_errhandler(fh, &errhandler);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_get_errhandler failed");

    ret = MPI_File_set_errhandler(fh, errhandler);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_set_errhandler failed");

    /* Verify function pointer is loaded */
    if (unimpi.file_call_errhandler == NULL) {
        FAIL("file_call_errhandler not loaded");
    }

    MPI_File_close(&fh);
    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret, rank;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed\n");
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI 2.2 I/O Extension Tests ===\n\n");

    ret = test_file_atomicity();
    if (ret != 0) goto cleanup;

    ret = test_file_sync();
    if (ret != 0) goto cleanup;

    ret = test_file_errhandler();
    if (ret != 0) goto cleanup;

    printf("\n=== All I/O extension tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
