/* tests/unit/test_collective.c - Collective communication tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"

#define TEST_BUF_SIZE 256

void test_bcast(void) {
    int rank, size;
    int buf = 0;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        buf = 42;
    }

    /* Broadcast from rank 0 */
    unimpi.bcast(&buf, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* All ranks should have the value */
    assert(buf == 42);

    if (rank == 0) {
        printf("  Bcast test passed\n");
    }
}

void test_reduce(void) {
    int rank, size;
    int send_val, recv_val;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    send_val = rank + 1;
    recv_val = 0;

    /* Sum reduction */
    unimpi.reduce(&send_val, &recv_val, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        /* Expected: 1 + 2 + ... + size = size * (size + 1) / 2 */
        int expected = size * (size + 1) / 2;
        assert(recv_val == expected);
        printf("  Reduce test passed\n");
    }
}

void test_allreduce(void) {
    int rank, size;
    int send_val, recv_val;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    send_val = rank + 1;
    recv_val = 0;

    /* Allreduce with sum */
    unimpi.allreduce(&send_val, &recv_val, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    /* All ranks should have the sum */
    int expected = size * (size + 1) / 2;
    assert(recv_val == expected);

    if (rank == 0) {
        printf("  Allreduce test passed\n");
    }
}

void test_gather(void) {
    int rank, size;
    int send_val;
    int *recv_buf = NULL;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    send_val = rank * 10;

    if (rank == 0) {
        recv_buf = (int*)malloc(size * sizeof(int));
        assert(recv_buf != NULL);
    }

    /* Gather to rank 0 */
    unimpi.gather(&send_val, 1, MPI_INT, recv_buf, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            assert(recv_buf[i] == i * 10);
        }
        printf("  Gather test passed\n");
        free(recv_buf);
    }
}

void test_allgather(void) {
    int rank, size;
    int send_val;
    int *recv_buf;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    send_val = rank * 10;
    recv_buf = (int*)malloc(size * sizeof(int));
    assert(recv_buf != NULL);

    /* Allgather */
    unimpi.allgather(&send_val, 1, MPI_INT, recv_buf, 1, MPI_INT, MPI_COMM_WORLD);

    /* All ranks should have all values */
    for (int i = 0; i < size; i++) {
        assert(recv_buf[i] == i * 10);
    }

    free(recv_buf);

    if (rank == 0) {
        printf("  Allgather test passed\n");
    }
}

void test_scatter(void) {
    int rank, size;
    int *send_buf = NULL;
    int recv_val;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        send_buf = (int*)malloc(size * sizeof(int));
        assert(send_buf != NULL);
        for (int i = 0; i < size; i++) {
            send_buf[i] = i * 100;
        }
    }

    /* Scatter from rank 0 */
    unimpi.scatter(send_buf, 1, MPI_INT, &recv_val, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* Each rank should receive its value */
    assert(recv_val == rank * 100);

    if (rank == 0) {
        printf("  Scatter test passed\n");
        free(send_buf);
    }
}

void test_barrier(void) {
    int rank;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Simple barrier test - just make sure it doesn't hang */
    unimpi.barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("  Barrier test passed\n");
    }
}

int main(int argc, char **argv) {
    int ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to initialize TFTK-MPI: %s\n", unimpi_error_string(ret));
        return 1;
    }

    printf("Running collective communication tests...\n");
    printf("Using backend: %s\n", unimpi_get_backend_name());

    test_bcast();
    test_reduce();
    test_allreduce();
    test_gather();
    test_allgather();
    test_scatter();
    test_barrier();

    printf("All collective tests passed!\n");

    unimpi_finalize();
    return 0;
}
