/* tests/mpi/test_collective_extended.c - Extended collective communication tests */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

void test_alltoall(void) {
    int rank, size;
    int *send_buf, *recv_buf;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    send_buf = (int*)malloc(size * sizeof(int));
    recv_buf = (int*)malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        send_buf[i] = rank * 100 + i;
    }

    TEST_CHECK_SUCCESS(MPI_Alltoall(send_buf, 1, MPI_INT, recv_buf, 1, MPI_INT, MPI_COMM_WORLD));

    /* Verify: rank r receives from rank i: i*100 + r */
    for (int i = 0; i < size; i++) {
        assert(recv_buf[i] == i * 100 + rank);
    }

    free(send_buf);
    free(recv_buf);

    if (rank == 0) printf("  Alltoall test passed\n");
}

void test_alltoallw(void) {
    int rank, size;
    int *send_buf = NULL;
    int *recv_buf = NULL;
    int *counts = NULL;
    int *displacements = NULL;
    MPI_Datatype *types = NULL;
    int local_ready;
    int all_ready;
    int local_ok;
    int all_ok;
    int i;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));
    send_buf = (int*)malloc((size_t)size * sizeof(*send_buf));
    recv_buf = (int*)malloc((size_t)size * sizeof(*recv_buf));
    counts = (int*)malloc((size_t)size * sizeof(*counts));
    displacements = (int*)malloc((size_t)size * sizeof(*displacements));
    types = (MPI_Datatype*)malloc((size_t)size * sizeof(*types));

    /* Coordinate allocation failure so one rank cannot exit while peers
     * continue into later collectives. */
    local_ready = (send_buf && recv_buf && counts && displacements && types)
        ? 1 : 0;
    TEST_CHECK_SUCCESS(MPI_Allreduce(
        &local_ready, &all_ready, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD));
    if (!all_ready) {
        free(types);
        free(displacements);
        free(counts);
        free(recv_buf);
        free(send_buf);
        if (rank == 0) {
            fprintf(stderr, "Alltoallw allocation failed on at least one rank\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
        return;
    }

    for (i = 0; i < size; i++) {
        send_buf[i] = rank * 100 + i;
        counts[i] = 1;
        displacements[i] = i * (int)sizeof(int);
        types[i] = MPI_INT;
    }
    TEST_CHECK_SUCCESS(MPI_Alltoallw(
        send_buf, counts, displacements, types,
        recv_buf, counts, displacements, types, MPI_COMM_WORLD));

    local_ok = 1;
    for (i = 0; i < size; i++) {
        if (recv_buf[i] != i * 100 + rank) {
            local_ok = 0;
            break;
        }
    }
    TEST_CHECK_SUCCESS(MPI_Allreduce(
        &local_ok, &all_ok, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD));

    free(types);
    free(displacements);
    free(counts);
    free(recv_buf);
    free(send_buf);

    if (!all_ok) {
        if (rank == 0) {
            fprintf(stderr, "Alltoallw result validation failed\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
        return;
    }
    if (rank == 0) {
        printf("  Alltoallw test passed\n");
    }
}

void test_scan(void) {
    int rank, size;
    int send_val, recv_val;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    send_val = rank + 1;

    TEST_CHECK_SUCCESS(MPI_Scan(&send_val, &recv_val, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD));

    /* Scan result: sum of ranks 0..rank = (rank+1)*(rank+2)/2 */
    int expected = (rank + 1) * (rank + 2) / 2;
    assert(recv_val == expected);

    if (rank == 0) printf("  Scan test passed\n");
}

void test_reduce_scatter(void) {
    int rank, size;
    int *send_buf, *recv_buf;
    int *recv_counts;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    send_buf = (int*)malloc(size * sizeof(int));
    recv_buf = (int*)malloc(sizeof(int));
    recv_counts = (int*)malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        send_buf[i] = rank;
        recv_counts[i] = 1;
    }

    TEST_CHECK_SUCCESS(MPI_Reduce_scatter(send_buf, recv_buf, recv_counts, MPI_INT, MPI_SUM, MPI_COMM_WORLD));

    /* Each rank receives sum of all ranks' values at that position */
    int expected = 0;
    for (int i = 0; i < size; i++) {
        expected += i;
    }
    assert(recv_buf[0] == expected);

    free(send_buf);
    free(recv_buf);
    free(recv_counts);

    if (rank == 0) printf("  Reduce_scatter test passed\n");
}

void test_gatherv(void) {
    int rank, size;
    int *send_buf, *recv_buf = NULL;
    int *recv_counts = NULL, *displs = NULL;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    /* Each rank sends (rank+1) elements */
    int send_count = rank + 1;
    send_buf = (int*)malloc(send_count * sizeof(int));
    for (int i = 0; i < send_count; i++) {
        send_buf[i] = rank;
    }

    if (rank == 0) {
        recv_buf = (int*)malloc(size * (size + 1) / 2 * sizeof(int));
        recv_counts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));
        int disp = 0;
        for (int i = 0; i < size; i++) {
            recv_counts[i] = i + 1;
            displs[i] = disp;
            disp += i + 1;
        }
    }

    TEST_CHECK_SUCCESS(MPI_Gatherv(send_buf, send_count, MPI_INT, recv_buf, recv_counts, displs, MPI_INT, 0, MPI_COMM_WORLD));

    if (rank == 0) {
        /* Verify: at displacement i, we have i+1 copies of value i */
        int pos = 0;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < i + 1; j++) {
                assert(recv_buf[pos++] == i);
            }
        }
        free(recv_buf);
        free(recv_counts);
        free(displs);
    }

    free(send_buf);

    if (rank == 0) printf("  Gatherv test passed\n");
}

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    const char *backend_name;

    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: error code %d\n", ret);
        return 1;
    }

    printf("Running extended collective tests...\n");
    backend_name = unimpi_get_backend_name();
    assert(backend_name != NULL);
    printf("Using backend: %s\n", backend_name);

    test_alltoall();
    test_alltoallw();
    test_scan();
    test_reduce_scatter();
    test_gatherv();

    printf("All extended collective tests passed!\n");

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
