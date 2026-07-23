#include <stdio.h>

#include "unimpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;

    /* Initialize using standard MPI naming */
    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    /* Get rank and size using standard MPI calls */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d (standard MPI style)\n", rank, size);

    /* Barrier */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Finalize using standard MPI naming */
    ret = MPI_Finalize();
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Finalize failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    return 0;
}
