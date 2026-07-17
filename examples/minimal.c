#include <stdio.h>
#include "unimpi.h"

int main(int argc, char **argv) {
    int ret;
    int rank, size;

    /* Initialize TFTK-MPI (auto-detects backend) */
    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "TFTK-MPI init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    /* Get backend name */
    printf("Using backend: %s\n", unimpi_get_backend_name());

    /* Get rank and size using vtable */
    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);

    printf("Hello from rank %d of %d\n", rank, size);

    /* Barrier */
    unimpi.barrier(UNIMPI_COMM_WORLD);

    /* Finalize */
    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "TFTK-MPI finalize failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    return 0;
}
