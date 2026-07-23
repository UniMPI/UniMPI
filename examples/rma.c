/* rma.c - Portable fence-and-put Remote Memory Access example. */
#include <stdio.h>

#include "unimpi.h"

static int check_mpi_call(int code, const char *operation) {
    if (code == MPI_SUCCESS) {
        return 0;
    }
    fprintf(stderr, "%s failed with MPI error %d: %s\n",
            operation, code, unimpi_mpi_error_string(code));
    (void)MPI_Abort(MPI_COMM_WORLD, code);
    return -1;
}

int main(int argc, char **argv) {
    MPI_Win window;
    int *window_value = NULL;
    int origin_value;
    int expected_value;
    int local_valid;
    int globally_valid;
    int target_rank;
    int previous_rank;
    int rank;
    int size;
    int ret;

    ret = MPI_Init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    if (check_mpi_call(MPI_Comm_rank(MPI_COMM_WORLD, &rank),
                       "MPI_Comm_rank") != 0 ||
        check_mpi_call(MPI_Comm_size(MPI_COMM_WORLD, &size),
                       "MPI_Comm_size") != 0) {
        return 1;
    }
    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr, "rma example requires at least two processes\n");
        }
        ret = MPI_Finalize();
        return ret == UNIMPI_OK ? 2 : 1;
    }

    if (check_mpi_call(
            MPI_Win_allocate((MPI_Aint)sizeof(*window_value),
                             (int)sizeof(*window_value), MPI_INFO_NULL,
                             MPI_COMM_WORLD, &window_value, &window),
            "MPI_Win_allocate") != 0) {
        return 1;
    }

    *window_value = -1;
    origin_value = 1000 + rank;
    target_rank = (rank + 1) % size;
    previous_rank = (rank - 1 + size) % size;

    if (check_mpi_call(MPI_Win_fence(0, window),
                       "MPI_Win_fence(open epoch)") != 0 ||
        check_mpi_call(
            MPI_Put(&origin_value, 1, MPI_INT, target_rank, (MPI_Aint)0,
                    1, MPI_INT, window),
            "MPI_Put") != 0 ||
        check_mpi_call(MPI_Win_fence(0, window),
                       "MPI_Win_fence(close epoch)") != 0) {
        return 1;
    }

    expected_value = 1000 + previous_rank;
    local_valid = *window_value == expected_value;
    globally_valid = 0;
    if (check_mpi_call(
            MPI_Allreduce(&local_valid, &globally_valid, 1, MPI_INT, MPI_MIN,
                          MPI_COMM_WORLD),
            "MPI_Allreduce(validation)") != 0) {
        return 1;
    }
    if (!globally_valid) {
        fprintf(stderr,
                "rank %d received %d through RMA, expected %d\n",
                rank, *window_value, expected_value);
        (void)MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    if (rank == 0) {
        printf("RMA ring put succeeded across %d processes using %s\n",
               size,
               unimpi_get_backend_name() ? unimpi_get_backend_name()
                                         : "unknown");
    }

    if (check_mpi_call(MPI_Win_free(&window), "MPI_Win_free") != 0) {
        return 1;
    }

    ret = MPI_Finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
