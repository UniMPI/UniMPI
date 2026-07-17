/* tests/benchmark/bench_collective.c - Collective communication benchmark */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "unimpi.h"

#define WARMUP_ITERATIONS 10
#define BENCHMARK_ITERATIONS 100
#define MAX_BUF_SIZE (1024 * 1024)

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void benchmark_bcast(int buf_size) {
    int rank, size;
    char *buf = (char*)malloc(buf_size);
    double start, end;
    double time_us;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    memset(buf, rank, buf_size);

    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        unimpi.bcast(buf, buf_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    unimpi.barrier(MPI_COMM_WORLD);

    /* Benchmark */
    start = get_time();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        unimpi.bcast(buf, buf_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    end = get_time();

    time_us = (end - start) * 1e6 / BENCHMARK_ITERATIONS;

    if (rank == 0) {
        printf("  Bcast  %10d bytes: %8.3f us\n", buf_size, time_us);
    }

    free(buf);
}

void benchmark_allreduce(int buf_size) {
    int rank, size;
    int *sendbuf = (int*)malloc(buf_size);
    int *recvbuf = (int*)malloc(buf_size);
    double start, end;
    double time_us;
    int count = buf_size / sizeof(int);

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    for (int i = 0; i < count; i++) {
        sendbuf[i] = rank + i;
        recvbuf[i] = 0;
    }

    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        unimpi.allreduce(sendbuf, recvbuf, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    }

    unimpi.barrier(MPI_COMM_WORLD);

    /* Benchmark */
    start = get_time();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        unimpi.allreduce(sendbuf, recvbuf, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    }
    end = get_time();

    time_us = (end - start) * 1e6 / BENCHMARK_ITERATIONS;

    if (rank == 0) {
        printf("  Allreduce %10d bytes: %8.3f us\n", buf_size, time_us);
    }

    free(sendbuf);
    free(recvbuf);
}

void benchmark_gather(int buf_size) {
    int rank, size;
    char *sendbuf = (char*)malloc(buf_size);
    char *recvbuf = NULL;
    double start, end;
    double time_us;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    memset(sendbuf, rank, buf_size);

    if (rank == 0) {
        recvbuf = (char*)malloc(buf_size * size);
    }

    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        unimpi.gather(sendbuf, buf_size, MPI_CHAR, recvbuf, buf_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    unimpi.barrier(MPI_COMM_WORLD);

    /* Benchmark */
    start = get_time();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        unimpi.gather(sendbuf, buf_size, MPI_CHAR, recvbuf, buf_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    end = get_time();

    time_us = (end - start) * 1e6 / BENCHMARK_ITERATIONS;

    if (rank == 0) {
        printf("  Gather %10d bytes: %8.3f us\n", buf_size, time_us);
        free(recvbuf);
    }

    free(sendbuf);
}

void benchmark_barrier(void) {
    int rank, size;
    double start, end;
    double time_us;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        unimpi.barrier(MPI_COMM_WORLD);
    }

    /* Benchmark */
    start = get_time();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        unimpi.barrier(MPI_COMM_WORLD);
    }
    end = get_time();

    time_us = (end - start) * 1e6 / BENCHMARK_ITERATIONS;

    if (rank == 0) {
        printf("  Barrier: %8.3f us\n", time_us);
    }
}

int main(int argc, char **argv) {
    int ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to initialize TFTK-MPI: %s\n", unimpi_error_string(ret));
        return 1;
    }

    int rank;
    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("=== Collective Communication Benchmark ===\n");
        printf("Backend: %s\n\n", unimpi_get_backend_name());
    }

    int buf_sizes[] = {64, 1024, 16384, 65536, 262144};
    int num_sizes = sizeof(buf_sizes) / sizeof(buf_sizes[0]);

    /* Benchmark Bcast */
    if (rank == 0) {
        printf("Broadcast:\n");
    }
    for (int i = 0; i < num_sizes; i++) {
        benchmark_bcast(buf_sizes[i]);
    }

    /* Benchmark Allreduce */
    if (rank == 0) {
        printf("\nAllreduce:\n");
    }
    for (int i = 0; i < num_sizes; i++) {
        benchmark_allreduce(buf_sizes[i]);
    }

    /* Benchmark Gather */
    if (rank == 0) {
        printf("\nGather:\n");
    }
    for (int i = 0; i < num_sizes; i++) {
        benchmark_gather(buf_sizes[i]);
    }

    /* Benchmark Barrier */
    if (rank == 0) {
        printf("\nBarrier:\n");
    }
    benchmark_barrier();

    if (rank == 0) {
        printf("\nBenchmark complete!\n");
    }

    unimpi_finalize();
    return 0;
}
