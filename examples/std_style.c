#include <stdio.h>

#include "unimpi.h"

static int check_mpi_call(int code, const char *operation) {
    if (code == MPI_SUCCESS) {
        return 0;
    }
    fprintf(stderr, "%s failed with MPI error %d: %s\n",
            operation, code, unimpi_mpi_error_string(code));
    return -1;
}

int main(int argc, char **argv) {
    const char *backend_name;
    int ret;
    int rank;
    int size;

    ret = MPI_Init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "MPI_Init through unimpi failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }

    if (check_mpi_call(MPI_Comm_rank(MPI_COMM_WORLD, &rank),
                       "MPI_Comm_rank") != 0 ||
        check_mpi_call(MPI_Comm_size(MPI_COMM_WORLD, &size),
                       "MPI_Comm_size") != 0) {
        (void)MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    backend_name = unimpi_get_backend_name();
    printf("Hello from rank %d of %d using %s (standard MPI style)\n",
           rank, size, backend_name ? backend_name : "unknown");

    if (check_mpi_call(MPI_Barrier(MPI_COMM_WORLD), "MPI_Barrier") != 0) {
        (void)MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    ret = MPI_Finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "MPI_Finalize through unimpi failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
