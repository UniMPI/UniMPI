/* tests/benchmark/bench_overhead.c - Paired dispatch timing benchmark */
#define UNIMPI_USE_STD_NAMES
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"

#define DEFAULT_WARMUP_ITERATIONS 10000
#define DEFAULT_BENCHMARK_ITERATIONS 1000000
#define DEFAULT_BATCHES 9
#define SMOKE_WARMUP_ITERATIONS 100
#define SMOKE_BENCHMARK_ITERATIONS 10000
#define SMOKE_BATCHES 3

typedef int (*comm_rank_function_t)(MPI_Comm, int *);

typedef enum {
    OUTPUT_TEXT = 0,
    OUTPUT_CSV
} output_format_t;

typedef struct {
    int warmup_iterations;
    int benchmark_iterations;
    int batches;
    int smoke;
    int warmup_was_set;
    int iterations_were_set;
    int batches_were_set;
    output_format_t format;
} benchmark_config_t;

static void print_usage(const char *program) {
    printf("Usage: %s [--smoke] [--warmup N] [--iterations N] "
           "[--batches N] [--format text|csv]\n",
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
    config->batches = DEFAULT_BATCHES;
    config->smoke = 0;
    config->warmup_was_set = 0;
    config->iterations_were_set = 0;
    config->batches_were_set = 0;
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
        } else if (strcmp(argv[i], "--batches") == 0) {
            if (++i >= argc ||
                parse_positive_int(argv[i], &config->batches) != 0) {
                fprintf(stderr, "--batches requires a positive integer\n");
                return -1;
            }
            config->batches_were_set = 1;
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
        if (!config->batches_were_set) {
            config->batches = SMOKE_BATCHES;
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

static int resolve_direct_comm_rank(unimpi_lib_handle_t handle,
                                    comm_rank_function_t *function) {
    void *symbol;

    symbol = unimpi_platform_dlsym(handle, "MPI_Comm_rank");
    if (!symbol) {
        fprintf(stderr, "failed to resolve direct MPI_Comm_rank: %s\n",
                unimpi_platform_dlerror());
        return -1;
    }
    if (sizeof(*function) != sizeof(symbol)) {
        fprintf(stderr,
                "function pointer representation is incompatible with dlsym\n");
        return -1;
    }

    /*
     * POSIX specifies that dlsym results may be converted to function
     * pointers. Copying the representation avoids a non-ISO object-pointer
     * cast while retaining the same portable runtime check on Windows.
     */
    memcpy(function, &symbol, sizeof(*function));
    return 0;
}

static int warm_up_calls(comm_rank_function_t direct_function,
                         int iterations,
                         int expected_rank) {
    int observed_rank = -1;
    int i;

    for (i = 0; i < iterations; ++i) {
        if (require_mpi_success(
                direct_function(MPI_COMM_WORLD, &observed_rank),
                "direct MPI_Comm_rank(warmup)") != 0 ||
            require_mpi_success(
                unimpi.comm_rank(MPI_COMM_WORLD, &observed_rank),
                "vtable MPI_Comm_rank(warmup)") != 0) {
            return -1;
        }
    }

    if (observed_rank != expected_rank) {
        fprintf(stderr, "MPI_Comm_rank warmup returned rank %d, expected %d\n",
                observed_rank, expected_rank);
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }
    return 0;
}

static int measure_direct(comm_rank_function_t direct_function,
                          int iterations,
                          int expected_rank,
                          double *nanoseconds_per_call) {
    double start;
    double end;
    int observed_rank = -1;
    int i;

    start = MPI_Wtime();
    for (i = 0; i < iterations; ++i) {
        if (require_mpi_success(
                direct_function(MPI_COMM_WORLD, &observed_rank),
                "direct MPI_Comm_rank(measurement)") != 0) {
            return -1;
        }
    }
    end = MPI_Wtime();

    if (observed_rank != expected_rank) {
        fprintf(stderr, "direct MPI_Comm_rank returned rank %d, expected %d\n",
                observed_rank, expected_rank);
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    *nanoseconds_per_call =
        (end - start) * 1.0e9 / (double)iterations;
    return 0;
}

static int measure_vtable(int iterations,
                          int expected_rank,
                          double *nanoseconds_per_call) {
    double start;
    double end;
    int observed_rank = -1;
    int i;

    start = MPI_Wtime();
    for (i = 0; i < iterations; ++i) {
        if (require_mpi_success(
                unimpi.comm_rank(MPI_COMM_WORLD, &observed_rank),
                "vtable MPI_Comm_rank(measurement)") != 0) {
            return -1;
        }
    }
    end = MPI_Wtime();

    if (observed_rank != expected_rank) {
        fprintf(stderr, "vtable MPI_Comm_rank returned rank %d, expected %d\n",
                observed_rank, expected_rank);
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return -1;
    }

    *nanoseconds_per_call =
        (end - start) * 1.0e9 / (double)iterations;
    return 0;
}

static int compare_doubles(const void *left, const void *right) {
    double left_value = *(const double *)left;
    double right_value = *(const double *)right;

    if (left_value < right_value) {
        return -1;
    }
    if (left_value > right_value) {
        return 1;
    }
    return 0;
}

static double median(double *values, int count) {
    qsort(values, (size_t)count, sizeof(*values), compare_doubles);
    if ((count % 2) != 0) {
        return values[count / 2];
    }
    return (values[count / 2 - 1] + values[count / 2]) / 2.0;
}

int main(int argc, char **argv) {
    benchmark_config_t config;
    unimpi_lib_handle_t direct_handle;
    comm_rank_function_t direct_comm_rank = NULL;
    const char *backend_name;
    const char *library_path;
    double *direct_times;
    double *vtable_times;
    double *delta_times;
    double direct_median;
    double vtable_median;
    double delta_median;
    int parse_result;
    int ret;
    int rank;
    int process_count;
    int batch;

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
    library_path = unimpi_get_library_path();
    if (!backend_name) {
        backend_name = "unknown";
    }
    if (!library_path) {
        fprintf(stderr, "unimpi did not report the loaded MPI library path\n");
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    direct_handle = unimpi_platform_dlopen(library_path);
    if (!direct_handle) {
        fprintf(stderr, "failed to reopen MPI library %s: %s\n",
                library_path, unimpi_platform_dlerror());
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    if (resolve_direct_comm_rank(direct_handle, &direct_comm_rank) != 0) {
        unimpi_platform_dlclose(direct_handle);
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    direct_times =
        (double *)malloc((size_t)config.batches * sizeof(*direct_times));
    vtable_times =
        (double *)malloc((size_t)config.batches * sizeof(*vtable_times));
    delta_times =
        (double *)malloc((size_t)config.batches * sizeof(*delta_times));
    if (!direct_times || !vtable_times || !delta_times) {
        fprintf(stderr, "failed to allocate benchmark result arrays\n");
        free(direct_times);
        free(vtable_times);
        free(delta_times);
        unimpi_platform_dlclose(direct_handle);
        (void)unimpi.abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    if (rank == 0) {
        if (warm_up_calls(direct_comm_rank, config.warmup_iterations,
                          rank) != 0) {
            return 1;
        }
        if (config.format == OUTPUT_CSV) {
            printf("backend,library,processes,batch,iterations,"
                   "direct_ns,vtable_ns,delta_ns\n");
        } else {
            printf("=== UniMPI Paired MPI_Comm_rank Dispatch Benchmark ===\n");
            printf("Backend: %s\nLibrary: %s\nProcesses: %d\n"
                   "Warmup: %d, iterations: %d, batches: %d\n",
                   backend_name, library_path, process_count,
                   config.warmup_iterations, config.benchmark_iterations,
                   config.batches);
            printf("%8s %16s %16s %16s\n",
                   "Batch", "Direct(ns)", "Vtable(ns)", "Delta(ns)");
        }
    }

    for (batch = 0; batch < config.batches; ++batch) {
        if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                                "MPI_Barrier(before batch)") != 0) {
            return 1;
        }

        if (rank == 0) {
            if ((batch % 2) == 0) {
                if (measure_direct(direct_comm_rank,
                                   config.benchmark_iterations, rank,
                                   &direct_times[batch]) != 0 ||
                    measure_vtable(config.benchmark_iterations, rank,
                                   &vtable_times[batch]) != 0) {
                    return 1;
                }
            } else {
                if (measure_vtable(config.benchmark_iterations, rank,
                                   &vtable_times[batch]) != 0 ||
                    measure_direct(direct_comm_rank,
                                   config.benchmark_iterations, rank,
                                   &direct_times[batch]) != 0) {
                    return 1;
                }
            }
            delta_times[batch] =
                vtable_times[batch] - direct_times[batch];

            if (config.format == OUTPUT_CSV) {
                printf("%s,\"%s\",%d,%d,%d,%.9f,%.9f,%.9f\n",
                       backend_name, library_path, process_count, batch + 1,
                       config.benchmark_iterations, direct_times[batch],
                       vtable_times[batch], delta_times[batch]);
            } else {
                printf("%8d %16.6f %16.6f %16.6f\n",
                       batch + 1, direct_times[batch], vtable_times[batch],
                       delta_times[batch]);
            }
        }

        if (require_mpi_success(MPI_Barrier(MPI_COMM_WORLD),
                                "MPI_Barrier(after batch)") != 0) {
            return 1;
        }
    }

    if (rank == 0) {
        direct_median = median(direct_times, config.batches);
        vtable_median = median(vtable_times, config.batches);
        delta_median = median(delta_times, config.batches);
        if (config.format == OUTPUT_CSV) {
            printf("%s,\"%s\",%d,median,%d,%.9f,%.9f,%.9f\n",
                   backend_name, library_path, process_count,
                   config.benchmark_iterations, direct_median,
                   vtable_median, delta_median);
        } else {
            printf("Median   %16.6f %16.6f %16.6f\n",
                   direct_median, vtable_median, delta_median);
            printf("Delta is the paired timing difference observed in this "
                   "run; it is not a fixed overhead guarantee.\n");
        }
    }

    free(direct_times);
    free(vtable_times);
    free(delta_times);
    unimpi_platform_dlclose(direct_handle);

    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "failed to finalize unimpi: %s\n",
                unimpi_error_string(ret));
        return 1;
    }

    return 0;
}
