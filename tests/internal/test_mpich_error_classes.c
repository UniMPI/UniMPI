/* Regression: MPICH/Intel backend initializers must publish MPICH public
 * error-class values, not Open MPI numbering.
 *
 * Usage: test_mpich_error_classes <mpich_fake> <intelmpi_fake>
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_errors.h"
#include "unimpi_loader.h"

int unimpi_vtable_init_mpich(unimpi_lib_handle_t handle);
int unimpi_vtable_init_intelmpi(unimpi_lib_handle_t handle);

static void assert_mpich_compatible_error_classes(const char *backend_name) {
    /* Required anchors from the bug report. */
    assert(MPI_ERR_IN_STATUS == 17);
    assert(MPI_ERR_PENDING == 18);
    assert(MPI_ERR_REQUEST == 19);
    assert(MPI_ERR_NO_MEM == 34);

    /* Representative remaining MPICH public constants. */
    assert(MPI_SUCCESS == 0);
    assert(MPI_ERR_BUFFER == 1);
    assert(MPI_ERR_COUNT == 2);
    assert(MPI_ERR_TYPE == 3);
    assert(MPI_ERR_TAG == 4);
    assert(MPI_ERR_COMM == 5);
    assert(MPI_ERR_RANK == 6);
    assert(MPI_ERR_ROOT == 7);
    assert(MPI_ERR_GROUP == 8);
    assert(MPI_ERR_OP == 9);
    assert(MPI_ERR_TOPOLOGY == 10);
    assert(MPI_ERR_DIMS == 11);
    assert(MPI_ERR_ARG == 12);
    assert(MPI_ERR_UNKNOWN == 13);
    assert(MPI_ERR_TRUNCATE == 14);
    assert(MPI_ERR_OTHER == 15);
    assert(MPI_ERR_INTERN == 16);
    assert(MPI_ERR_ACCESS == 20);
    assert(MPI_ERR_AMODE == 21);
    assert(MPI_ERR_BAD_FILE == 22);
    assert(MPI_ERR_CONVERSION == 23);
    assert(MPI_ERR_DUP_DATAREP == 24);
    assert(MPI_ERR_FILE_EXISTS == 25);
    assert(MPI_ERR_FILE_IN_USE == 26);
    assert(MPI_ERR_FILE == 27);
    assert(MPI_ERR_INFO == 28);
    assert(MPI_ERR_INFO_KEY == 29);
    assert(MPI_ERR_INFO_VALUE == 30);
    assert(MPI_ERR_INFO_NOKEY == 31);
    assert(MPI_ERR_IO == 32);
    assert(MPI_ERR_NAME == 33);
    assert(MPI_ERR_NOT_SAME == 35);
    assert(MPI_ERR_NO_SPACE == 36);
    assert(MPI_ERR_NO_SUCH_FILE == 37);
    assert(MPI_ERR_PORT == 38);
    assert(MPI_ERR_QUOTA == 39);
    assert(MPI_ERR_READ_ONLY == 40);
    assert(MPI_ERR_SERVICE == 41);
    assert(MPI_ERR_SPAWN == 42);
    assert(MPI_ERR_UNSUPPORTED_DATAREP == 43);
    assert(MPI_ERR_UNSUPPORTED_OPERATION == 44);
    assert(MPI_ERR_WIN == 45);
    assert(MPI_ERR_BASE == 46);
    assert(MPI_ERR_LOCKTYPE == 47);
    assert(MPI_ERR_KEYVAL == 48);
    assert(MPI_ERR_RMA_CONFLICT == 49);
    assert(MPI_ERR_RMA_SYNC == 50);
    assert(MPI_ERR_SIZE == 51);
    assert(MPI_ERR_DISP == 52);
    assert(MPI_ERR_ASSERT == 53);
    assert(MPI_ERR_LASTCODE == 0x3fffffff);

    /* Stable MPIX process-failure class; unsupported stop uses INT_MIN. */
    assert(MPI_ERR_PROC_FAILED == 101);
    assert(MPI_ERR_PROC_FAIL_STOP == INT_MIN);
    assert(MPI_ERR_PROC_FAILED != MPI_ERR_OTHER);
    assert(MPI_ERR_PROC_FAIL_STOP != MPI_ERR_OTHER);
    /* Native 55 is MPI_ERR_RMA_RANGE, not PROC_FAIL_STOP / PROC_FAILED. */
    assert(MPI_ERR_PROC_FAIL_STOP != 55);
    assert(MPI_ERR_PROC_FAILED != 55);
    assert(strcmp(unimpi_mpi_error_string(55), "MPI_ERR_PROC_FAIL_STOP") != 0);
    assert(strcmp(unimpi_mpi_error_string(55), "MPI_ERR_PROC_FAILED") != 0);
    assert(strcmp(unimpi_mpi_error_string(101), "MPI_ERR_PROC_FAILED") == 0);
    assert(strcmp(unimpi_mpi_error_string(INT_MIN),
                  "MPI_ERR_PROC_FAIL_STOP") == 0);

    printf("  %s error classes match MPICH public constants\n", backend_name);
}

static void test_mpich_initializer(const char *library_path) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(library_path, &handle) == UNIMPI_OK);
    assert(handle != NULL);
    assert(unimpi_vtable_init_mpich(handle) == UNIMPI_OK);
    assert_mpich_compatible_error_classes("MPICH");
    unimpi_loader_unload(handle);
}

static void test_intelmpi_initializer(const char *library_path) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(library_path, &handle) == UNIMPI_OK);
    assert(handle != NULL);
    assert(unimpi_vtable_init_intelmpi(handle) == UNIMPI_OK);
    assert_mpich_compatible_error_classes("Intel MPI");
    unimpi_loader_unload(handle);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <mpich_fake> <intelmpi_fake>\n",
                argv[0]);
        return 2;
    }

    printf("Running MPICH-compatible error-class regression...\n");
    test_mpich_initializer(argv[1]);
    test_intelmpi_initializer(argv[2]);
    printf("MPICH-compatible error-class regression passed\n");
    return 0;
}
