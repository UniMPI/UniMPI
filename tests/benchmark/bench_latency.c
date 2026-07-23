/* tests/benchmark/bench_latency.c - MPI ping-pong latency benchmark */
#define UNIMPI_USE_STD_NAMES
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"

#define DEFAULT_WARMUP_ITERATIONS 100
#define DEFAULT_BENCHMARK_ITERATIONS 1000
#define SMOKE_WARMUP_ITERATIONS 2
#define SMOKE_BENCHMARK_ITERATIONS 10

typedef enum {
    OUTPUT_TEXT = 0,
    OUTPUT_CSV
} output_format_t;

typedef struct {
    int warmup_iterations;
    int benchmark_iterations;
    int smoke;
    int warmup_was_set;
    int iterations_were_set;
    output_format_t format;
} benchmark_config_t;

static void print_usage(const char *program) {
    printf("Usage: %s [--smoke] [--warmup N] [--iterations N] "
           "[--format text|csv]\n",
           program);
}

static int parse_positive_int(const char *text, int *value) {
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed <= 0 || parsed > INT_MAX) {
        return -1;
    }

    *value = (int)parsed;
    return 0;
}

static int parse_arguments(int argc, char **argv, benchmark_config_t *config) {
    int i;

    config->warmup_iterations = DEFAULT_WARMUP_ITERATIONS;
    config->benchmark_iterations = DEFAULT_BENCHMARK_ITERATIONS;
    config->smoke = 0;
    config->warmup_was_set = 0;
    config->iterations_were_set = 0;
    config->format = OUTPUT_TEXT;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--smoke") == 0) {
            config->smoke = 1;
        } else if (strcmp(argv[i], "--warmup") == 0) {
            if (++i >= argc ||
                parse_positive_int(argv[i], &config->warmup_iterations) != 0) {
                fprintf(stderr, "--warmup requires a positive integer\n");
                return -1;
            }
            config->warmup_was_set = 1;
        } else if (strcmp(argv[i], "--iterations") == 0) {
            if (++i >= argc ||
                parse_positive_int(argv[i],
                                   &config->benchmark_iterations) != 0) {
                fprintf(stderr, "--iterations requires a positive integer\n");
                return -1;
            }
            config->iterations_were_set = 1;
        } else if (strcmp(argv[i], "--format") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "--format requires text or csv\n");
                return -1;
            }
            if (strcmp(argv[i], "text") == 0) {
                config->format = OUTPUT_TEXT;
            } else if (strcmp(argv[i], "csv") == 0) {
                config->format = OUTPUT_CSV;
            } else {
                fprintf(stderr, "unsupported output format: %s\n", argv[i]);
                return -1;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            return -1;
        }
    }

    if (config->smoke) {
        if (!config->warmup_was_set) {
            config->warmup_iterations = SMOKE_WARMUP_ITERATIONS;
        }
        if (!config->iterations_were_set) {
            config->benchmark_iterations = SMOKE_BENCHMARK_ITERATIONS;
        }
    }

    return 0;
}

static int require_mpi_success(int code, const char *operation) {
    if (code == MPI_SUCCESS) {
        return 0;
    }

    fprintf(stderr, "%s failed with MPI error %d: %s\n",
            operation, code, unimpi_mpi_error_string(code));
    (void)unimpi.abort(MPI_COMM_WORLD, code);
    return -1;
}

static unsigned char payload_byte(size_t index) {
    return (unsigned char)((index * 131U + 17U) % 251U);
}

static void initialize_payload(unsigned char *buffer, size_t size) {
    size_t i;

    for (i = 0; i < size; ++i) {
        buffer[i] = payload_byte(i);
    }
}

static int payload_is_valid(const unsigned char *buffer, size_t size) {
    size_t i;

    for (i = 0; i < size; ++i) {
        if (buffer[i] != payload_byte(i)) {
            return 0;
        }
    }
    return 1;
}

static int validate_payload(const unsigned char *buffer, size_t size,
                            int rank, const char *phase) {
    int local_valid = 1;
    int globally_valid = 0;

    if (rank == 0 || rank == 1) {
        local_valid = payload_is_valid(buffer, size);
    }

    if (require_mpi_success(
            MPI_Allreduce(&local_valid, &globally_valid, 1, MPI_INT, MPI_MIN,
                          MPI_COMM_WORLD),
            "MPI_Allreduce(payload validation)") != 0) {
        return -1;
    }

    if (!globally_valid) {
        if (rank == 0) {
            fprintf(stderr, "payload validation failed after %s\n", phase);
        }
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    return 0;
}

static int benchmark_ping_pong(int message_size,
                               const benchmark_config_t *config,
                               int rank,
                               double *latency_us,
                               double *bandwidth_mb_s) {
    unsigned char *buffer;
    MPI_Status status;
    double start = 0.0;
    double end = 0.0;
    int i;
    int local_allocated;
    int globally_allocated = 0;

    buffer = (unsigned char *)malloc((size_t)message_size);
    local_allocated = buffer != NULL;
    if (require_mpi_success(
            MPI_Allreduce(&local_allocated, &globally_allocated, 1, MPI_INT,
                          MPI_MIN, MPI_COMM_WORLD),
            "MPI_Allreduce(allocation status)") != 0) {
        free(buffer);
        return -1;
    }
    if (!globally_allocated) {
        if (rank == 0) {
            fprintf(stderr, "failed to allocate %d-byte benchmark buffer\n",
                    message_size);
        }
        free(buffer);
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    if (rank == 0) {
        initialize_payload(buffer, (size_t)message_size);
    } else {
        memset(buffer, 0, (size_t)message_size);
    }

    if (rank == 0) {
        for (i = 0; i < config->warmup_iterations; ++i) {
            if (require_mpi_success(
                    MPI_Send(buffer, message_size, MPI_CHAR, 1, 100,
                             MPI_COMM_WORLD),
                    "MPI_Send(warmup)") != 0 ||
                require_mpi_success(
                    MPI_Recv(buffer, message_size, MPI_CHAR, 1, 100,
                             MPI_COMM_WORLD, &status),
                    "MPI_Recv(warmup)") != 0) {
                free(buffer);
                return -1;
            }
        }
    } else if (rank == 1) {
        for (i = 0; i < config->warmup_iterations; ++i) {
            if (require_mpi_success(
                    MPI_Recv(buffer, message_size, MPI_CHAR, 0, 100,
                             MPI_COMM_WORLD, &status),
                    "MPI_Recv(warmup responder)") != 0 ||
                require_mpi_success(
                    MPI_Send(buffer, message_size, MPI_CHAR, 0, 100,
                             MPI_COMM_WORLD),
                    "MPI_Send(warmup responder)") != 0) {
                free(buffer);
                return -1;
            }
        }
    }

    if (validate_payload(buffer, (size_t)message_size, rank, "warmup") != 0 ||
        require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                            "MPI_Barrier(before measurement)") != 0) {
        free(buffer);
        return -1;
    }

    if (rank == 0) {
        start = MPI_Wtime();
        for (i = 0; i < config->benchmark_iterations; ++i) {
            if (require_mpi_success(
                    MPI_Send(buffer, message_size, MPI_CHAR, 1, 101,
                             MPI_COMM_WORLD),
                    "MPI_Send(measurement)") != 0 ||
                require_mpi_success(
                    MPI_Recv(buffer, message_size, MPI_CHAR, 1, 101,
                             MPI_COMM_WORLD, &status),
                    "MPI_Recv(measurement)") != 0) {
                free(buffer);
                return -1;
            }
        }
        end = MPI_Wtime();
    } else if (rank == 1) {
        for (i = 0; i < config->benchmark_iterations; ++i) {
            if (require_mpi_success(
                    MPI_Recv(buffer, message_size, MPI_CHAR, 0, 101,
                             MPI_COMM_WORLD, &status),
                    "MPI_Recv(measurement responder)") != 0 ||
                require_mpi_success(
                    MPI_Send(buffer, message_size, MPI_CHAR, 0, 101,
                             MPI_COMM_WORLD),
                    "MPI_Send(measurement responder)") != 0) {
                free(buffer);
                return -1;
            }
        }
    }

    if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                            "MPI_Barrier(after measurement)") != 0 ||
        validate_payload(buffer, (size_t)message_size, rank,
                         "measurement") != 0) {
        free(buffer);
        return -1;
    }

    if (rank == 0) {
        *latency_us =
            (end - start) * 1.0e6 /
            (2.0 * (double)config->benchmark_iterations);
        if (*latency_us > 0.0) {
            *bandwidth_mb_s =
                (double)message_size / (*latency_us * 1.0e-6) / 1.0e6;
        } else {
            *bandwidth_mb_s = 0.0;
        }
    }

    free(buffer);
    return 0;
}

int main(int argc, char **argv) {
    static const int full_message_sizes[] = {
        1, 64, 256, 1024, 4096, 16384, 65536, 262144, 1048576
    };
    static const int smoke_message_sizes[] = {1, 1024, 65536};
    const int *message_sizes;
    int message_size_count;
    benchmark_config_t config;
    const char *backend_name;
    int parse_result;
    int ret;
    int rank;
    int size;
    int i;

    parse_result = parse_arguments(argc, argv, &config);
    if (parse_result > 0) {
        return 0;
    }
    if (parse_result < 0) {
        print_usage(argv[0]);
        return 2;
    }

    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "failed to initialize unimpi: %s\n",
                unimpi_error_string(ret));
        return 1;
    }

    if (require_mpi_success(MPI_Comm_rank(MPI_COMM_WORLD, &rank),
                            "MPI_Comm_rank") != 0 ||
        require_mpi_success(MPI_Comm_size(MPI_COMM_WORLD, &size),
                            "MPI_Comm_size") != 0) {
        return 1;
    }

    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr,
                    "bench_latency requires at least two MPI processes\n");
        }
        ret = unimpi_finalize();
        return ret == UNIMPI_OK ? 2 : 1;
    }

    backend_name = unimpi_get_backend_name();
    if (!backend_name) {
        backend_name = "unknown";
    }

    if (config.smoke) {
        message_sizes = smoke_message_sizes;
        message_size_count =
            (int)(sizeof(smoke_message_sizes) /
                  sizeof(smoke_message_sizes[0]));
    } else {
        message_sizes = full_message_sizes;
        message_size_count =
            (int)(sizeof(full_message_sizes) /
                  sizeof(full_message_sizes[0]));
    }

    if (rank == 0) {
        if (config.format == OUTPUT_CSV) {
            printf("backend,processes,size_bytes,warmup,iterations,"
                   "latency_us,bandwidth_mb_s\n");
        } else {
            printf("=== UniMPI Point-to-Point Latency Benchmark ===\n");
            printf("Backend: %s, processes: %d, warmup: %d, iterations: %d\n",
                   backend_name, size, config.warmup_iterations,
                   config.benchmark_iterations);
            printf("%12s %14s %18s\n",
                   "Size(bytes)", "Latency(us)", "Bandwidth(MB/s)");
        }
    }

    for (i = 0; i < message_size_count; ++i) {
        double latency_us = 0.0;
        double bandwidth_mb_s = 0.0;

        if (benchmark_ping_pong(message_sizes[i], &config, rank,
                                &latency_us, &bandwidth_mb_s) != 0) {
            return 1;
        }

        if (rank == 0) {
            if (config.format == OUTPUT_CSV) {
                printf("%s,%d,%d,%d,%d,%.9f,%.9f\n",
                       backend_name, size, message_sizes[i],
                       config.warmup_iterations,
                       config.benchmark_iterations,
                       latency_us, bandwidth_mb_s);
            } else {
                printf("%12d %14.6f %18.3f\n",
                       message_sizes[i], latency_us, bandwidth_mb_s);
            }
        }
    }

    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "failed to finalize unimpi: %s\n",
                unimpi_error_string(ret));
        return 1;
    }

    return 0;
}
