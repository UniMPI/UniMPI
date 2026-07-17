/* Minimal example using standard MPI style */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;

    /* Initialize unimpi (auto-detects backend) - must call this first! */
    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    /* Get backend name */
    printf("Using backend: %s\n", unimpi_get_backend_name());

    /* Get rank and size using standard MPI names */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d\n", rank, size);

    /* Barrier */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Finalize */
    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    return 0;
}
