/* nonblocking.c - Non-blocking communication example using Isend/Irecv */
#include <stdio.h>
#include <string.h>
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

int main(int argc, char **argv) {
    int rank, size;
    int sendbuf, recvbuf;
    MPI_Request requests[2];
    MPI_Status statuses[2];
    int flag;

    /* Initialize unimpi (auto-detects backend) */
    unimpi_init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        printf("This example requires at least 2 processes\n");
        unimpi_finalize();
        return 1;
    }

    /* Rank 0 sends to Rank 1 */
    if (rank == 0) {
        sendbuf = 42;
        printf("Rank 0: Initiating non-blocking send of %d to rank 1\n", sendbuf);

        /* Non-blocking send */
        MPI_Isend(&sendbuf, 1, MPI_INT, 1, 100, MPI_COMM_WORLD, &requests[0]);

        /* Do some work while send completes */
        printf("Rank 0: Doing other work while send completes...\n");

        /* Wait for send to complete */
        MPI_Wait(&requests[0], &statuses[0]);
        printf("Rank 0: Send completed\n");

    } else if (rank == 1) {
        /* Non-blocking receive */
        printf("Rank 1: Initiating non-blocking receive\n");
        MPI_Irecv(&recvbuf, 1, MPI_INT, 0, 100, MPI_COMM_WORLD, &requests[0]);

        /* Test if receive is complete */
        MPI_Test(&requests[0], &flag, &statuses[0]);
        if (!flag) {
            printf("Rank 1: Receive not yet complete, waiting...\n");
        }

        /* Wait for receive to complete */
        MPI_Wait(&requests[0], &statuses[0]);
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
    MPI_Sendrecv(&sendbuf, 1, MPI_INT, next, 200,
                 &recvbuf, 1, MPI_INT, prev, 200,
                 MPI_COMM_WORLD, &statuses[0]);

    printf("Rank %d: Ring complete - received %d\n", rank, recvbuf);

    /* Non-blocking barrier test */
    MPI_Request barrier_req;
    printf("Rank %d: Initiating non-blocking barrier\n", rank);
    MPI_Ibarrier(MPI_COMM_WORLD, &barrier_req);

    /* Wait for barrier */
    MPI_Wait(&barrier_req, &statuses[0]);
    printf("Rank %d: Barrier completed\n", rank);

    unimpi_finalize();
    return 0;
}
