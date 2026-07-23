/* nonblocking.c - Non-blocking communication example using Isend/Irecv */
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

static int mpi_failure(const char *operation, int code) {
    fprintf(stderr, "%s failed with MPI error %d\n", operation, code);
    if (unimpi.abort != NULL) {
        (void)unimpi.abort(MPI_COMM_WORLD, code);
    }
    return 1;
}

static int validation_failure(const char *message) {
    fprintf(stderr, "validation failed: %s\n", message);
    if (unimpi.abort != NULL) {
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
    }
    return 1;
}

#define CHECK_MPI(call)                                                     \
    do {                                                                    \
        int check_result = (call);                                          \
        if (check_result != MPI_SUCCESS) {                                  \
            return mpi_failure(#call, check_result);                        \
        }                                                                   \
    } while (0)

int main(int argc, char **argv) {
    int ret;
    int rank, size;
    int sendbuf, recvbuf;
    MPI_Request requests[2];
    MPI_Status statuses[2];
    int flag;

    /* Initialize unimpi (auto-detects backend) */
    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    CHECK_MPI(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    CHECK_MPI(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("This example requires at least 2 processes\n");
        ret = unimpi_finalize();
        if (ret != UNIMPI_OK) {
            fprintf(stderr, "unimpi finalize failed: %s\n",
                    unimpi_error_string(ret));
        }
        return 1;
    }

    /* Rank 0 sends to Rank 1 */
    if (rank == 0) {
        sendbuf = 42;
        printf("Rank 0: Initiating non-blocking send of %d to rank 1\n", sendbuf);

        /* Non-blocking send */
        CHECK_MPI(MPI_Isend(
            &sendbuf, 1, MPI_INT, 1, 100, MPI_COMM_WORLD, &requests[0]));

        /* Do some work while send completes */
        printf("Rank 0: Doing other work while send completes...\n");

        /* Wait for send to complete */
        CHECK_MPI(MPI_Wait(&requests[0], &statuses[0]));
        printf("Rank 0: Send completed\n");

    } else if (rank == 1) {
        /* Non-blocking receive */
        printf("Rank 1: Initiating non-blocking receive\n");
        CHECK_MPI(MPI_Irecv(
            &recvbuf, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, &requests[0]));

        /* Test if receive is complete */
        CHECK_MPI(MPI_Test(&requests[0], &flag, &statuses[0]));
        if (!flag) {
            printf("Rank 1: Receive not yet complete, waiting...\n");
        }

        /* Wait for receive to complete */
        CHECK_MPI(MPI_Wait(&requests[0], &statuses[0]));
        if (recvbuf != 42) {
            return validation_failure(
                "rank 1 did not receive the point-to-point value 42");
        }
        printf("Rank 1: Received value %d from rank 0\n", recvbuf);
    }

    /* Ring communication example: Each process sends to next, receives from previous */
    int next = (rank + 1) % size;
    int prev = (rank - 1 + size) % size;
    sendbuf = rank * 100;
    recvbuf = -1;

    printf("Rank %d: Ring communication - sending %d to rank %d, receiving from rank %d\n",
           rank, sendbuf, next, prev);

    /* Use MPI_Sendrecv for ring communication */
    CHECK_MPI(MPI_Sendrecv(
        &sendbuf, 1, MPI_INT, next, 200,
        &recvbuf, 1, MPI_INT, prev, 200,
        MPI_COMM_WORLD, &statuses[0]));
    if (recvbuf != prev * 100) {
        return validation_failure(
            "ring receive did not match the previous rank's payload");
    }

    printf("Rank %d: Ring complete - received %d\n", rank, recvbuf);

    /* Non-blocking barrier test */
    MPI_Request barrier_req;
    if (unimpi.ibarrier == NULL) {
        return validation_failure("MPI_Ibarrier is not available");
    }
    printf("Rank %d: Initiating non-blocking barrier\n", rank);
    CHECK_MPI(MPI_Ibarrier(MPI_COMM_WORLD, &barrier_req));

    /* Wait for barrier */
    CHECK_MPI(MPI_Wait(&barrier_req, &statuses[0]));
    printf("Rank %d: Barrier completed\n", rank);

    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n", unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
