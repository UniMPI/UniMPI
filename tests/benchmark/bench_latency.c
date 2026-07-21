/* tests/benchmark/bench_latency.c - MPI Latency benchmark */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"

#define WARMUP_ITERATIONS 100
#define BENCHMARK_ITERATIONS 1000
#define MAX_MESSAGE_SIZE (1024 * 1024)

static double get_time(void) {
    return unimpi.wtime();
}

void benchmark_ping_pong(int message_size) {
    int rank, size;
    char *buf = (char*)malloc(message_size);
    MPI_Status status;
    double start, end;
    double latency;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            printf("  Skipping (need 2+ processes)\n");
        }
        free(buf);
        return;
    }

    memset(buf, rank, message_size);

    /* Warmup */
    if (rank == 0) {
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            unimpi.send(buf, message_size, MPI_CHAR, 1, 100, MPI_COMM_WORLD);
            unimpi.recv(buf, message_size, MPI_CHAR, 1, 100, MPI_COMM_WORLD, &status);
        }
    } else if (rank == 1) {
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            unimpi.recv(buf, message_size, MPI_CHAR, 0, 100, MPI_COMM_WORLD, &status);
            unimpi.send(buf, message_size, MPI_CHAR, 0, 100, MPI_COMM_WORLD);
        }
    }

    unimpi.barrier(MPI_COMM_WORLD);

    /* Benchmark */
    if (rank == 0) {
        start = get_time();
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            unimpi.send(buf, message_size, MPI_CHAR, 1, 101, MPI_COMM_WORLD);
            unimpi.recv(buf, message_size, MPI_CHAR, 1, 101, MPI_COMM_WORLD, &status);
        }
        end = get_time();

        /* Latency = total time / (2 * iterations) in microseconds */
        latency = (end - start) * 1e6 / (2 * BENCHMARK_ITERATIONS);
        printf("  %10d bytes: %8.3f us latency, %8.3f MB/s bandwidth\n",
               message_size, latency, message_size / (latency * 1e-6) / 1e6);
    } else if (rank == 1) {
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            unimpi.recv(buf, message_size, MPI_CHAR, 0, 101, MPI_COMM_WORLD, &status);
            unimpi.send(buf, message_size, MPI_CHAR, 0, 101, MPI_COMM_WORLD);
        }
    }

    free(buf);
}

void benchmark_throughput(void) {
    int rank, size;
    int message_sizes[] = {1, 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576};
    int num_sizes = sizeof(message_sizes) / sizeof(message_sizes[0]);

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("\n=== Point-to-Point Latency Benchmark ===\n");
        printf("Backend: %s\n", unimpi_get_backend_name());
        printf("%10s %12s %15s\n", "Size", "Latency(us)", "Bandwidth(MB/s)");
    }

    for (int i = 0; i < num_sizes; i++) {
        benchmark_ping_pong(message_sizes[i]);
    }
}

int main(int argc, char **argv) {
    int ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to initialize TFTK-MPI: %s\n", unimpi_error_string(ret));
        return 1;
    }

    benchmark_throughput();

    if (unimpi.comm_rank(MPI_COMM_WORLD, &(int){0}) == 0) {
        printf("\nBenchmark complete!\n");
    }

    unimpi_finalize();
    return 0;
}
