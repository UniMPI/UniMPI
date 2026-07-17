/* tests/unit/test_p2p.c - Point-to-point communication tests */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"

#define TEST_TAG 100
#define TEST_BUF_SIZE 1024

void test_send_recv(void) {
    int rank, size;
    char send_buf[TEST_BUF_SIZE];
    char recv_buf[TEST_BUF_SIZE];
    UNIMPI_Status status;

    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("  Skipping send/recv test (need 2+ processes)\n");
        return;
    }

    /* Initialize send buffer */
    memset(send_buf, rank, TEST_BUF_SIZE);
    memset(recv_buf, 0, TEST_BUF_SIZE);

    if (rank == 0) {
        /* Send to rank 1 */
        unimpi.send(send_buf, TEST_BUF_SIZE, MPI_CHAR, 1, TEST_TAG, UNIMPI_COMM_WORLD);
        /* Receive from rank 1 */
        unimpi.recv(recv_buf, TEST_BUF_SIZE, MPI_CHAR, 1, TEST_TAG, UNIMPI_COMM_WORLD, &status);
        /* Verify data */
        assert(recv_buf[0] == 1);
    } else if (rank == 1) {
        /* Receive from rank 0 */
        unimpi.recv(recv_buf, TEST_BUF_SIZE, MPI_CHAR, 0, TEST_TAG, UNIMPI_COMM_WORLD, &status);
        /* Verify data */
        assert(recv_buf[0] == 0);
        /* Send to rank 0 */
        unimpi.send(send_buf, TEST_BUF_SIZE, MPI_CHAR, 0, TEST_TAG, UNIMPI_COMM_WORLD);
    }

    printf("  Send/recv test passed\n");
}

void test_isend_irecv(void) {
    int rank, size;
    char send_buf[TEST_BUF_SIZE];
    char recv_buf[TEST_BUF_SIZE];
    MPI_Request send_req, recv_req;
    UNIMPI_Status status;

    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("  Skipping isend/irecv test (need 2+ processes)\n");
        return;
    }

    memset(send_buf, rank, TEST_BUF_SIZE);
    memset(recv_buf, 0, TEST_BUF_SIZE);

    if (rank == 0) {
        /* Start recv first */
        unimpi.irecv(recv_buf, TEST_BUF_SIZE, MPI_CHAR, 1, TEST_TAG + 1, UNIMPI_COMM_WORLD, &recv_req);
        /* Then start send */
        unimpi.isend(send_buf, TEST_BUF_SIZE, MPI_CHAR, 1, TEST_TAG, UNIMPI_COMM_WORLD, &send_req);

        /* Wait for both */
        unimpi.wait(&send_req, &status);
        unimpi.wait(&recv_req, &status);

        assert(recv_buf[0] == 1);
    } else if (rank == 1) {
        /* Start recv first */
        unimpi.irecv(recv_buf, TEST_BUF_SIZE, MPI_CHAR, 0, TEST_TAG, UNIMPI_COMM_WORLD, &recv_req);
        /* Then start send */
        unimpi.isend(send_buf, TEST_BUF_SIZE, MPI_CHAR, 0, TEST_TAG + 1, UNIMPI_COMM_WORLD, &send_req);

        /* Wait for both */
        unimpi.wait(&recv_req, &status);
        unimpi.wait(&send_req, &status);

        assert(recv_buf[0] == 0);
    }

    printf("  Isend/Irecv test passed\n");
}

void test_sendrecv_replace(void) {
    int rank, size;
    int buf;
    UNIMPI_Status status;

    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("  Skipping sendrecv_replace test (need 2+ processes)\n");
        return;
    }

    buf = rank;

    if (rank == 0) {
        unimpi.sendrecv_replace(&buf, 1, MPI_INT, 1, TEST_TAG + 2, 1, TEST_TAG + 2, UNIMPI_COMM_WORLD, &status);
        assert(buf == 1);
    } else if (rank == 1) {
        unimpi.sendrecv_replace(&buf, 1, MPI_INT, 0, TEST_TAG + 2, 0, TEST_TAG + 2, UNIMPI_COMM_WORLD, &status);
        assert(buf == 0);
    }

    printf("  Sendrecv_replace test passed\n");
}

void test_probe(void) {
    int rank, size;
    int flag;
    UNIMPI_Status status;

    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("  Skipping probe test (need 2+ processes)\n");
        return;
    }

    if (rank == 0) {
        int buf = 42;
        /* Send to rank 1 */
        unimpi.send(&buf, 1, MPI_INT, 1, TEST_TAG + 3, UNIMPI_COMM_WORLD);
    } else if (rank == 1) {
        /* Probe for message */
        do {
            unimpi.iprobe(0, TEST_TAG + 3, UNIMPI_COMM_WORLD, &flag, &status);
        } while (!flag);

        /* Now receive */
        int buf;
        unimpi.recv(&buf, 1, MPI_INT, 0, TEST_TAG + 3, UNIMPI_COMM_WORLD, &status);
        assert(buf == 42);
    }

    printf("  Probe test passed\n");
}

int main(int argc, char **argv) {
    int ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to initialize TFTK-MPI: %s\n", unimpi_error_string(ret));
        return 1;
    }

    printf("Running point-to-point tests...\n");
    printf("Using backend: %s\n", unimpi_get_backend_name());

    test_send_recv();
    test_isend_irecv();
    test_sendrecv_replace();
    test_probe();

    printf("All P2P tests passed!\n");

    unimpi_finalize();
    return 0;
}
