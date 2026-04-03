#include <stdio.h>
#include "tftk_mpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;

    /* Initialize TFTK-MPI (auto-detects backend) */
    ret = tftk_mpi_init(&argc, &argv);
    if (ret != TFTK_MPI_OK) {
        fprintf(stderr, "TFTK-MPI init failed: %s\n", tftk_mpi_error_string(ret));
        return 1;
    }

    /* Get backend name */
    printf("Using backend: %s\n", tftk_mpi_get_backend_name());

    /* Get rank and size using vtable */
    tftk_mpi.comm_rank(TFTK_MPI_COMM_WORLD, &rank);
    tftk_mpi.comm_size(TFTK_MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d\n", rank, size);

    /* Barrier */
    tftk_mpi.barrier(TFTK_MPI_COMM_WORLD);

    /* Finalize */
    ret = tftk_mpi_finalize();
    if (ret != TFTK_MPI_OK) {
        fprintf(stderr, "TFTK-MPI finalize failed: %s\n", tftk_mpi_error_string(ret));
        return 1;
    }

    return 0;
}
