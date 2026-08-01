/* Unit tests for backend array ABI conversion without a real MPI runtime. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "datatype_array_wrappers.h"
#include "request_array_wrappers.h"
#include "unimpi_errors.h"
#include "unimpi_std_macros.h"

/* Complete vendor native status tag for in-process fakes registered with the
 * integer-backend completion adapters (must match five-int stride). */
struct MPI_Status {
    int count_lo;
    int count_hi_and_cancelled;
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
};

enum {
    FAKE_ERR_ARG_CLASS = 12,
    FAKE_ERR_IN_STATUS_CLASS = 17,
    FAKE_ENCODED_ERR_ARG = 0x10000 | FAKE_ERR_ARG_CLASS,
    FAKE_ENCODED_ERR_IN_STATUS =
        0x10000 | FAKE_ERR_IN_STATUS_CLASS
};

static int UNIMPI_MPI_CALL fake_error_class(
    int error_code, int *error_class) {
    assert(error_class);
    *error_class = error_code & 0xff;
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_waitall(
    int count, int *requests, struct MPI_Status *statuses) {
    int i;

    assert(count == 2);
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i].count_lo = i + 1;
        statuses[i].MPI_SOURCE = 10 + i;
        statuses[i].MPI_TAG = 20 + i;
        statuses[i].MPI_ERROR = 0;
    }
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_waitall(
    int count, struct ompi_request_t **requests,
    struct ompi_status_public_t *statuses) {
    int i;

    assert(count == 2);
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i].MPI_SOURCE = 30 + i;
        statuses[i].MPI_TAG = 40 + i;
        statuses[i].MPI_ERROR = 0;
        statuses[i]._ucount = (size_t)(i + 3);
    }
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_some(
    int count, int *requests, int *outcount, int *indices,
    struct MPI_Status *statuses) {
    assert(count == 2);
    requests[1] = 0;
    *outcount = 1;
    indices[0] = 1;
    statuses[0].count_lo = 7;
    statuses[0].MPI_SOURCE = 51;
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_some(
    int count, struct ompi_request_t **requests, int *outcount, int *indices,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    requests[1] = 0;
    *outcount = 1;
    indices[0] = 1;
    statuses[0]._ucount = 8;
    statuses[0].MPI_SOURCE = 61;
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_testall(
    int count, int *requests, int *flag,
    struct MPI_Status *statuses) {
    int i;

    assert(count == 2);
    *flag = 1;
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i].count_lo = 9 + i;
    }
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_testall(
    int count, struct ompi_request_t **requests, int *flag,
    struct ompi_status_public_t *statuses) {
    int i;

    assert(count == 2);
    *flag = 1;
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i]._ucount = (size_t)(11 + i);
    }
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_waitall_ignore(
    int count, int *requests, struct MPI_Status *statuses) {
    assert(count == 2);
    assert(statuses ==
           (struct MPI_Status *)(intptr_t)1);
    requests[0] = 0;
    requests[1] = 0;
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_some_ignore(
    int count, int *requests, int *outcount, int *indices,
    struct MPI_Status *statuses) {
    assert(count == 2);
    assert(statuses ==
           (struct MPI_Status *)(intptr_t)1);
    requests[0] = 0;
    *outcount = 1;
    indices[0] = 0;
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_testall_ignore(
    int count, int *requests, int *flag,
    struct MPI_Status *statuses) {
    assert(count == 2);
    assert(statuses ==
           (struct MPI_Status *)(intptr_t)1);
    requests[0] = 0;
    requests[1] = 0;
    *flag = 1;
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_waitall_ignore(
    int count, struct ompi_request_t **requests,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    assert(statuses == NULL);
    requests[0] = 0;
    requests[1] = 0;
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_some_ignore(
    int count, struct ompi_request_t **requests, int *outcount, int *indices,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    assert(statuses == NULL);
    requests[0] = 0;
    *outcount = 1;
    indices[0] = 0;
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_testall_ignore(
    int count, struct ompi_request_t **requests, int *flag,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    assert(statuses == NULL);
    requests[0] = 0;
    requests[1] = 0;
    *flag = 1;
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_waitall_zero(
    int count, int *requests, struct MPI_Status *statuses) {
    assert(count == 0);
    assert(requests == NULL);
    assert(statuses ==
           (struct MPI_Status *)UNIMPI_STATUSES_IGNORE);
    return 0;
}

static int UNIMPI_MPI_CALL fake_openmpi_waitall_zero(
    int count, struct ompi_request_t **requests,
    struct ompi_status_public_t *statuses) {
    assert(count == 0);
    assert(requests == NULL);
    assert(statuses == NULL);
    return 0;
}

static int UNIMPI_MPI_CALL fake_legacy_waitall_error(
    int count, int *requests, struct MPI_Status *statuses) {
    assert(count == 2);
    assert(requests);
    assert(statuses);
    return FAKE_ENCODED_ERR_ARG;
}

static int UNIMPI_MPI_CALL fake_legacy_some_error(
    int count, int *requests, int *outcount, int *indices,
    struct MPI_Status *statuses) {
    assert(count == 2);
    assert(requests);
    assert(outcount);
    assert(indices);
    assert(statuses);
    return FAKE_ENCODED_ERR_ARG;
}

static int UNIMPI_MPI_CALL fake_legacy_testall_error(
    int count, int *requests, int *flag,
    struct MPI_Status *statuses) {
    assert(count == 2);
    assert(requests);
    assert(flag);
    assert(statuses);
    return FAKE_ENCODED_ERR_ARG;
}

static int UNIMPI_MPI_CALL fake_openmpi_waitall_error(
    int count, struct ompi_request_t **requests,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    assert(requests);
    assert(statuses);
    return FAKE_ENCODED_ERR_ARG;
}

static int UNIMPI_MPI_CALL fake_openmpi_some_error(
    int count, struct ompi_request_t **requests, int *outcount, int *indices,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    assert(requests);
    assert(outcount);
    assert(indices);
    assert(statuses);
    return FAKE_ENCODED_ERR_ARG;
}

static int UNIMPI_MPI_CALL fake_openmpi_testall_error(
    int count, struct ompi_request_t **requests, int *flag,
    struct ompi_status_public_t *statuses) {
    assert(count == 2);
    assert(requests);
    assert(flag);
    assert(statuses);
    return FAKE_ENCODED_ERR_ARG;
}

static int UNIMPI_MPI_CALL fake_legacy_testall_in_status(
    int count, int *requests, int *flag,
    struct MPI_Status *statuses) {
    int i;

    assert(count == 2);
    assert(requests);
    assert(flag);
    assert(statuses);
    for (i = 0; i < count; ++i) {
        statuses[i].MPI_ERROR = FAKE_ERR_IN_STATUS_CLASS;
        statuses[i].count_lo = 70 + i;
    }
    return FAKE_ENCODED_ERR_IN_STATUS;
}

static int UNIMPI_MPI_CALL fake_openmpi_testall_in_status(
    int count, struct ompi_request_t **requests, int *flag,
    struct ompi_status_public_t *statuses) {
    int i;

    assert(count == 2);
    assert(requests);
    assert(flag);
    assert(statuses);
    for (i = 0; i < count; ++i) {
        statuses[i].MPI_ERROR = FAKE_ERR_IN_STATUS_CLASS;
        statuses[i]._ucount = (size_t)(80 + i);
    }
    return FAKE_ENCODED_ERR_IN_STATUS;
}

static int fake_peer_count = 2;
static int fake_is_inter;
static int UNIMPI_MPI_CALL fake_comm_size(int comm, int *size) {
    assert(comm == 9);
    *size = fake_peer_count;
    return 0;
}

static int UNIMPI_MPI_CALL fake_comm_test_inter(int comm, int *flag) {
    assert(comm == 9);
    *flag = fake_is_inter;
    return 0;
}

static int UNIMPI_MPI_CALL fake_comm_remote_size(int comm, int *size) {
    assert(comm == 9);
    assert(fake_is_inter);
    *size = fake_peer_count;
    return 0;
}

static void check_native_types(
    const int *sendtypes, const int *recvtypes) {
    int i;

    for (i = 0; i < fake_peer_count; ++i) {
        assert(sendtypes[i] == 100 + i);
        assert(recvtypes[i] == 200 + i);
    }
}

static int UNIMPI_MPI_CALL fake_alltoallw(
    const void *sendbuf, const int *sendcounts, const int *sdispls,
    const int *sendtypes, void *recvbuf, const int *recvcounts,
    const int *rdispls, const int *recvtypes, int comm) {
    (void)sendbuf;
    (void)sendcounts;
    (void)sdispls;
    (void)recvbuf;
    (void)recvcounts;
    (void)rdispls;
    assert(comm == 9);
    check_native_types(sendtypes, recvtypes);
    return 0;
}

static void test_status_array_strides(void) {
    MPI_Request legacy_requests[2] = {7, 8};
    MPI_Request openmpi_requests[2] = {9, 10};
    MPI_Status statuses[2];
    int indices[2] = {-1, -1};
    int outcount = 0;
    int flag = 0;

    unimpi_wrapper_set_waitall(fake_legacy_waitall);
    assert(unimpi_wrap_waitall(2, legacy_requests, statuses) == 0);
    assert(legacy_requests[0] == 0 && legacy_requests[1] == 0);
    assert(statuses[0].legacy.count_lo == 1);
    assert(statuses[1].legacy.count_lo == 2);
    assert(statuses[0].legacy.MPI_SOURCE == 10);
    assert(statuses[1].legacy.MPI_SOURCE == 11);

    unimpi_wrapper_set_openmpi_waitall(fake_openmpi_waitall);
    assert(unimpi_wrap_openmpi_waitall(
               2, openmpi_requests, statuses) == 0);
    assert(statuses[0].openmpi._ucount == 3);
    assert(statuses[1].openmpi._ucount == 4);
    assert(statuses[0].openmpi.MPI_SOURCE == 30);
    assert(statuses[1].openmpi.MPI_SOURCE == 31);

    legacy_requests[0] = 7;
    legacy_requests[1] = 8;
    unimpi_wrapper_set_testsome(fake_legacy_some);
    assert(unimpi_wrap_testsome(
               2, legacy_requests, &outcount, indices, statuses) == 0);
    assert(outcount == 1 && indices[0] == 1);
    assert(statuses[0].legacy.count_lo == 7);
    unimpi_wrapper_set_waitsome(fake_legacy_some);
    assert(unimpi_wrap_waitsome(
               2, legacy_requests, &outcount, indices, statuses) == 0);

    openmpi_requests[0] = 9;
    openmpi_requests[1] = 10;
    unimpi_wrapper_set_openmpi_testsome(fake_openmpi_some);
    assert(unimpi_wrap_openmpi_testsome(
               2, openmpi_requests, &outcount, indices, statuses) == 0);
    assert(statuses[0].openmpi._ucount == 8);
    unimpi_wrapper_set_openmpi_waitsome(fake_openmpi_some);
    assert(unimpi_wrap_openmpi_waitsome(
               2, openmpi_requests, &outcount, indices, statuses) == 0);

    legacy_requests[0] = 7;
    legacy_requests[1] = 8;
    unimpi_wrapper_set_testall(fake_legacy_testall);
    assert(unimpi_wrap_testall(
               2, legacy_requests, &flag, statuses) == 0);
    assert(flag == 1 && statuses[1].legacy.count_lo == 10);

    flag = 0;
    openmpi_requests[0] = 9;
    openmpi_requests[1] = 10;
    unimpi_wrapper_set_openmpi_testall(fake_openmpi_testall);
    assert(unimpi_wrap_openmpi_testall(
               2, openmpi_requests, &flag, statuses) == 0);
    assert(flag == 1 && statuses[1].openmpi._ucount == 12);
}

static void test_statuses_ignore_translation(void) {
    MPI_Request legacy_requests[2] = {7, 8};
    MPI_Request openmpi_requests[2] = {9, 10};
    MPI_Status *statuses_ignore = UNIMPI_STATUSES_IGNORE;
    int indices[2] = {-1, -1};
    int outcount = 0;
    int flag = 0;

    assert(MPI_STATUSES_IGNORE == UNIMPI_STATUSES_IGNORE);

    unimpi_wrapper_set_waitall(fake_legacy_waitall_ignore);
    assert(unimpi_wrap_waitall(
               2, legacy_requests, statuses_ignore) == 0);

    legacy_requests[0] = 7;
    legacy_requests[1] = 8;
    unimpi_wrapper_set_testsome(fake_legacy_some_ignore);
    assert(unimpi_wrap_testsome(
               2, legacy_requests, &outcount, indices,
               statuses_ignore) == 0);
    legacy_requests[0] = 7;
    legacy_requests[1] = 8;
    unimpi_wrapper_set_waitsome(fake_legacy_some_ignore);
    assert(unimpi_wrap_waitsome(
               2, legacy_requests, &outcount, indices,
               statuses_ignore) == 0);
    legacy_requests[0] = 7;
    legacy_requests[1] = 8;
    unimpi_wrapper_set_testall(fake_legacy_testall_ignore);
    assert(unimpi_wrap_testall(
               2, legacy_requests, &flag, statuses_ignore) == 0);

    legacy_requests[0] = 7;
    legacy_requests[1] = 8;
    unimpi_wrapper_set_waitall(fake_legacy_waitall_ignore);
    assert(unimpi_wrap_waitall(2, legacy_requests, NULL) == 0);

    unimpi_wrapper_set_openmpi_waitall(fake_openmpi_waitall_ignore);
    assert(unimpi_wrap_openmpi_waitall(
               2, openmpi_requests, statuses_ignore) == 0);
    openmpi_requests[0] = 9;
    openmpi_requests[1] = 10;
    unimpi_wrapper_set_openmpi_testsome(fake_openmpi_some_ignore);
    assert(unimpi_wrap_openmpi_testsome(
               2, openmpi_requests, &outcount, indices,
               statuses_ignore) == 0);
    openmpi_requests[0] = 9;
    openmpi_requests[1] = 10;
    unimpi_wrapper_set_openmpi_waitsome(fake_openmpi_some_ignore);
    assert(unimpi_wrap_openmpi_waitsome(
               2, openmpi_requests, &outcount, indices,
               statuses_ignore) == 0);
    openmpi_requests[0] = 9;
    openmpi_requests[1] = 10;
    flag = 0;
    unimpi_wrapper_set_openmpi_testall(fake_openmpi_testall_ignore);
    assert(unimpi_wrap_openmpi_testall(
               2, openmpi_requests, &flag, statuses_ignore) == 0);

    unimpi_wrapper_set_waitall(fake_legacy_waitall_zero);
    assert(unimpi_wrap_waitall(
               0, NULL, UNIMPI_STATUSES_IGNORE) == 0);
    unimpi_wrapper_set_openmpi_waitall(fake_openmpi_waitall_zero);
    assert(unimpi_wrap_openmpi_waitall(
               0, NULL, UNIMPI_STATUSES_IGNORE) == 0);
}

static void test_error_output_validity(void) {
    MPI_Request legacy_requests[2] = {7, 8};
    MPI_Request openmpi_requests[2] = {9, 10};
    MPI_Status statuses[2];
    MPI_Status expected[2];
    int indices[2] = {-1, -1};
    int outcount = 123;
    int flag = 456;

    memset(statuses, 0x5a, sizeof(statuses));
    memcpy(expected, statuses, sizeof(expected));

    unimpi_wrapper_set_waitall(fake_legacy_waitall_error);
    assert(unimpi_wrap_waitall(
               2, legacy_requests, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);

    unimpi_wrapper_set_testsome(fake_legacy_some_error);
    assert(unimpi_wrap_testsome(
               2, legacy_requests, &outcount, indices, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);
    assert(outcount == 123);

    unimpi_wrapper_set_waitsome(fake_legacy_some_error);
    assert(unimpi_wrap_waitsome(
               2, legacy_requests, &outcount, indices, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);
    assert(outcount == 123);

    unimpi_wrapper_set_testall(fake_legacy_testall_error);
    assert(unimpi_wrap_testall(
               2, legacy_requests, &flag, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);
    assert(flag == 456);

    unimpi_wrapper_set_openmpi_waitall(fake_openmpi_waitall_error);
    assert(unimpi_wrap_openmpi_waitall(
               2, openmpi_requests, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);

    unimpi_wrapper_set_openmpi_testsome(fake_openmpi_some_error);
    assert(unimpi_wrap_openmpi_testsome(
               2, openmpi_requests, &outcount, indices, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);
    assert(outcount == 123);

    unimpi_wrapper_set_openmpi_waitsome(fake_openmpi_some_error);
    assert(unimpi_wrap_openmpi_waitsome(
               2, openmpi_requests, &outcount, indices, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);
    assert(outcount == 123);

    unimpi_wrapper_set_openmpi_testall(fake_openmpi_testall_error);
    assert(unimpi_wrap_openmpi_testall(
               2, openmpi_requests, &flag, statuses) ==
           FAKE_ENCODED_ERR_ARG);
    assert(memcmp(statuses, expected, sizeof(statuses)) == 0);
    assert(flag == 456);
    assert(legacy_requests[0] == 7 && legacy_requests[1] == 8);
    assert(openmpi_requests[0] == 9 && openmpi_requests[1] == 10);
    assert(indices[0] == -1 && indices[1] == -1);

    flag = 0;
    memset(statuses, 0, sizeof(statuses));
    unimpi_wrapper_set_testall(fake_legacy_testall_in_status);
    assert(unimpi_wrap_testall(
               2, legacy_requests, &flag, statuses) ==
           FAKE_ENCODED_ERR_IN_STATUS);
    assert(flag == 0);
    assert(statuses[0].legacy.count_lo == 70);
    assert(statuses[1].legacy.count_lo == 71);
    assert(statuses[0].legacy.MPI_ERROR == FAKE_ERR_IN_STATUS_CLASS);
    assert(statuses[1].legacy.MPI_ERROR == FAKE_ERR_IN_STATUS_CLASS);

    flag = 0;
    memset(statuses, 0, sizeof(statuses));
    unimpi_wrapper_set_openmpi_testall(fake_openmpi_testall_in_status);
    assert(unimpi_wrap_openmpi_testall(
               2, openmpi_requests, &flag, statuses) ==
           FAKE_ENCODED_ERR_IN_STATUS);
    assert(flag == 0);
    assert(statuses[0].openmpi._ucount == 80);
    assert(statuses[1].openmpi._ucount == 81);
    assert(statuses[0].openmpi.MPI_ERROR == FAKE_ERR_IN_STATUS_CLASS);
    assert(statuses[1].openmpi.MPI_ERROR == FAKE_ERR_IN_STATUS_CLASS);
}

static void test_datatype_arrays(void) {
    int counts[3] = {1, 1, 1};
    int displs[3] = {0, 4, 8};
    MPI_Datatype sendtypes[3] = {100, 101, 102};
    MPI_Datatype recvtypes[3] = {200, 201, 202};

    unimpi_datatype_array_adapter_init(
        fake_comm_size, fake_comm_test_inter, fake_comm_remote_size,
        NULL);
    assert(!unimpi_datatype_array_has_alltoallw());

    unimpi_datatype_array_adapter_init(
        fake_comm_size, fake_comm_test_inter, fake_comm_remote_size,
        fake_alltoallw);
    assert(unimpi_datatype_array_has_alltoallw());

    fake_is_inter = 0;
    fake_peer_count = 2;
    assert(unimpi_wrap_alltoallw(
               NULL, counts, displs, sendtypes, NULL, counts, displs,
               recvtypes, (MPI_Comm)9) == 0);

    fake_is_inter = 1;
    fake_peer_count = 3;
    assert(unimpi_wrap_alltoallw(
               NULL, counts, displs, sendtypes, NULL, counts, displs,
               recvtypes, (MPI_Comm)9) == 0);
}

int main(void) {
    MPI_SUCCESS = 0;
    MPI_ERR_IN_STATUS = FAKE_ERR_IN_STATUS_CLASS;
    MPI_ERR_REQUEST = 19;
    MPI_ERR_NO_MEM = 34;
    unimpi_wrapper_set_error_class(fake_error_class);

    test_status_array_strides();
    test_statuses_ignore_translation();
    test_error_output_validity();
    test_datatype_arrays();
    puts("ABI adapter unit tests passed");
    return 0;
}
