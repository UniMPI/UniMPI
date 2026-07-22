/* tests/mpi/test_benchmarks.c - Performance benchmarks */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "unimpi.h"
#include "test_mpi_helpers.h"

#define WARMUP_ITERATIONS 10
#define BENCHMARK_ITERATIONS 100

void benchmark_send_recv_latency(void) {
    int rank, size;
    int buf = 0;
    MPI_Status status;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    if (size < 2) {
        printf("  Skipping latency benchmark (need 2+ processes)\n");
        return;
    }

    if (rank == 0) {
        /* Warmup */
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            MPI_Send(&buf, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
            MPI_Recv(&buf, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
        }

        /* Benchmark */
        double start = MPI_Wtime();
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            MPI_Send(&buf, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
            MPI_Recv(&buf, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
        }
        double end = MPI_Wtime();

        double latency_us = ((end - start) * 1e6) / (2 * BENCHMARK_ITERATIONS);
        printf("  Send/recv latency: %.2f us\n", latency_us);
    } else if (rank == 1) {
        /* Warmup */
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            MPI_Recv(&buf, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            MPI_Send(&buf, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        }

        /* Benchmark */
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            MPI_Recv(&buf, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            MPI_Send(&buf, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        }
    }
}

void benchmark_bcast(void) {
    int rank, size;
    int buf[1024]; /* 4KB message */

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        MPI_Bcast(buf, 1024, MPI_INT, 0, MPI_COMM_WORLD);
    }

    /* Benchmark */
    double start = MPI_Wtime();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        MPI_Bcast(buf, 1024, MPI_INT, 0, MPI_COMM_WORLD);
    }
    double end = MPI_Wtime();

    if (rank == 0) {
        double time_ms = (end - start) * 1000 / BENCHMARK_ITERATIONS;
        printf("  Bcast 4KB time: %.3f ms\n", time_ms);
    }
}

void benchmark_reduce(void) {
    int rank, size;

    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    TEST_CHECK_SUCCESS(MPI_Comm_size(MPI_COMM_WORLD, &size));

    int send_val = rank;
    int recv_val = 0;

    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        MPI_Reduce(&send_val, &recv_val, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    /* Benchmark */
    double start = MPI_Wtime();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        MPI_Reduce(&send_val, &recv_val, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    double end = MPI_Wtime();

    if (rank == 0) {
        double time_ms = (end - start) * 1000 / BENCHMARK_ITERATIONS;
        printf("  Reduce time: %.3f ms\n", time_ms);
    }
}

int main(int argc, char **argv) {
    int ret = MPI_Init(&argc, &argv);
    const char *backend_name;

    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: error code %d\n", ret);
        return 1;
    }

    printf("Running benchmarks...\n");
    backend_name = unimpi_get_backend_name();
    assert(backend_name != NULL);
    printf("Using backend: %s\n", backend_name);

    benchmark_send_recv_latency();
    benchmark_bcast();
    benchmark_reduce();

    printf("Benchmarks complete!\n");

    TEST_CHECK_SUCCESS(MPI_Finalize());
    return 0;
}
