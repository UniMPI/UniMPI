/* tests/mpi/test_io_extended.c - MPI 2.2 I/O extension tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

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

static void make_test_path(char *path, size_t capacity, const char *kind) {
    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    memset(path, 0, capacity);
    if (rank == 0) {
        double nonce = MPI_Wtime() * 1000000000.0;
#ifdef _WIN32
        snprintf(path, capacity, "%s\\unimpi_%s_%d_%.0f.tmp",
                 get_temp_dir(), kind, size, nonce);
#else
        snprintf(path, capacity, "%s/unimpi_%s_%d_%.0f.tmp",
                 get_temp_dir(), kind, size, nonce);
#endif
    }
    TEST_CHECK_SUCCESS(MPI_Bcast(
        path, (int)capacity, MPI_CHAR, 0, MPI_COMM_WORLD));
}

static void delete_test_path(const char *path) {
    int rank;
    int result = MPI_SUCCESS;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Barrier(MPI_COMM_WORLD));
    if (rank == 0) {
        result = MPI_File_delete(path, MPI_INFO_NULL);
    }
    TEST_CHECK_SUCCESS(
        MPI_Bcast(&result, 1, MPI_INT, 0, MPI_COMM_WORLD));
    TEST_CHECK_SUCCESS(result);
    TEST_CHECK_SUCCESS(MPI_Barrier(MPI_COMM_WORLD));
}

int test_file_atomicity(void) {
    int rank;
    MPI_File fh;
    int flag;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("File_set_atomicity/File_get_atomicity");

    /* Open a file for testing */
    char path1[512];
    make_test_path(path1, sizeof(path1), "atomicity");
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
    delete_test_path(path1);
    PASS();
    return 0;
}

int test_file_sync(void) {
    int rank, size;
    MPI_File fh;
    MPI_Group file_group;
    MPI_Info info;
    MPI_Info used_info;
    MPI_Offset file_size = 0;
    MPI_Offset offset;
    MPI_Request request;
    MPI_Status status;
    int amode = 0;
    int group_size = 0;
    int readback = -1;
    int value;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    TEST("MPI-IO independent, collective, nonblocking and metadata paths");

    char path2[512];
    make_test_path(path2, sizeof(path2), "data");
    int ret = MPI_File_open(MPI_COMM_WORLD, path2,
                            MPI_MODE_CREATE | MPI_MODE_RDWR,
                            MPI_INFO_NULL, &fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_open failed");

    ret = MPI_File_set_size(fh, 0);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_set_size failed");

    value = 1000 + rank;
    offset = (MPI_Offset)rank * (MPI_Offset)sizeof(value);
    ret = MPI_File_write_at(fh, offset, &value, 1, MPI_INT, &status);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_write_at failed");

    /*
     * Independent writes need an explicit communicator synchronization
     * before another rank may enter the read phase.  In particular, MS-MPI
     * does not make this ordering portable through File_sync alone.
     */
    ret = MPI_Barrier(MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) FAIL("MPI_Barrier before MPI_File_sync failed");
    ret = MPI_File_sync(fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_sync failed");
    ret = MPI_Barrier(MPI_COMM_WORLD);
    if (ret != MPI_SUCCESS) FAIL("MPI_Barrier after MPI_File_sync failed");

    ret = MPI_File_read_at(fh, offset, &readback, 1, MPI_INT, &status);
    if (ret != MPI_SUCCESS || readback != value) {
        FAIL("MPI_File_read_at did not round-trip data");
    }

    ret = MPI_File_get_size(fh, &file_size);
    if (ret != MPI_SUCCESS ||
        file_size < (MPI_Offset)size * (MPI_Offset)sizeof(value)) {
        FAIL("MPI_File_get_size returned a short file");
    }

    ret = MPI_File_get_amode(fh, &amode);
    if (ret != MPI_SUCCESS || !(amode & MPI_MODE_RDWR)) {
        FAIL("MPI_File_get_amode returned the wrong mode");
    }

    ret = MPI_File_get_group(fh, &file_group);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_get_group failed");
    ret = MPI_Group_size(file_group, &group_size);
    if (ret != MPI_SUCCESS || group_size != size) {
        FAIL("MPI_File_get_group returned the wrong group");
    }
    MPI_Group_free(&file_group);

    ret = MPI_Info_create(&info);
    if (ret != MPI_SUCCESS) FAIL("MPI_Info_create failed");
    ret = MPI_Info_set(info, "unimpi_test_hint", "1");
    if (ret != MPI_SUCCESS) FAIL("MPI_Info_set failed");
    ret = MPI_File_set_info(fh, info);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_set_info failed");
    ret = MPI_File_get_info(fh, &used_info);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_get_info failed");
    MPI_Info_free(&used_info);
    MPI_Info_free(&info);

    /* Exercise collective positioned I/O at a second region. */
    value = 2000 + rank;
    offset = (MPI_Offset)(size + rank) * (MPI_Offset)sizeof(value);
    ret = MPI_File_write_at_all(fh, offset, &value, 1, MPI_INT, &status);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_write_at_all failed");
    readback = -1;
    ret = MPI_File_read_at_all(
        fh, offset, &readback, 1, MPI_INT, &status);
    if (ret != MPI_SUCCESS || readback != value) {
        FAIL("MPI_File_read_at_all did not round-trip data");
    }

    /* Exercise nonblocking positioned I/O at a third region. */
    value = 3000 + rank;
    offset = (MPI_Offset)(2 * size + rank) * (MPI_Offset)sizeof(value);
    ret = MPI_File_iwrite_at(fh, offset, &value, 1, MPI_INT, &request);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_iwrite_at failed");
    ret = MPI_Wait(&request, &status);
    if (ret != MPI_SUCCESS) FAIL("MPI_Wait for MPI_File_iwrite_at failed");
    readback = -1;
    ret = MPI_File_iread_at(
        fh, offset, &readback, 1, MPI_INT, &request);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_iread_at failed");
    ret = MPI_Wait(&request, &status);
    if (ret != MPI_SUCCESS || readback != value) {
        FAIL("MPI_File_iread_at did not round-trip data");
    }

    ret = MPI_File_close(&fh);
    if (ret != MPI_SUCCESS) FAIL("MPI_File_close failed");
    delete_test_path(path2);
    PASS();
    return 0;
}

int test_file_errhandler(void) {
    int rank;
    MPI_File fh;
    MPI_Errhandler errhandler;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    TEST("File_errhandler");

    char path3[512];
    make_test_path(path3, sizeof(path3), "errhandler");
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
    delete_test_path(path3);
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
