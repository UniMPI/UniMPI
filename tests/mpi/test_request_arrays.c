/* test_request_arrays.c - Request-array ABI and completion semantics */
#define UNIMPI_USE_STD_NAMES
#include <assert.h>
#include <stdio.h>

#include "unimpi.h"
#include "test_mpi_helpers.h"

#define TAG_BASE 700
#define REQUEST_COUNT 2

static void world_barrier(void) {
    TEST_CHECK_SUCCESS(MPI_Barrier(MPI_COMM_WORLD));
}

static void test_zero_count(void) {
    MPI_Request request = MPI_REQUEST_NULL;
    MPI_Status status;
    int flag = 0;
    int index = 0;
    int outcount = 0;
    int indices[1] = {0};

    TEST_CHECK_SUCCESS(MPI_Waitall(0, &request, &status));

    TEST_CHECK_SUCCESS(MPI_Testall(0, &request, &flag, &status));
    assert(flag == 1);

    flag = 0;
    index = 0;
    TEST_CHECK_SUCCESS(MPI_Testany(0, &request, &index, &flag, &status));
    assert(flag == 1);
    assert(index == MPI_UNDEFINED);

    index = 0;
    TEST_CHECK_SUCCESS(MPI_Waitany(0, &request, &index, &status));
    assert(index == MPI_UNDEFINED);

    outcount = 0;
    TEST_CHECK_SUCCESS(
        MPI_Testsome(0, &request, &outcount, indices, &status));
    assert(outcount == MPI_UNDEFINED);

    outcount = 0;
    TEST_CHECK_SUCCESS(
        MPI_Waitsome(0, &request, &outcount, indices, &status));
    assert(outcount == MPI_UNDEFINED);

    TEST_CHECK_SUCCESS(MPI_Startall(0, &request));
    world_barrier();
}

static void test_testall(int rank, int size) {
    MPI_Request requests[REQUEST_COUNT] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL
    };
    MPI_Status statuses[REQUEST_COUNT];
    int values[3] = {-1, -1, -1};
    int flag = 0;

    if (size >= 2 && rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[0], 1, MPI_INT, 1, TAG_BASE, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[1], 2, MPI_INT, 1, TAG_BASE + 1, MPI_COMM_WORLD,
            &requests[1]));
        while (!flag) {
            TEST_CHECK_SUCCESS(
                MPI_Testall(REQUEST_COUNT, requests, &flag, statuses));
        }
        assert(values[0] == 10);
        assert(values[1] == 11);
        assert(values[2] == 12);
        {
            int first_count = 0;
            int second_count = 0;

            TEST_CHECK_SUCCESS(
                MPI_Get_count(&statuses[0], MPI_INT, &first_count));
            TEST_CHECK_SUCCESS(
                MPI_Get_count(&statuses[1], MPI_INT, &second_count));
            assert(first_count == 1);
            assert(second_count == 2);
        }
        assert(requests[0] == MPI_REQUEST_NULL);
        assert(requests[1] == MPI_REQUEST_NULL);
    } else if (size >= 2 && rank == 1) {
        int sends[3] = {10, 11, 12};

        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[0], 1, MPI_INT, 0, TAG_BASE, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[1], 2, MPI_INT, 0, TAG_BASE + 1, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
    }
    world_barrier();
}

static void test_testany(int rank, int size) {
    MPI_Request requests[REQUEST_COUNT] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL
    };
    MPI_Status statuses[REQUEST_COUNT];
    int values[REQUEST_COUNT] = {-1, -1};
    int flag = 0;
    int index = MPI_UNDEFINED;

    if (size >= 2 && rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[0], 1, MPI_INT, 1, TAG_BASE + 10, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[1], 1, MPI_INT, 1, TAG_BASE + 11, MPI_COMM_WORLD,
            &requests[1]));
        while (!flag) {
            TEST_CHECK_SUCCESS(
                MPI_Testany(REQUEST_COUNT, requests, &index, &flag,
                            &statuses[0]));
        }
        assert(index == 0 || index == 1);
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
        assert(values[0] == 20);
        assert(values[1] == 21);
    } else if (size >= 2 && rank == 1) {
        int sends[REQUEST_COUNT] = {20, 21};

        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[0], 1, MPI_INT, 0, TAG_BASE + 10, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[1], 1, MPI_INT, 0, TAG_BASE + 11, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
    }
    world_barrier();
}

static void test_testsome(int rank, int size) {
    MPI_Request requests[REQUEST_COUNT] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL
    };
    MPI_Status statuses[REQUEST_COUNT];
    int indices[REQUEST_COUNT] = {-1, -1};
    int values[REQUEST_COUNT] = {-1, -1};
    int completed = 0;

    if (size >= 2 && rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[0], 1, MPI_INT, 1, TAG_BASE + 20, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[1], 1, MPI_INT, 1, TAG_BASE + 21, MPI_COMM_WORLD,
            &requests[1]));
        while (completed < REQUEST_COUNT) {
            int outcount = 0;

            TEST_CHECK_SUCCESS(
                MPI_Testsome(REQUEST_COUNT, requests, &outcount, indices,
                             statuses));
            if (outcount != MPI_UNDEFINED) {
                assert(outcount >= 0);
                completed += outcount;
            }
        }
        assert(values[0] == 30);
        assert(values[1] == 31);
    } else if (size >= 2 && rank == 1) {
        int sends[REQUEST_COUNT] = {30, 31};

        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[0], 1, MPI_INT, 0, TAG_BASE + 20, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[1], 1, MPI_INT, 0, TAG_BASE + 21, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
    }
    world_barrier();
}

static void test_waitany(int rank, int size) {
    MPI_Request requests[REQUEST_COUNT] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL
    };
    MPI_Status statuses[REQUEST_COUNT];
    int values[REQUEST_COUNT] = {-1, -1};
    int seen[REQUEST_COUNT] = {0, 0};

    if (size >= 2 && rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[0], 1, MPI_INT, 1, TAG_BASE + 30, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[1], 1, MPI_INT, 1, TAG_BASE + 31, MPI_COMM_WORLD,
            &requests[1]));
        for (int i = 0; i < REQUEST_COUNT; i++) {
            int index = MPI_UNDEFINED;

            TEST_CHECK_SUCCESS(
                MPI_Waitany(REQUEST_COUNT, requests, &index, &statuses[i]));
            assert(index == 0 || index == 1);
            assert(seen[index] == 0);
            seen[index] = 1;
        }
        assert(values[0] == 40);
        assert(values[1] == 41);
    } else if (size >= 2 && rank == 1) {
        int sends[REQUEST_COUNT] = {40, 41};

        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[0], 1, MPI_INT, 0, TAG_BASE + 30, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[1], 1, MPI_INT, 0, TAG_BASE + 31, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
    }
    world_barrier();
}

static void test_waitsome(int rank, int size) {
    MPI_Request requests[REQUEST_COUNT] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL
    };
    MPI_Status statuses[REQUEST_COUNT];
    int indices[REQUEST_COUNT] = {-1, -1};
    int values[REQUEST_COUNT] = {-1, -1};
    int completed = 0;

    if (size >= 2 && rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[0], 1, MPI_INT, 1, TAG_BASE + 40, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Irecv(
            &values[1], 1, MPI_INT, 1, TAG_BASE + 41, MPI_COMM_WORLD,
            &requests[1]));
        while (completed < REQUEST_COUNT) {
            int outcount = 0;

            TEST_CHECK_SUCCESS(
                MPI_Waitsome(REQUEST_COUNT, requests, &outcount, indices,
                             statuses));
            assert(outcount > 0);
            completed += outcount;
        }
        assert(values[0] == 50);
        assert(values[1] == 51);
    } else if (size >= 2 && rank == 1) {
        int sends[REQUEST_COUNT] = {50, 51};

        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[0], 1, MPI_INT, 0, TAG_BASE + 40, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Isend(
            &sends[1], 1, MPI_INT, 0, TAG_BASE + 41, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
    }
    world_barrier();
}

static void test_startall(int rank, int size) {
    MPI_Request requests[REQUEST_COUNT] = {
        MPI_REQUEST_NULL, MPI_REQUEST_NULL
    };
    MPI_Status statuses[REQUEST_COUNT];
    int values[REQUEST_COUNT] = {-1, -1};

    if (size >= 2 && rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Recv_init(
            &values[0], 1, MPI_INT, 1, TAG_BASE + 50, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Recv_init(
            &values[1], 1, MPI_INT, 1, TAG_BASE + 51, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(MPI_Startall(REQUEST_COUNT, requests));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
        assert(values[0] == 60);
        assert(values[1] == 61);
    } else if (size >= 2 && rank == 1) {
        int sends[REQUEST_COUNT] = {60, 61};

        TEST_CHECK_SUCCESS(MPI_Send_init(
            &sends[0], 1, MPI_INT, 0, TAG_BASE + 50, MPI_COMM_WORLD,
            &requests[0]));
        TEST_CHECK_SUCCESS(MPI_Send_init(
            &sends[1], 1, MPI_INT, 0, TAG_BASE + 51, MPI_COMM_WORLD,
            &requests[1]));
        TEST_CHECK_SUCCESS(MPI_Startall(REQUEST_COUNT, requests));
        TEST_CHECK_SUCCESS(
            MPI_Waitall(REQUEST_COUNT, requests, statuses));
    }

    if (size >= 2 && (rank == 0 || rank == 1)) {
        TEST_CHECK_SUCCESS(MPI_Request_free(&requests[0]));
        TEST_CHECK_SUCCESS(MPI_Request_free(&requests[1]));
    }
    world_barrier();
}

int main(int argc, char **argv) {
    int rank;
    int size;

    TEST_CHECK_SUCCESS(MPI_Init(&argc, &argv));
    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    test_zero_count();
    test_testall(rank, size);
    test_testany(rank, size);
    test_testsome(rank, size);
    test_waitany(rank, size);
    test_waitsome(rank, size);
    test_startall(rank, size);

    if (rank == 0) {
        if (size < 2) {
            printf("Request-array zero-count scenarios passed; "
                   "communication scenarios require 2+ ranks\n");
        } else {
            printf("All request-array scenarios passed\n");
        }
    }

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
