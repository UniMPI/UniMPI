/* tests/benchmark/bench_collective.c - MPI collective benchmark */
#define UNIMPI_USE_STD_NAMES
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"

#define DEFAULT_WARMUP_ITERATIONS 10
#define DEFAULT_BENCHMARK_ITERATIONS 100
#define SMOKE_WARMUP_ITERATIONS 1
#define SMOKE_BENCHMARK_ITERATIONS 3

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

static int allocation_succeeded(int local_success, int rank,
                                const char *operation) {
    int global_success = 0;

    if (require_mpi_success(
            MPI_Allreduce(&local_success, &global_success, 1, MPI_INT, MPI_MIN,
                          MPI_COMM_WORLD),
            "MPI_Allreduce(allocation status)") != 0) {
        return -1;
    }
    if (!global_success) {
        if (rank == 0) {
            fprintf(stderr, "%s allocation failed\n", operation);
        }
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }
    return 0;
}

static int validate_collective_result(int local_valid, int rank,
                                      const char *operation) {
    int global_valid = 0;

    if (require_mpi_success(
            MPI_Allreduce(&local_valid, &global_valid, 1, MPI_INT, MPI_MIN,
                          MPI_COMM_WORLD),
            "MPI_Allreduce(result validation)") != 0) {
        return -1;
    }
    if (!global_valid) {
        if (rank == 0) {
            fprintf(stderr, "%s result validation failed\n", operation);
        }
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }
    return 0;
}

static int reduce_max_time(double local_time_us, double *maximum_time_us) {
    return require_mpi_success(
        MPI_Reduce(&local_time_us, maximum_time_us, 1, MPI_DOUBLE, MPI_MAX, 0,
                   MPI_COMM_WORLD),
        "MPI_Reduce(maximum elapsed time)");
}

static int benchmark_bcast(int buffer_size,
                           const benchmark_config_t *config,
                           int rank,
                           double *maximum_time_us) {
    unsigned char *buffer;
    double start;
    double end;
    double local_time_us;
    int i;
    int local_valid = 1;

    buffer = (unsigned char *)malloc((size_t)buffer_size);
    if (allocation_succeeded(buffer != NULL, rank, "broadcast buffer") != 0) {
        free(buffer);
        return -1;
    }

    if (rank == 0) {
        memset(buffer, 0x5a, (size_t)buffer_size);
    } else {
        memset(buffer, 0, (size_t)buffer_size);
    }

    for (i = 0; i < config->warmup_iterations; ++i) {
        if (require_mpi_success(
                MPI_Bcast(buffer, buffer_size, MPI_CHAR, 0, MPI_COMM_WORLD),
                "MPI_Bcast(warmup)") != 0) {
            free(buffer);
            return -1;
        }
    }
    if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                            "MPI_Barrier(before broadcast)") != 0) {
        free(buffer);
        return -1;
    }

    start = MPI_Wtime();
    for (i = 0; i < config->benchmark_iterations; ++i) {
        if (require_mpi_success(
                MPI_Bcast(buffer, buffer_size, MPI_CHAR, 0, MPI_COMM_WORLD),
                "MPI_Bcast(measurement)") != 0) {
            free(buffer);
            return -1;
        }
    }
    end = MPI_Wtime();
    local_time_us =
        (end - start) * 1.0e6 / (double)config->benchmark_iterations;

    for (i = 0; i < buffer_size; ++i) {
        if (buffer[i] != 0x5aU) {
            local_valid = 0;
            break;
        }
    }
    if (validate_collective_result(local_valid, rank, "MPI_Bcast") != 0 ||
        reduce_max_time(local_time_us, maximum_time_us) != 0) {
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

static int benchmark_allreduce(int buffer_size,
                               const benchmark_config_t *config,
                               int rank,
                               int process_count,
                               double *maximum_time_us) {
    int count = buffer_size / (int)sizeof(int);
    int expected = process_count * (process_count + 1) / 2;
    int *send_buffer;
    int *receive_buffer;
    double start;
    double end;
    double local_time_us;
    int i;
    int local_valid = 1;
    int local_allocated;

    send_buffer = (int *)malloc((size_t)count * sizeof(*send_buffer));
    receive_buffer = (int *)malloc((size_t)count * sizeof(*receive_buffer));
    local_allocated = send_buffer != NULL && receive_buffer != NULL;
    if (allocation_succeeded(local_allocated, rank, "allreduce buffers") != 0) {
        free(send_buffer);
        free(receive_buffer);
        return -1;
    }

    for (i = 0; i < count; ++i) {
        send_buffer[i] = rank + 1;
        receive_buffer[i] = 0;
    }

    for (i = 0; i < config->warmup_iterations; ++i) {
        if (require_mpi_success(
                MPI_Allreduce(send_buffer, receive_buffer, count, MPI_INT,
                              MPI_SUM, MPI_COMM_WORLD),
                "MPI_Allreduce(warmup)") != 0) {
            free(send_buffer);
            free(receive_buffer);
            return -1;
        }
    }
    if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                            "MPI_Barrier(before allreduce)") != 0) {
        free(send_buffer);
        free(receive_buffer);
        return -1;
    }

    start = MPI_Wtime();
    for (i = 0; i < config->benchmark_iterations; ++i) {
        if (require_mpi_success(
                MPI_Allreduce(send_buffer, receive_buffer, count, MPI_INT,
                              MPI_SUM, MPI_COMM_WORLD),
                "MPI_Allreduce(measurement)") != 0) {
            free(send_buffer);
            free(receive_buffer);
            return -1;
        }
    }
    end = MPI_Wtime();
    local_time_us =
        (end - start) * 1.0e6 / (double)config->benchmark_iterations;

    for (i = 0; i < count; ++i) {
        if (receive_buffer[i] != expected) {
            local_valid = 0;
            break;
        }
    }
    if (validate_collective_result(local_valid, rank, "MPI_Allreduce") != 0 ||
        reduce_max_time(local_time_us, maximum_time_us) != 0) {
        free(send_buffer);
        free(receive_buffer);
        return -1;
    }

    free(send_buffer);
    free(receive_buffer);
    return 0;
}

static int benchmark_gather(int buffer_size,
                            const benchmark_config_t *config,
                            int rank,
                            int process_count,
                            double *maximum_time_us) {
    unsigned char *send_buffer;
    unsigned char *receive_buffer = NULL;
    size_t receive_size = 0;
    double start;
    double end;
    double local_time_us;
    int i;
    int source_rank;
    int local_valid = 1;
    int local_allocated;

    send_buffer = (unsigned char *)malloc((size_t)buffer_size);
    if (rank == 0) {
        if ((size_t)process_count > (size_t)-1 / (size_t)buffer_size) {
            fprintf(stderr, "gather receive buffer size overflow\n");
            free(send_buffer);
            (void)unimpi.abort(MPI_COMM_WORLD, 1);
            return -1;
        }
        receive_size = (size_t)buffer_size * (size_t)process_count;
        receive_buffer = (unsigned char *)malloc(receive_size);
    }
    local_allocated =
        send_buffer != NULL && (rank != 0 || receive_buffer != NULL);
    if (allocation_succeeded(local_allocated, rank, "gather buffers") != 0) {
        free(send_buffer);
        free(receive_buffer);
        return -1;
    }

    memset(send_buffer, (unsigned char)(rank % 251), (size_t)buffer_size);

    for (i = 0; i < config->warmup_iterations; ++i) {
        if (require_mpi_success(
                MPI_Gather(send_buffer, buffer_size, MPI_CHAR, receive_buffer,
                           buffer_size, MPI_CHAR, 0, MPI_COMM_WORLD),
                "MPI_Gather(warmup)") != 0) {
            free(send_buffer);
            free(receive_buffer);
            return -1;
        }
    }
    if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                            "MPI_Barrier(before gather)") != 0) {
        free(send_buffer);
        free(receive_buffer);
        return -1;
    }

    start = MPI_Wtime();
    for (i = 0; i < config->benchmark_iterations; ++i) {
        if (require_mpi_success(
                MPI_Gather(send_buffer, buffer_size, MPI_CHAR, receive_buffer,
                           buffer_size, MPI_CHAR, 0, MPI_COMM_WORLD),
                "MPI_Gather(measurement)") != 0) {
            free(send_buffer);
            free(receive_buffer);
            return -1;
        }
    }
    end = MPI_Wtime();
    local_time_us =
        (end - start) * 1.0e6 / (double)config->benchmark_iterations;

    if (rank == 0) {
        for (source_rank = 0; source_rank < process_count; ++source_rank) {
            for (i = 0; i < buffer_size; ++i) {
                size_t index =
                    (size_t)source_rank * (size_t)buffer_size + (size_t)i;
                if (receive_buffer[index] !=
                    (unsigned char)(source_rank % 251)) {
                    local_valid = 0;
                    break;
                }
            }
            if (!local_valid) {
                break;
            }
        }
    }

    if (validate_collective_result(local_valid, rank, "MPI_Gather") != 0 ||
        reduce_max_time(local_time_us, maximum_time_us) != 0) {
        free(send_buffer);
        free(receive_buffer);
        return -1;
    }

    free(send_buffer);
    free(receive_buffer);
    return 0;
}

static int benchmark_alltoallw(int buffer_size,
                               const benchmark_config_t *config,
                               int rank,
                               int process_count,
                               double *maximum_time_us) {
    size_t total_size = (size_t)buffer_size * (size_t)process_count;
    unsigned char *send_buffer = (unsigned char *)malloc(total_size);
    unsigned char *receive_buffer = (unsigned char *)malloc(total_size);
    int *counts = (int *)malloc(
        (size_t)process_count * sizeof(*counts));
    int *displacements = (int *)malloc(
        (size_t)process_count * sizeof(*displacements));
    MPI_Datatype *types = (MPI_Datatype *)malloc(
        (size_t)process_count * sizeof(*types));
    double start;
    double end;
    double local_time_us;
    int local_allocated;
    int local_valid = 1;
    int i;

    local_allocated = send_buffer && receive_buffer && counts &&
                      displacements && types;
    if (allocation_succeeded(
            local_allocated, rank, "alltoallw buffers") != 0) {
        free(types);
        free(displacements);
        free(counts);
        free(receive_buffer);
        free(send_buffer);
        return -1;
    }
    memset(send_buffer, rank + 1, total_size);
    memset(receive_buffer, 0, total_size);
    for (i = 0; i < process_count; ++i) {
        counts[i] = buffer_size;
        displacements[i] = i * buffer_size;
        types[i] = MPI_CHAR;
    }

    for (i = 0; i < config->warmup_iterations; ++i) {
        if (require_mpi_success(MPI_Alltoallw(
                send_buffer, counts, displacements, types,
                receive_buffer, counts, displacements, types,
                MPI_COMM_WORLD), "MPI_Alltoallw(warmup)") != 0) {
            local_valid = 0;
            break;
        }
    }
    if (local_valid &&
        require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                            "MPI_Barrier(before alltoallw)") != 0) {
        local_valid = 0;
    }
    start = MPI_Wtime();
    for (i = 0; local_valid && i < config->benchmark_iterations; ++i) {
        if (require_mpi_success(MPI_Alltoallw(
                send_buffer, counts, displacements, types,
                receive_buffer, counts, displacements, types,
                MPI_COMM_WORLD), "MPI_Alltoallw(measurement)") != 0) {
            local_valid = 0;
        }
    }
    end = MPI_Wtime();
    local_time_us =
        (end - start) * 1.0e6 / (double)config->benchmark_iterations;

    for (i = 0; local_valid && i < process_count; ++i) {
        size_t offset = (size_t)i * (size_t)buffer_size;
        int j;

        for (j = 0; j < buffer_size; ++j) {
            if (receive_buffer[offset + (size_t)j] !=
                (unsigned char)(i + 1)) {
                local_valid = 0;
                break;
            }
        }
    }
    if (validate_collective_result(
            local_valid, rank, "MPI_Alltoallw") != 0 ||
        reduce_max_time(local_time_us, maximum_time_us) != 0) {
        local_valid = 0;
    }

    free(types);
    free(displacements);
    free(counts);
    free(receive_buffer);
    free(send_buffer);
    return local_valid ? 0 : -1;
}

static int benchmark_barrier(const benchmark_config_t *config,
                             double *maximum_time_us) {
    double start;
    double end;
    double local_time_us;
    int i;

    for (i = 0; i < config->warmup_iterations; ++i) {
        if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                                "MPI_Barrier(warmup)") != 0) {
            return -1;
        }
    }

    start = MPI_Wtime();
    for (i = 0; i < config->benchmark_iterations; ++i) {
        if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                                "MPI_Barrier(measurement)") != 0) {
            return -1;
        }
    }
    end = MPI_Wtime();
    local_time_us =
        (end - start) * 1.0e6 / (double)config->benchmark_iterations;

    return reduce_max_time(local_time_us, maximum_time_us);
}

static void print_result(output_format_t format,
                         const char *backend_name,
                         int process_count,
                         const benchmark_config_t *config,
                         const char *operation,
                         int buffer_size,
                         double maximum_time_us) {
    if (format == OUTPUT_CSV) {
        printf("%s,%d,%s,%d,%d,%d,%.9f\n",
               backend_name, process_count, operation, buffer_size,
               config->warmup_iterations, config->benchmark_iterations,
               maximum_time_us);
    } else if (buffer_size > 0) {
        printf("%-10s %12d bytes: %12.6f us\n",
               operation, buffer_size, maximum_time_us);
    } else {
        printf("%-10s %12s        %12.6f us\n",
               operation, "-", maximum_time_us);
    }
}

int main(int argc, char **argv) {
    static const int full_buffer_sizes[] = {
        64, 1024, 16384, 65536, 262144
    };
    static const int smoke_buffer_sizes[] = {64, 4096};
    const int *buffer_sizes;
    int buffer_size_count;
    benchmark_config_t config;
    const char *backend_name;
    int parse_result;
    int ret;
    int rank;
    int process_count;
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
        require_mpi_success(MPI_Comm_size(MPI_COMM_WORLD, &process_count),
                            "MPI_Comm_size") != 0) {
        return 1;
    }

    backend_name = unimpi_get_backend_name();
    if (!backend_name) {
        backend_name = "unknown";
    }

    if (config.smoke) {
        buffer_sizes = smoke_buffer_sizes;
        buffer_size_count =
            (int)(sizeof(smoke_buffer_sizes) / sizeof(smoke_buffer_sizes[0]));
    } else {
        buffer_sizes = full_buffer_sizes;
        buffer_size_count =
            (int)(sizeof(full_buffer_sizes) / sizeof(full_buffer_sizes[0]));
    }

    if (rank == 0) {
        if (config.format == OUTPUT_CSV) {
            printf("backend,processes,operation,size_bytes,warmup,"
                   "iterations,max_time_us\n");
        } else {
            printf("=== UniMPI Collective Benchmark ===\n");
            printf("Backend: %s, processes: %d, warmup: %d, iterations: %d\n",
                   backend_name, process_count, config.warmup_iterations,
                   config.benchmark_iterations);
            printf("Times are the maximum per-call time across all ranks.\n");
        }
    }

    for (i = 0; i < buffer_size_count; ++i) {
        double maximum_time_us = 0.0;

        if (benchmark_bcast(buffer_sizes[i], &config, rank,
                            &maximum_time_us) != 0) {
            return 1;
        }
        if (rank == 0) {
            print_result(config.format, backend_name, process_count, &config,
                         "bcast", buffer_sizes[i], maximum_time_us);
        }

        maximum_time_us = 0.0;
        if (benchmark_allreduce(buffer_sizes[i], &config, rank, process_count,
                                &maximum_time_us) != 0) {
            return 1;
        }
        if (rank == 0) {
            print_result(config.format, backend_name, process_count, &config,
                         "allreduce", buffer_sizes[i], maximum_time_us);
        }

        maximum_time_us = 0.0;
        if (benchmark_gather(buffer_sizes[i], &config, rank, process_count,
                             &maximum_time_us) != 0) {
            return 1;
        }
        if (rank == 0) {
            print_result(config.format, backend_name, process_count, &config,
                         "gather", buffer_sizes[i], maximum_time_us);
        }

        maximum_time_us = 0.0;
        if (benchmark_alltoallw(
                buffer_sizes[i], &config, rank, process_count,
                &maximum_time_us) != 0) {
            return 1;
        }
        if (rank == 0) {
            print_result(config.format, backend_name, process_count, &config,
                         "alltoallw", buffer_sizes[i], maximum_time_us);
        }
    }

    {
        double maximum_time_us = 0.0;
        if (benchmark_barrier(&config, &maximum_time_us) != 0) {
            return 1;
        }
        if (rank == 0) {
            print_result(config.format, backend_name, process_count, &config,
                         "barrier", 0, maximum_time_us);
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
