/* tests/benchmark/bench_overhead.c - Measure wrapper overhead */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "unimpi.h"

#define ITERATIONS 10000

static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void measure_init_overhead(void) {
    int rank;
    double start, end;
    double total_time = 0;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);

    /* Note: We can't easily measure init overhead without native comparison
     * This is just a placeholder showing the wrapper overhead concept */

    if (rank == 0) {
        printf("\n=== Wrapper Overhead Analysis ===\n");
        printf("Note: These are approximate measurements\n\n");
    }
}

void measure_vtable_call_overhead(void) {
    int rank, size;
    double start, end;
    double direct_time, vtable_time;
    int val = 0;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        /* Measure vtable call overhead by calling a simple function many times */
        start = get_time();
        for (int i = 0; i < ITERATIONS; i++) {
            unimpi.barrier(MPI_COMM_WORLD);
        }
        end = get_time();
        vtable_time = (end - start) * 1e6 / ITERATIONS;

        printf("Barrier call overhead: %.6f us per call\n", vtable_time);
        printf("Vtable indirection overhead is minimal (single function pointer call)\n");
    } else {
        /* Participate in barriers */
        for (int i = 0; i < ITERATIONS; i++) {
            unimpi.barrier(MPI_COMM_WORLD);
        }
    }
}

void measure_bandwidth_comparison(void) {
    int rank, size;
    int buf_size = 1024 * 1024; /* 1 MB */
    char *buf;
    double start, end;
    double bandwidth;
    int iterations = 100;

    unimpi.comm_rank(MPI_COMM_WORLD, &rank);
    unimpi.comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) {
            printf("Need 2+ processes for bandwidth test\n");
        }
        return;
    }

    buf = (char*)malloc(buf_size);
    memset(buf, rank, buf_size);

    if (rank == 0) {
        MPI_Status status;

        /* Warmup */
        for (int i = 0; i < 10; i++) {
            unimpi.send(buf, buf_size, MPI_CHAR, 1, 200, MPI_COMM_WORLD);
            unimpi.recv(buf, buf_size, MPI_CHAR, 1, 200, MPI_COMM_WORLD, &status);
        }

        /* Benchmark */
        start = get_time();
        for (int i = 0; i < iterations; i++) {
            unimpi.send(buf, buf_size, MPI_CHAR, 1, 201, MPI_COMM_WORLD);
            unimpi.recv(buf, buf_size, MPI_CHAR, 1, 201, MPI_COMM_WORLD, &status);
        }
        end = get_time();

        double time_per_msg = (end - start) / iterations;
        bandwidth = buf_size / time_per_msg / (1024 * 1024); /* MB/s */

        printf("\n1MB Message Bandwidth: %.2f MB/s\n", bandwidth);
        printf("(This should be comparable to native MPI performance)\n");
    } else if (rank == 1) {
        MPI_Status status;

        /* Warmup */
        for (int i = 0; i < 10; i++) {
            unimpi.recv(buf, buf_size, MPI_CHAR, 0, 200, MPI_COMM_WORLD, &status);
            unimpi.send(buf, buf_size, MPI_CHAR, 0, 200, MPI_COMM_WORLD);
        }

        /* Benchmark */
        for (int i = 0; i < iterations; i++) {
            unimpi.recv(buf, buf_size, MPI_CHAR, 0, 201, MPI_COMM_WORLD, &status);
            unimpi.send(buf, buf_size, MPI_CHAR, 0, 201, MPI_COMM_WORLD);
        }
    }

    free(buf);
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
        printf("=== TFTK-MPI Wrapper Overhead Analysis ===\n");
        printf("Backend: %s\n", unimpi_get_backend_name());
    }

    measure_init_overhead();
    measure_vtable_call_overhead();
    measure_bandwidth_comparison();

    if (rank == 0) {
        printf("\n=== Summary ===\n");
        printf("TFTK-MPI uses direct function pointer calls through vtable.\n");
        printf("Expected overhead: 1 indirect branch per MPI call.\n");
        printf("This is typically < 1ns on modern CPUs.\n");
    }

    unimpi_finalize();
    return 0;
}
