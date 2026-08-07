/* test_collective_nonblocking.c - MPI-3 nonblocking collective coverage */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>

#include "unimpi.h"
#include "test_mpi_helpers.h"

static void check_value(const char *operation, int actual, int expected,
                        int rank) {
    if (actual != expected) {
        fprintf(stderr,
                "%s failed on rank %d: expected %d, got %d\n",
                operation, rank, expected, actual);
        abort();
    }
}

static int use_optional(const char *operation, int available,
                        int required, int rank) {
    if (available) {
        return 1;
    }
    if (required) {
        fprintf(stderr, "%s is missing on a backend that requires the "
                        "complete nonblocking-collective profile\n",
                operation);
        abort();
    }
    if (rank == 0) {
        printf("SKIP: %s is not exported by this MS-MPI runtime\n",
               operation);
    }
    return 0;
}

static void wait_for(MPI_Request *request) {
    MPI_Status status;

    TEST_CHECK_SUCCESS(MPI_Wait(request, &status));
}

static void test_barrier(void) {
    MPI_Request request = MPI_REQUEST_NULL;

    TEST_CHECK_SUCCESS(MPI_Ibarrier(MPI_COMM_WORLD, &request));
    wait_for(&request);
}

static void test_bcast(int rank) {
    MPI_Request request = MPI_REQUEST_NULL;
    int value = rank == 0 ? 1234 : -1;

    TEST_CHECK_SUCCESS(
        MPI_Ibcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD, &request));
    wait_for(&request);
    check_value("MPI_Ibcast", value, 1234, rank);
}

static void test_gather_family(int rank, int size) {
    MPI_Request request = MPI_REQUEST_NULL;
    int send = rank;
    int received = -1;
    int *buffer = (int *)calloc((size_t)size, sizeof(*buffer));
    int *counts = (int *)calloc((size_t)size, sizeof(*counts));
    int *displacements =
        (int *)calloc((size_t)size, sizeof(*displacements));

    if (!buffer || !counts || !displacements) {
        fprintf(stderr, "collective buffer allocation failed\n");
        abort();
    }
    for (int i = 0; i < size; i++) {
        counts[i] = 1;
        displacements[i] = i;
    }

    TEST_CHECK_SUCCESS(MPI_Igather(
        &send, 1, MPI_INT, buffer, 1, MPI_INT, 0, MPI_COMM_WORLD,
        &request));
    wait_for(&request);
    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            check_value("MPI_Igather", buffer[i], i, rank);
        }
    }

    for (int i = 0; i < size; i++) {
        buffer[i] = -1;
    }
    TEST_CHECK_SUCCESS(MPI_Igatherv(
        &send, 1, MPI_INT, buffer, counts, displacements, MPI_INT, 0,
        MPI_COMM_WORLD, &request));
    wait_for(&request);
    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            check_value("MPI_Igatherv", buffer[i], i, rank);
        }
    }

    if (rank == 0) {
        for (int i = 0; i < size; i++) {
            buffer[i] = 100 + i;
        }
    }
    TEST_CHECK_SUCCESS(MPI_Iscatter(
        buffer, 1, MPI_INT, &received, 1, MPI_INT, 0, MPI_COMM_WORLD,
        &request));
    wait_for(&request);
    check_value("MPI_Iscatter", received, 100 + rank, rank);

    received = -1;
    TEST_CHECK_SUCCESS(MPI_Iscatterv(
        buffer, counts, displacements, MPI_INT, &received, 1, MPI_INT, 0,
        MPI_COMM_WORLD, &request));
    wait_for(&request);
    check_value("MPI_Iscatterv", received, 100 + rank, rank);

    free(buffer);
    free(counts);
    free(displacements);
}

static void test_allgather_family(int rank, int size, int require_optional) {
    MPI_Request request = MPI_REQUEST_NULL;
    int send = rank + 10;
    int *buffer = (int *)calloc((size_t)size, sizeof(*buffer));
    int *counts = (int *)calloc((size_t)size, sizeof(*counts));
    int *displacements =
        (int *)calloc((size_t)size, sizeof(*displacements));

    if (!buffer || !counts || !displacements) {
        fprintf(stderr, "allgather buffer allocation failed\n");
        abort();
    }
    for (int i = 0; i < size; i++) {
        counts[i] = 1;
        displacements[i] = i;
    }

    TEST_CHECK_SUCCESS(MPI_Iallgather(
        &send, 1, MPI_INT, buffer, 1, MPI_INT, MPI_COMM_WORLD, &request));
    wait_for(&request);
    for (int i = 0; i < size; i++) {
        check_value("MPI_Iallgather", buffer[i], 10 + i, rank);
        buffer[i] = -1;
    }

    if (use_optional("MPI_Iallgatherv", unimpi.iallgatherv != NULL,
                     require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Iallgatherv(
            &send, 1, MPI_INT, buffer, counts, displacements, MPI_INT,
            MPI_COMM_WORLD, &request));
        wait_for(&request);
        for (int i = 0; i < size; i++) {
            check_value("MPI_Iallgatherv", buffer[i], 10 + i, rank);
        }
    }

    free(buffer);
    free(counts);
    free(displacements);
}

static void test_alltoall_family(int rank, int size, int require_optional,
                                 int datatype_arrays_compatible) {
    MPI_Request request = MPI_REQUEST_NULL;
    int *send = (int *)calloc((size_t)size, sizeof(*send));
    int *receive = (int *)calloc((size_t)size, sizeof(*receive));
    int *counts = (int *)calloc((size_t)size, sizeof(*counts));
    int *displacements =
        (int *)calloc((size_t)size, sizeof(*displacements));
    int *byte_displacements =
        (int *)calloc((size_t)size, sizeof(*byte_displacements));
    MPI_Datatype *types =
        (MPI_Datatype *)calloc((size_t)size, sizeof(*types));

    if (!send || !receive || !counts || !displacements ||
        !byte_displacements || !types) {
        fprintf(stderr, "alltoall buffer allocation failed\n");
        abort();
    }
    for (int i = 0; i < size; i++) {
        send[i] = rank * 100 + i;
        counts[i] = 1;
        displacements[i] = i;
        byte_displacements[i] = i * (int)sizeof(int);
        types[i] = MPI_INT;
    }

    if (use_optional("MPI_Ialltoall", unimpi.ialltoall != NULL,
                     require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Ialltoall(
            send, 1, MPI_INT, receive, 1, MPI_INT, MPI_COMM_WORLD,
            &request));
        wait_for(&request);
        for (int i = 0; i < size; i++) {
            check_value("MPI_Ialltoall", receive[i],
                        i * 100 + rank, rank);
            receive[i] = -1;
        }
    }

    if (use_optional("MPI_Ialltoallv", unimpi.ialltoallv != NULL,
                     require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Ialltoallv(
            send, counts, displacements, MPI_INT, receive, counts,
            displacements, MPI_INT, MPI_COMM_WORLD, &request));
        wait_for(&request);
        for (int i = 0; i < size; i++) {
            check_value("MPI_Ialltoallv", receive[i],
                        i * 100 + rank, rank);
            receive[i] = -1;
        }
    }

    if (!datatype_arrays_compatible) {
        if (rank == 0) {
            printf("SKIP: MPI_Ialltoallw requires a typed datatype-array "
                   "adapter for this backend ABI\n");
        }
    } else if (use_optional(
                   "MPI_Ialltoallw", unimpi.ialltoallw != NULL,
                   require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Ialltoallw(
            send, counts, byte_displacements, types, receive, counts,
            byte_displacements, types, MPI_COMM_WORLD, &request));
        wait_for(&request);
        for (int i = 0; i < size; i++) {
            check_value("MPI_Ialltoallw", receive[i],
                        i * 100 + rank, rank);
        }
    }

    free(send);
    free(receive);
    free(counts);
    free(displacements);
    free(byte_displacements);
    free(types);
}

static void test_reduce_family(int rank, int size, int require_optional) {
    MPI_Request request = MPI_REQUEST_NULL;
    int send = rank + 1;
    int receive = -1;
    int expected_sum = size * (size + 1) / 2;
    int *scatter_send =
        (int *)calloc((size_t)size, sizeof(*scatter_send));
    int *counts = (int *)calloc((size_t)size, sizeof(*counts));

    if (!scatter_send || !counts) {
        fprintf(stderr, "reduce buffer allocation failed\n");
        abort();
    }
    for (int i = 0; i < size; i++) {
        scatter_send[i] = rank + i;
        counts[i] = 1;
    }

    TEST_CHECK_SUCCESS(MPI_Ireduce(
        &send, &receive, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD,
        &request));
    wait_for(&request);
    if (rank == 0) {
        check_value("MPI_Ireduce", receive, expected_sum, rank);
    }

    receive = -1;
    TEST_CHECK_SUCCESS(MPI_Iallreduce(
        &send, &receive, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD, &request));
    wait_for(&request);
    check_value("MPI_Iallreduce", receive, expected_sum, rank);

    if (use_optional("MPI_Ireduce_scatter",
                     unimpi.ireduce_scatter != NULL,
                     require_optional, rank)) {
        receive = -1;
        TEST_CHECK_SUCCESS(MPI_Ireduce_scatter(
            scatter_send, &receive, counts, MPI_INT, MPI_SUM,
            MPI_COMM_WORLD, &request));
        wait_for(&request);
        check_value("MPI_Ireduce_scatter", receive,
                    size * (size - 1) / 2 + size * rank, rank);
    }

    if (use_optional("MPI_Ireduce_scatter_block",
                     unimpi.ireduce_scatter_block != NULL,
                     require_optional, rank)) {
        receive = -1;
        TEST_CHECK_SUCCESS(MPI_Ireduce_scatter_block(
            scatter_send, &receive, 1, MPI_INT, MPI_SUM,
            MPI_COMM_WORLD, &request));
        wait_for(&request);
        check_value("MPI_Ireduce_scatter_block", receive,
                    size * (size - 1) / 2 + size * rank, rank);
    }

    if (use_optional("MPI_Iscan", unimpi.iscan != NULL,
                     require_optional, rank)) {
        receive = -1;
        TEST_CHECK_SUCCESS(MPI_Iscan(
            &rank, &receive, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD,
            &request));
        wait_for(&request);
        check_value("MPI_Iscan", receive, rank * (rank + 1) / 2, rank);
    }

    if (use_optional("MPI_Iexscan", unimpi.iexscan != NULL,
                     require_optional, rank)) {
        receive = -1;
        TEST_CHECK_SUCCESS(MPI_Iexscan(
            &rank, &receive, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD,
            &request));
        wait_for(&request);
        if (rank > 0) {
            check_value("MPI_Iexscan", receive,
                        rank * (rank - 1) / 2, rank);
        }
    }

    free(scatter_send);
    free(counts);
}

int main(int argc, char **argv) {
    unimpi_backend_type_t backend;
    int datatype_arrays_compatible;
    int rank;
    int size;
    int require_optional;

    TEST_CHECK_SUCCESS(MPI_Init(&argc, &argv));
    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));
    backend = unimpi_get_backend_type();
    require_optional = backend != UNIMPI_BACKEND_MSMPI;
    /* Datatype-array collectives are now adapted for every backend through the
     * in-place datatype-array wrappers; whether the operation actually runs is
     * gated on the resolved symbol inside use_optional() below. */
    datatype_arrays_compatible = 1;

    use_optional("MPI_Ibarrier", unimpi.ibarrier != NULL, 1, rank);
    use_optional("MPI_Ibcast", unimpi.ibcast != NULL, 1, rank);
    use_optional("MPI_Igather", unimpi.igather != NULL, 1, rank);
    use_optional("MPI_Igatherv", unimpi.igatherv != NULL, 1, rank);
    use_optional("MPI_Iscatter", unimpi.iscatter != NULL, 1, rank);
    use_optional("MPI_Iscatterv", unimpi.iscatterv != NULL, 1, rank);
    use_optional("MPI_Iallgather", unimpi.iallgather != NULL, 1, rank);
    use_optional("MPI_Ireduce", unimpi.ireduce != NULL, 1, rank);
    use_optional("MPI_Iallreduce", unimpi.iallreduce != NULL, 1, rank);

    test_barrier();
    test_bcast(rank);
    test_gather_family(rank, size);
    test_allgather_family(rank, size, require_optional);
    test_alltoall_family(
        rank, size, require_optional, datatype_arrays_compatible);
    test_reduce_family(rank, size, require_optional);

    if (rank == 0) {
        if (require_optional) {
            printf("All tested MPI-3 nonblocking collective scenarios "
                   "passed\n");
        } else {
            printf("MS-MPI nonblocking collective capability profile "
                   "passed\n");
        }
    }

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
