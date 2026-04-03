#include <stdio.h>

/* Must define this before including header for standard MPI names */
#define TFTK_MPI_USE_STD_NAMES
#include "tftk_mpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;

    /* Initialize using standard MPI naming */
    ret = tftk_mpi_init(&argc, &argv);
    if (ret != TFTK_MPI_OK) {
        fprintf(stderr, "TFTK-MPI init failed: %s\n", tftk_mpi_error_string(ret));
        return 1;
    }

    /* Get rank and size using standard MPI calls */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d (standard MPI style)\n", rank, size);

    /* Barrier */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Finalize using standard MPI naming */
    ret = tftk_mpi_finalize();
    if (ret != TFTK_MPI_OK) {
        fprintf(stderr, "TFTK-MPI finalize failed: %s\n", tftk_mpi_error_string(ret));
        return 1;
    }

    return 0;
}
