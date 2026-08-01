/* Open MPI completion-array binder regressions.
 *
 * 1) Real ompi_request_t / ompi_status_public_t tags via fake DSO after
 *    unimpi_vtable_init_openmpi (UBSan function-type contract).
 * 2) Missing-symbol identity fixture leaves waitall/testall/testsome/waitsome
 *    NULL when those symbols are absent.
 *
 * Usage:
 *   test_openmpi_request_arrays <openmpi_api_fake> <openmpi_identity_fake>
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_errors.h"
#include "unimpi_loader.h"
#include "unimpi_vtable.h"

enum {
    FAKE_ERR_IN_STATUS_CODE = 0x10000 | 18
};

int unimpi_vtable_init_openmpi(unimpi_lib_handle_t handle);

static void assert_array_slots_present(void) {
    assert(unimpi.waitall != NULL);
    assert(unimpi.testall != NULL);
    assert(unimpi.testsome != NULL);
    assert(unimpi.waitsome != NULL);
}

static void assert_array_slots_null(void) {
    assert(unimpi.waitall == NULL);
    assert(unimpi.testall == NULL);
    assert(unimpi.testsome == NULL);
    assert(unimpi.waitsome == NULL);
}

static void test_openmpi_array_paths(const char *path) {
    unimpi_lib_handle_t handle = NULL;
    MPI_Request requests[2];
    MPI_Status statuses[2];
    int flag = 0;
    int outcount = 0;
    int indices[2] = {-1, -1};

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    assert(unimpi_vtable_init_openmpi(handle) == UNIMPI_OK);
    assert_array_slots_present();

    assert(MPI_ERR_IN_STATUS == 18);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    memset(statuses, 0xff, sizeof(statuses));
    assert(unimpi.waitall(2, requests, statuses) == 0);
    assert(requests[0] == 0);
    assert(requests[1] == 0);
    assert(statuses[0].openmpi.MPI_SOURCE == 10);
    assert(statuses[1].openmpi.MPI_SOURCE == 11);
    assert(statuses[0].openmpi._ucount == 30);
    assert(statuses[1].openmpi._ucount == 31);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    flag = 1;
    memset(statuses, 0xff, sizeof(statuses));
    assert(unimpi.testall(2, requests, &flag, statuses) ==
           FAKE_ERR_IN_STATUS_CODE);
    assert(flag == 0);
    assert(requests[0] == 0);
    assert(requests[1] == 0);
    assert(statuses[0].openmpi.MPI_ERROR == 18);
    assert(statuses[0].openmpi._ucount == 60);
    assert(statuses[1].openmpi._ucount == 61);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    outcount = 0;
    indices[0] = -1;
    memset(statuses, 0xff, sizeof(statuses));
    assert(unimpi.testsome(2, requests, &outcount, indices, statuses) == 0);
    assert(outcount == 1);
    assert(indices[0] == 0);
    assert(requests[0] == 0);
    assert(statuses[0].openmpi._ucount == 71);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    outcount = 0;
    assert(unimpi.waitsome(2, requests, &outcount, indices, statuses) == 0);
    assert(outcount == 1);

    unimpi_loader_unload(handle);
    printf("  Open MPI array binder paths passed\n");
}

static void test_openmpi_missing_array_symbols(const char *path) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    assert(unimpi_vtable_init_openmpi(handle) == UNIMPI_OK);
    assert_array_slots_null();
    unimpi_loader_unload(handle);
    printf("  Open MPI missing array symbols remain NULL\n");
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <openmpi_api_fake> <openmpi_identity_fake>\n",
                argv[0]);
        return 2;
    }

    printf("Running Open MPI request-array regressions...\n");
    test_openmpi_array_paths(argv[1]);
    test_openmpi_missing_array_symbols(argv[2]);
    printf("Open MPI request-array regressions passed\n");
    return 0;
}
