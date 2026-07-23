/* tests/mpi/test_spawn_child.c - Child process for spawn tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"

#define CHILD_MAGIC 0xC0FFEE

int main(int argc, char **argv) {
    int ret, rank;
    MPI_Comm parent;
    int sendbuf = CHILD_MAGIC;
    int recvbuf = 0;
    MPI_Status status;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Child: MPI_Init failed\n");
        return 1;
    }

    /* Get parent intercommunicator */
    ret = MPI_Comm_get_parent(&parent);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Child: MPI_Comm_get_parent failed\n");
        MPI_Finalize();
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Exchange data with parent - child sends, then receives */
    ret = MPI_Send(&sendbuf, 1, MPI_INT, 0, 100, parent);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Child: MPI_Send failed\n");
        MPI_Comm_disconnect(&parent);
        MPI_Finalize();
        return 1;
    }

    ret = MPI_Recv(&recvbuf, 1, MPI_INT, 0, 101, parent, &status);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Child: MPI_Recv failed\n");
        MPI_Comm_disconnect(&parent);
        MPI_Finalize();
        return 1;
    }

    if (recvbuf != CHILD_MAGIC + 1) {
        fprintf(stderr, "Child: Received wrong data (expected %d, got %d)\n",
                CHILD_MAGIC + 1, recvbuf);
        MPI_Comm_disconnect(&parent);
        MPI_Finalize();
        return 1;
    }

    /* Disconnect from parent */
    ret = MPI_Comm_disconnect(&parent);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Child: MPI_Comm_disconnect failed\n");
        MPI_Finalize();
        return 1;
    }

    MPI_Finalize();
    return 0;
}
