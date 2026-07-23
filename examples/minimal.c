/* Minimal example using the unimpi function-pointer interface. */
#include <stdio.h>

#include "unimpi.h"

static int check_mpi_call(int code, const char *operation) {
    if (code == 0) {
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

    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    if (check_mpi_call(
            unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank),
            "unimpi.comm_rank") != 0 ||
        check_mpi_call(
            unimpi.comm_size(UNIMPI_COMM_WORLD, &size),
            "unimpi.comm_size") != 0) {
        (void)unimpi.abort(UNIMPI_COMM_WORLD, 1);
        return 1;
    }

    backend_name = unimpi_get_backend_name();
    printf("Hello from rank %d of %d using %s (vtable style)\n",
           rank, size, backend_name ? backend_name : "unknown");

    if (check_mpi_call(unimpi.barrier(UNIMPI_COMM_WORLD),
                       "unimpi.barrier") != 0) {
        (void)unimpi.abort(UNIMPI_COMM_WORLD, 1);
        return 1;
    }

    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
