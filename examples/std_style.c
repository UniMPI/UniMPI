#include <stdio.h>

#include "unimpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;

    /* Initialize using standard MPI naming */
    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    /* Get rank and size using standard MPI calls */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d (standard MPI style)\n", rank, size);

    /* Barrier */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Finalize using standard MPI naming */
    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    return 0;
}
