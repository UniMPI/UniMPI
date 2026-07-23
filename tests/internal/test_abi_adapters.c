/* Unit tests for backend array ABI conversion without a real MPI runtime. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "datatype_array_wrappers.h"
#include "request_array_wrappers.h"

static int fake_legacy_waitall(
    int count, int *requests, struct unimpi_status_legacy *statuses) {
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

static int fake_openmpi_waitall(
    int count, MPI_Request *requests,
    unimpi_openmpi_native_status_t *statuses) {
    int i;

    assert(count == 2);
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i].MPI_SOURCE = 30 + i;
        statuses[i].MPI_TAG = 40 + i;
        statuses[i].MPI_ERROR = 0;
        statuses[i].count = (size_t)(i + 3);
    }
    return 0;
}

static int fake_legacy_some(
    int count, int *requests, int *outcount, int *indices,
    struct unimpi_status_legacy *statuses) {
    assert(count == 2);
    requests[1] = 0;
    *outcount = 1;
    indices[0] = 1;
    statuses[0].count_lo = 7;
    statuses[0].MPI_SOURCE = 51;
    return 0;
}

static int fake_openmpi_some(
    int count, MPI_Request *requests, int *outcount, int *indices,
    unimpi_openmpi_native_status_t *statuses) {
    assert(count == 2);
    requests[1] = 0;
    *outcount = 1;
    indices[0] = 1;
    statuses[0].count = 8;
    statuses[0].MPI_SOURCE = 61;
    return 0;
}

static int fake_legacy_testall(
    int count, int *requests, int *flag,
    struct unimpi_status_legacy *statuses) {
    int i;

    assert(count == 2);
    *flag = 1;
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i].count_lo = 9 + i;
    }
    return 0;
}

static int fake_openmpi_testall(
    int count, MPI_Request *requests, int *flag,
    unimpi_openmpi_native_status_t *statuses) {
    int i;

    assert(count == 2);
    *flag = 1;
    for (i = 0; i < count; ++i) {
        requests[i] = 0;
        statuses[i].count = (size_t)(11 + i);
    }
    return 0;
}

static int fake_peer_count = 2;
static int fake_is_inter;
static int fake_comm_size(int comm, int *size) {
    assert(comm == 9);
    *size = fake_peer_count;
    return 0;
}

static int fake_comm_test_inter(int comm, int *flag) {
    assert(comm == 9);
    *flag = fake_is_inter;
    return 0;
}

static int fake_comm_remote_size(int comm, int *size) {
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

static int fake_alltoallw(
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

static int fake_ialltoallw(
    const void *sendbuf, const int *sendcounts, const int *sdispls,
    const int *sendtypes, void *recvbuf, const int *recvcounts,
    const int *rdispls, const int *recvtypes, int comm, int *request) {
    int result = fake_alltoallw(
        sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts,
        rdispls, recvtypes, comm);
    *request = 0x12345678;
    return result;
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

static void test_datatype_arrays(void) {
    int counts[3] = {1, 1, 1};
    int displs[3] = {0, 4, 8};
    MPI_Datatype sendtypes[3] = {100, 101, 102};
    MPI_Datatype recvtypes[3] = {200, 201, 202};
    MPI_Request request = 0;

    unimpi_datatype_array_adapter_init(
        fake_comm_size, fake_comm_test_inter, fake_comm_remote_size,
        NULL, NULL);
    assert(!unimpi_datatype_array_has_alltoallw());
    assert(!unimpi_datatype_array_has_ialltoallw());

    unimpi_datatype_array_adapter_init(
        fake_comm_size, fake_comm_test_inter, fake_comm_remote_size,
        fake_alltoallw, fake_ialltoallw);
    assert(unimpi_datatype_array_has_alltoallw());
    assert(unimpi_datatype_array_has_ialltoallw());

    fake_is_inter = 0;
    fake_peer_count = 2;
    assert(unimpi_wrap_alltoallw(
               NULL, counts, displs, sendtypes, NULL, counts, displs,
               recvtypes, (MPI_Comm)9) == 0);

    fake_is_inter = 1;
    fake_peer_count = 3;
    assert(unimpi_wrap_ialltoallw(
               NULL, counts, displs, sendtypes, NULL, counts, displs,
               recvtypes, (MPI_Comm)9, &request) == 0);
    assert(request == (MPI_Request)(intptr_t)0x12345678);
}

int main(void) {
    test_status_array_strides();
    test_datatype_arrays();
    puts("ABI adapter unit tests passed");
    return 0;
}
