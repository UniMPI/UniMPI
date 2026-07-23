/* thread_init.c - Portable MPI_Init_thread example. */
#include <stdio.h>

#include "unimpi.h"

static const char *thread_level_name(int level) {
    switch (level) {
        case UNIMPI_THREAD_SINGLE:
            return "single";
        case UNIMPI_THREAD_FUNNELED:
            return "funneled";
        case UNIMPI_THREAD_SERIALIZED:
            return "serialized";
        case UNIMPI_THREAD_MULTIPLE:
            return "multiple";
        default:
            return "unknown";
    }
}

static int check_mpi_call(int code, const char *operation) {
    if (code == 0) {
        return 0;
    }
    fprintf(stderr, "%s failed with MPI error %d: %s\n",
            operation, code, unimpi_mpi_error_string(code));
    return -1;
}

int main(int argc, char **argv) {
    const int requested = UNIMPI_THREAD_MULTIPLE;
    int provided = -1;
    int rank;
    int size;
    int ret;

    ret = unimpi_init_thread(&argc, &argv, requested, &provided);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi thread initialization failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }
    if (provided < UNIMPI_THREAD_SINGLE ||
        provided > UNIMPI_THREAD_MULTIPLE) {
        fprintf(stderr, "backend returned invalid thread level %d\n",
                provided);
        (void)unimpi.abort(UNIMPI_COMM_WORLD, 1);
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

    if (rank == 0) {
        printf("Requested thread support: %s\n",
               thread_level_name(requested));
        printf("Provided thread support: %s\n",
               thread_level_name(provided));
        printf("Backend: %s, processes: %d\n",
               unimpi_get_backend_name() ? unimpi_get_backend_name()
                                         : "unknown",
               size);
        if (provided < requested) {
            printf("The application must restrict MPI calls to the provided "
                   "thread level.\n");
        }
    }

    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
