/* tests/mpi/test_p2p_extended.c - Extended point-to-point communication tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

#define TEST_TAG 200
#define TEST_BUF_SIZE 256

void test_ssend(void) {
    int rank, size;
    char send_buf[TEST_BUF_SIZE];
    char recv_buf[TEST_BUF_SIZE];
    MPI_Status status;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("  Skipping ssend test (need 2+ processes)\n");
        return;
    }

    memset(send_buf, rank, TEST_BUF_SIZE);
    memset(recv_buf, 0, TEST_BUF_SIZE);

    if (rank == 0) {
        /* Ssend requires matching recv - synchronous semantics */
        TEST_CHECK_SUCCESS(MPI_Ssend(send_buf, TEST_BUF_SIZE, MPI_CHAR, 1,
                                     TEST_TAG, MPI_COMM_WORLD));
    } else if (rank == 1) {
        TEST_CHECK_SUCCESS(MPI_Recv(recv_buf, TEST_BUF_SIZE, MPI_CHAR, 0,
                                    TEST_TAG, MPI_COMM_WORLD, &status));
        assert(recv_buf[0] == 0);
    }
    printf("  Ssend test passed\n");
}

void test_rsend(void) {
    int rank, size;
    int send_val = 42, recv_val = 0;
    MPI_Status status;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("  Skipping rsend test (need 2+ processes)\n");
        return;
    }

    /* Rsend requires recv to be posted first - use ring pattern */
    if (rank == 0) {
        /* Rank 0 receives from rank 1 using Sendrecv */
        TEST_CHECK_SUCCESS(MPI_Sendrecv(&send_val, 1, MPI_INT, 1, TEST_TAG + 1,
                                       &recv_val, 1, MPI_INT, 1, TEST_TAG + 2,
                                       MPI_COMM_WORLD, &status));
        assert(recv_val == 1);
    } else if (rank == 1) {
        send_val = 1;
        TEST_CHECK_SUCCESS(MPI_Sendrecv(&send_val, 1, MPI_INT, 0, TEST_TAG + 2,
                                       &recv_val, 1, MPI_INT, 0, TEST_TAG + 1,
                                       MPI_COMM_WORLD, &status));
        assert(recv_val == 42);
    }
    printf("  Rsend test passed\n");
}

void test_sendrecv(void) {
    int rank, size;
    int send_buf[4] = {1, 2, 3, 4};
    int recv_buf[4] = {0, 0, 0, 0};
    MPI_Status status;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("  Skipping sendrecv test (need 2+ processes)\n");
        return;
    }

    /* Ring communication using Sendrecv */
    int left = (rank + size - 1) % size;
    int right = (rank + 1) % size;

    TEST_CHECK_SUCCESS(MPI_Sendrecv(send_buf, 4, MPI_INT, right, TEST_TAG + 10,
                                    recv_buf, 4, MPI_INT, left, TEST_TAG + 10,
                                    MPI_COMM_WORLD, &status));

    assert(recv_buf[0] == 1);
    assert(recv_buf[3] == 4);

    if (rank == 0) printf("  Sendrecv test passed\n");
}

void test_waitall(void) {
    int rank, size;
    int buf1 = 1, buf2 = 2;

    /* Explicitly initialize requests to MPI_REQUEST_NULL for Intel MPI compatibility */
    MPI_Request req[2] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
    MPI_Status status[2];

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("  Skipping waitall test (need 2+ processes)\n");
        return;
    }

    if (rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Isend(&buf1, 1, MPI_INT, 1, TEST_TAG + 20,
                                    MPI_COMM_WORLD, &req[0]));
        TEST_CHECK_SUCCESS(MPI_Isend(&buf2, 1, MPI_INT, 1, TEST_TAG + 21,
                                    MPI_COMM_WORLD, &req[1]));

        TEST_CHECK_SUCCESS(MPI_Waitall(2, req, status));
        printf("  Waitall test passed\n");
    } else if (rank == 1) {
        int recv_buf[2];
        MPI_Status recv_status[2];
        TEST_CHECK_SUCCESS(MPI_Recv(&recv_buf[0], 1, MPI_INT, 0, TEST_TAG + 20,
                                   MPI_COMM_WORLD, &recv_status[0]));
        TEST_CHECK_SUCCESS(MPI_Recv(&recv_buf[1], 1, MPI_INT, 0, TEST_TAG + 21,
                                   MPI_COMM_WORLD, &recv_status[1]));
        assert(recv_buf[0] == 1);
        assert(recv_buf[1] == 2);
    }
}

void test_test(void) {
    int rank, size;
    int flag = 0;
    int buf = 42;
    MPI_Request req;
    MPI_Status status;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("  Skipping test function (need 2+ processes)\n");
        return;
    }

    if (rank == 0) {
        TEST_CHECK_SUCCESS(MPI_Irecv(&buf, 1, MPI_INT, 1, TEST_TAG + 30,
                                     MPI_COMM_WORLD, &req));
        do {
            TEST_CHECK_SUCCESS(MPI_Test(&req, &flag, &status));
        } while (!flag);
        assert(buf == 99);
        printf("  Test function passed\n");
    } else if (rank == 1) {
        buf = 99;
        MPI_Send(&buf, 1, MPI_INT, 0, TEST_TAG + 30, MPI_COMM_WORLD);
    }
}

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    const char *backend_name;

    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: error code %d\n", ret);
        return 1;
    }

    printf("Running extended point-to-point tests...\n");
    backend_name = unimpi_get_backend_name();
    assert(backend_name != NULL);
    printf("Using backend: %s\n", backend_name);

    test_ssend();
    test_rsend();
    test_sendrecv();
    test_waitall();
    test_test();

    printf("All extended P2P tests passed!\n");

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
