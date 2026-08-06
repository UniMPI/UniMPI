/* Regression: integer-handle backends bind full-width facade request/message
 * handles through the production binder against real native int ABI signatures.
 *
 * Paths exercised after each real MPICH / Intel / MS-MPI initializer against
 * a dedicated fake integer-handle DSO:
 *   Irecv -> Testall(false) -> Wait
 *   Irecv -> Wait(STATUS_IGNORE)
 *   Send_init -> Startall -> Request_free
 *   Testall ERR_IN_STATUS status copyback
 *   Ibarrier / Rput / File_iread producers (valid buffers)
 *   Mprobe -> Imrecv -> Wait
 *   Mprobe -> Mrecv
 *   Improbe flag=false preserves caller message
 *   Failed Wait/Mprobe/Improbe/Mrecv leave caller status/message intact
 *
 * A separate missing-symbol fixture verifies optional slots stay NULL.
 *
 * Usage:
 *   test_request_handle_width <integer_api_fake> <missing_symbol_fake>
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_errors.h"
#include "unimpi_loader.h"
#include "unimpi_platform.h"
#include "unimpi_vtable.h"

enum {
    FAKE_NATIVE_REQUEST = (int)0xac000001,
    FAKE_NATIVE_MESSAGE = (int)0xad000002,
    FAKE_NATIVE_NULL = (int)0x2c000000,
    FAKE_ERR_IN_STATUS_CODE = 0x10000 | 17,
    FAKE_ERR_OTHER = 15
};

int unimpi_vtable_init_mpich(unimpi_lib_handle_t handle);
int unimpi_vtable_init_intelmpi(unimpi_lib_handle_t handle);
int unimpi_vtable_init_msmpi(unimpi_lib_handle_t handle);

typedef int (UNIMPI_MPI_CALL *fake_set_fail_fn)(int);

static unimpi_lib_handle_t g_active_handle;

static MPI_Request facade_request(int native) {
    return (MPI_Request)(intptr_t)native;
}

static MPI_Message facade_message(int native) {
    return (MPI_Message)(intptr_t)native;
}

static void set_fail(const char *symbol, int enable) {
    fake_set_fail_fn fn;

    assert(g_active_handle != NULL);
    fn = (fake_set_fail_fn)unimpi_platform_dlsym(g_active_handle, symbol);
    assert(fn != NULL);
    assert(fn(enable) == 0);
}

static void assert_present_request_slots(const char *backend_name) {
    assert(unimpi.irecv != NULL);
    assert(unimpi.wait != NULL);
    assert(unimpi.testall != NULL);
    assert(unimpi.send_init != NULL);
    assert(unimpi.startall != NULL);
    assert(unimpi.request_free != NULL);
    assert(unimpi.ibarrier != NULL);
    assert(unimpi.rput != NULL);
    assert(unimpi.file_iread != NULL);
    assert(unimpi.mprobe != NULL);
    assert(unimpi.improbe != NULL);
    assert(unimpi.mrecv != NULL);
    assert(unimpi.imrecv != NULL);
    assert(unimpi.ialltoallw == NULL);
    printf("  %s binder installed expected request/message slots\n",
           backend_name);
}

static void assert_missing_request_slots_null(const char *backend_name) {
    assert(unimpi.irecv == NULL);
    assert(unimpi.isend == NULL);
    assert(unimpi.wait == NULL);
    assert(unimpi.test == NULL);
    assert(unimpi.send_init == NULL);
    assert(unimpi.start == NULL);
    assert(unimpi.request_free == NULL);
    assert(unimpi.waitall == NULL);
    assert(unimpi.testall == NULL);
    assert(unimpi.startall == NULL);
    assert(unimpi.ibarrier == NULL);
    assert(unimpi.rput == NULL);
    assert(unimpi.file_iread == NULL);
    assert(unimpi.mprobe == NULL);
    assert(unimpi.improbe == NULL);
    assert(unimpi.mrecv == NULL);
    assert(unimpi.imrecv == NULL);
    assert(unimpi.ialltoallw == NULL);
    printf("  %s missing optional request/message symbols remain NULL\n",
           backend_name);
}

static void test_irecv_testall_wait(void) {
    MPI_Request request = 0;
    MPI_Status status;
    int flag = 1;
    char buffer[4] = {0};

    memset(&status, 0xff, sizeof(status));
    assert(unimpi.irecv(
               buffer, 1, (MPI_Datatype)1, 0, 0, (MPI_Comm)1,
               &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));

    flag = 1;
    assert(unimpi.testall(1, &request, &flag, &status) == 0);
    assert(flag == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));

    assert(unimpi.wait(&request, &status) == 0);
    assert(request == UNIMPI_REQUEST_NULL);
    assert(request == facade_request(FAKE_NATIVE_NULL));
    assert(status.legacy.MPI_SOURCE == 3);
    assert(status.legacy.MPI_TAG == 7);
    printf("    Irecv/Testall/Wait path passed\n");
}

/* Single-status ignore sentinel: Wait must not write through the sentinel. */
static void test_wait_status_ignore(void) {
    MPI_Request request = 0;
    char buffer[4] = {0};

    assert(UNIMPI_STATUS_IGNORE == UNIMPI_STATUSES_IGNORE);
    assert(unimpi.irecv(
               buffer, 1, (MPI_Datatype)1, 0, 0, (MPI_Comm)1,
               &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));
    assert(unimpi.wait(&request, UNIMPI_STATUS_IGNORE) == 0);
    assert(request == UNIMPI_REQUEST_NULL);
    assert(request == facade_request(FAKE_NATIVE_NULL));
    printf("    Wait(STATUS_IGNORE) path passed\n");
}

static void test_send_init_startall_free(void) {
    MPI_Request request = 0;
    char buffer[4] = {0};

    assert(unimpi.send_init(
               buffer, 1, (MPI_Datatype)1, 0, 0, (MPI_Comm)1,
               &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));
    assert(unimpi.startall(1, &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));
    assert(unimpi.request_free(&request) == 0);
    assert(request == UNIMPI_REQUEST_NULL);
    assert(request == facade_request(FAKE_NATIVE_NULL));
    printf("    Send_init/Startall/Request_free path passed\n");
}

static void test_err_in_status_copyback(void) {
    MPI_Request requests[2];
    MPI_Status statuses[2];
    int flag = 1;

    requests[0] = facade_request(FAKE_NATIVE_REQUEST);
    requests[1] = facade_request(FAKE_NATIVE_REQUEST);
    memset(statuses, 0xff, sizeof(statuses));

    assert(unimpi.testall(2, requests, &flag, statuses) ==
           FAKE_ERR_IN_STATUS_CODE);
    assert(flag == 0);
    assert(requests[0] == facade_request(FAKE_NATIVE_NULL));
    assert(requests[1] == facade_request(FAKE_NATIVE_NULL));
    assert(statuses[0].legacy.count_lo == 40);
    assert(statuses[1].legacy.count_lo == 41);
    assert(statuses[0].legacy.MPI_SOURCE == 10);
    assert(statuses[1].legacy.MPI_SOURCE == 11);
    assert(statuses[0].legacy.MPI_ERROR == 17);
    assert(statuses[1].legacy.MPI_ERROR == 17);
    assert(MPI_ERR_IN_STATUS == 17);
    printf("    post-init Testall ERR_IN_STATUS copyback passed\n");
}

static void test_representative_producers(void) {
    MPI_Request request = 0;
    int rput_origin = 42;
    char file_buf[8] = {0};

    assert(unimpi.ibarrier((MPI_Comm)1, &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));

    request = 0;
    assert(unimpi.rput(
               &rput_origin, 1, (MPI_Datatype)1, 0, (MPI_Aint)0, 1,
               (MPI_Datatype)1, (MPI_Win)1, &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));

    request = 0;
    assert(unimpi.file_iread(
               (MPI_File)1, file_buf, 1, (MPI_Datatype)1, &request) == 0);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));
    printf("    Ibarrier/Rput/File_iread producers passed\n");
}

static void test_mprobe_imrecv_wait(void) {
    MPI_Message message = 0;
    MPI_Request request = 0;
    MPI_Status status;
    char buffer[4] = {0};

    memset(&status, 0xff, sizeof(status));
    assert(unimpi.mprobe(0, 1, (MPI_Comm)1, &message, &status) == 0);
    assert(message == facade_message(FAKE_NATIVE_MESSAGE));

    assert(unimpi.imrecv(
               buffer, 1, (MPI_Datatype)1, &message, &request) == 0);
    assert(message == facade_message(FAKE_NATIVE_NULL));
    assert(request == facade_request(FAKE_NATIVE_REQUEST));

    assert(unimpi.wait(&request, &status) == 0);
    assert(request == UNIMPI_REQUEST_NULL);
    printf("    Mprobe/Imrecv/Wait path passed\n");
}

static void test_mprobe_mrecv(void) {
    MPI_Message message = 0;
    MPI_Status status;
    char buffer[4] = {0};

    memset(&status, 0xff, sizeof(status));
    assert(unimpi.mprobe(1, 2, (MPI_Comm)1, &message, &status) == 0);
    assert(message == facade_message(FAKE_NATIVE_MESSAGE));
    assert(unimpi.mrecv(
               buffer, 4, (MPI_Datatype)1, &message, &status) == 0);
    assert(message == facade_message(FAKE_NATIVE_NULL));
    assert(status.legacy.MPI_SOURCE == 1);
    assert(status.legacy.MPI_TAG == 2);
    printf("    Mprobe/Mrecv path passed\n");
}

static void test_improbe_false_preserves_message(void) {
    MPI_Message message = facade_message(0xbeef0001);
    MPI_Message preserved = message;
    MPI_Status status;
    int flag = 1;

    memset(&status, 0xff, sizeof(status));
    assert(unimpi.improbe(0, 0, (MPI_Comm)1, &flag, &message, &status) == 0);
    assert(flag == 0);
    assert(message == preserved);
    printf("    Improbe flag=false preserves message passed\n");
}

static void test_failure_does_not_corrupt_outputs(void) {
    MPI_Request request;
    MPI_Message message;
    MPI_Message preserved_message;
    MPI_Status status;
    MPI_Status preserved_status;
    int flag;
    char buffer[4] = {0};

    /* Failed Wait: facade request and status stay at caller values. */
    request = facade_request(FAKE_NATIVE_REQUEST);
    memset(&status, 0xa5, sizeof(status));
    preserved_status = status;
    set_fail("unimpi_fake_set_fail_next_wait", 1);
    assert(unimpi.wait(&request, &status) == FAKE_ERR_OTHER);
    assert(request == facade_request(FAKE_NATIVE_REQUEST));
    assert(memcmp(&status, &preserved_status, sizeof(status)) == 0);

    /* Failed Mprobe: message and status preserved. */
    message = facade_message(0x11110001);
    preserved_message = message;
    memset(&status, 0xb6, sizeof(status));
    preserved_status = status;
    set_fail("unimpi_fake_set_fail_next_mprobe", 1);
    assert(unimpi.mprobe(0, 0, (MPI_Comm)1, &message, &status) ==
           FAKE_ERR_OTHER);
    assert(message == preserved_message);
    assert(memcmp(&status, &preserved_status, sizeof(status)) == 0);

    /* Failed Improbe even with flag=true from native: no store. */
    message = facade_message(0x22220002);
    preserved_message = message;
    memset(&status, 0xc7, sizeof(status));
    preserved_status = status;
    flag = 0;
    set_fail("unimpi_fake_set_fail_next_improbe", 1);
    assert(unimpi.improbe(0, 0, (MPI_Comm)1, &flag, &message, &status) ==
           FAKE_ERR_OTHER);
    assert(message == preserved_message);
    assert(memcmp(&status, &preserved_status, sizeof(status)) == 0);

    /* Failed Mrecv after a successful Mprobe: message/status preserved. */
    assert(unimpi.mprobe(3, 4, (MPI_Comm)1, &message, &status) == 0);
    assert(message == facade_message(FAKE_NATIVE_MESSAGE));
    preserved_message = message;
    memset(&status, 0xd8, sizeof(status));
    preserved_status = status;
    set_fail("unimpi_fake_set_fail_next_mrecv", 1);
    assert(unimpi.mrecv(
               buffer, 1, (MPI_Datatype)1, &message, &status) ==
           FAKE_ERR_OTHER);
    assert(message == preserved_message);
    assert(memcmp(&status, &preserved_status, sizeof(status)) == 0);

    printf("    failure paths preserve caller status/message passed\n");
}

static void test_backend_on_integer_api(
    const char *path,
    int (*init_fn)(unimpi_lib_handle_t),
    const char *backend_name) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    g_active_handle = handle;
    assert(init_fn(handle) == UNIMPI_OK);
    assert_present_request_slots(backend_name);

    assert(MPI_ERR_IN_STATUS == 17);
    assert(MPI_ERR_PENDING == 18);
    assert(MPI_ERR_REQUEST == 19);
    assert(MPI_ERR_NO_MEM == 34);
    assert(UNIMPI_REQUEST_NULL == facade_request(FAKE_NATIVE_NULL));

    printf("  %s production binder path checks...\n", backend_name);
    test_irecv_testall_wait();
    test_wait_status_ignore();
    test_send_init_startall_free();
    test_err_in_status_copyback();
    test_representative_producers();
    test_mprobe_imrecv_wait();
    test_mprobe_mrecv();
    test_improbe_false_preserves_message();
    test_failure_does_not_corrupt_outputs();

    g_active_handle = NULL;
    unimpi_loader_unload(handle);
}

static void test_missing_symbols(
    const char *path,
    int (*init_fn)(unimpi_lib_handle_t),
    const char *backend_name) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    assert(init_fn(handle) == UNIMPI_OK);
    assert_missing_request_slots_null(backend_name);
    unimpi_loader_unload(handle);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <integer_api_fake> <missing_symbol_fake>\n",
                argv[0]);
        return 2;
    }

    printf("Running request/message handle width regressions...\n");

    test_backend_on_integer_api(argv[1], unimpi_vtable_init_mpich, "MPICH");
    test_backend_on_integer_api(
        argv[1], unimpi_vtable_init_intelmpi, "Intel MPI");
    test_backend_on_integer_api(argv[1], unimpi_vtable_init_msmpi, "MS-MPI");

    printf("  Missing-symbol fixture checks...\n");
    test_missing_symbols(argv[2], unimpi_vtable_init_mpich, "MPICH");
    test_missing_symbols(argv[2], unimpi_vtable_init_intelmpi, "Intel MPI");
    test_missing_symbols(argv[2], unimpi_vtable_init_msmpi, "MS-MPI");

    printf("Request/message handle width regressions passed\n");
    return 0;
}
