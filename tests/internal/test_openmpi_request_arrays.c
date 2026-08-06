/* Open MPI completion-array binder regressions.
 *
 * Covers all seven Open MPI request-array facade entry points bound through
 * the production openmpi backend initializer against a fake DSO that exports
 * real ompi_request_t / ompi_status_public_t native tags:
 *   Waitall, Testall, Testsome, Waitsome, Testany, Waitany, Startall
 *
 * Missing-symbol identity fixture leaves those slots NULL.
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
    assert(unimpi.wait != NULL);
    assert(unimpi.waitall != NULL);
    assert(unimpi.testall != NULL);
    assert(unimpi.testsome != NULL);
    assert(unimpi.waitsome != NULL);
    assert(unimpi.testany != NULL);
    assert(unimpi.waitany != NULL);
    assert(unimpi.startall != NULL);
}

static void assert_array_slots_null(void) {
    assert(unimpi.waitall == NULL);
    assert(unimpi.testall == NULL);
    assert(unimpi.testsome == NULL);
    assert(unimpi.waitsome == NULL);
    assert(unimpi.testany == NULL);
    assert(unimpi.waitany == NULL);
    assert(unimpi.startall == NULL);
}

static void test_openmpi_array_paths(const char *path) {
    unimpi_lib_handle_t handle = NULL;
    MPI_Request requests[2];
    MPI_Status statuses[2];
    MPI_Status status;
    int flag = 0;
    int outcount = 0;
    int indices[2] = {-1, -1};
    int index = -1;

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    assert(unimpi_vtable_init_openmpi(handle) == UNIMPI_OK);
    assert_array_slots_present();

    assert(MPI_ERR_IN_STATUS == 18);
    assert(UNIMPI_STATUS_IGNORE == NULL);
    assert(UNIMPI_STATUS_IGNORE == UNIMPI_STATUSES_IGNORE);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    assert(unimpi.wait(&requests[0], UNIMPI_STATUS_IGNORE) == 0);
    assert(requests[0] == 0);

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

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    flag = 0;
    index = -1;
    memset(&status, 0xff, sizeof(status));
    assert(unimpi.testany(2, requests, &index, &flag, &status) == 0);
    assert(flag == 1);
    assert(index == 1);
    assert(requests[1] == 0);
    assert(requests[0] == (MPI_Request)(intptr_t)0x51);
    assert(status.openmpi.MPI_SOURCE == 80);
    assert(status.openmpi._ucount == 82);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    index = -1;
    memset(&status, 0xff, sizeof(status));
    assert(unimpi.waitany(2, requests, &index, &status) == 0);
    assert(index == 1);
    assert(requests[1] == 0);
    assert(status.openmpi.MPI_SOURCE == 80);

    requests[0] = (MPI_Request)(intptr_t)0x51;
    requests[1] = (MPI_Request)(intptr_t)0x52;
    assert(unimpi.startall(2, requests) == 0);
    /* Fake ORs 0x100 into live handles to prove native pointer round-trip. */
    assert(requests[0] == (MPI_Request)(intptr_t)0x151);
    assert(requests[1] == (MPI_Request)(intptr_t)0x152);

    /*
     * Open MPI accepts Startall(0, non-NULL) and rejects Startall(0, NULL)
     * with MPI_ERR_REQUEST (class 7 in the fake).
     */
    requests[0] = (MPI_Request)(intptr_t)0x51;
    assert(unimpi.startall(0, requests) == 0);
    assert(unimpi.startall(0, NULL) == 7);

    unimpi_loader_unload(handle);
    printf("  Open MPI array binder paths passed (7 entry points)\n");
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
