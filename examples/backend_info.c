/* backend_info.c - Report the MPI implementation selected at runtime. */
#include <stdio.h>

#include "unimpi.h"

static int check_mpi_call(int code, const char *operation) {
    if (code == 0) {
        return 0;
    }
    fprintf(stderr, "%s failed with MPI error %d: %s\n",
            operation, code, unimpi_mpi_error_string(code));
    return -1;
}

int main(int argc, char **argv) {
    char version_string[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    const char *backend_name;
    const char *library_path;
    int version_length = 0;
    int mpi_version = 0;
    int mpi_subversion = 0;
    int rank;
    int ret;

    ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }

    if (!unimpi.get_library_version || !unimpi.get_version) {
        fprintf(stderr, "selected backend lacks MPI version query symbols\n");
        (void)unimpi.abort(UNIMPI_COMM_WORLD, 1);
        return 1;
    }
    if (check_mpi_call(
            unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank),
            "unimpi.comm_rank") != 0 ||
        check_mpi_call(
            unimpi.get_version(&mpi_version, &mpi_subversion),
            "unimpi.get_version") != 0 ||
        check_mpi_call(
            unimpi.get_library_version(version_string, &version_length),
            "unimpi.get_library_version") != 0) {
        (void)unimpi.abort(UNIMPI_COMM_WORLD, 1);
        return 1;
    }
    if (version_length < 0 ||
        version_length >= UNIMPI_MAX_LIBRARY_VERSION_STRING) {
        fprintf(stderr, "backend returned an invalid version string length\n");
        (void)unimpi.abort(UNIMPI_COMM_WORLD, 1);
        return 1;
    }
    version_string[version_length] = '\0';

    backend_name = unimpi_get_backend_name();
    library_path = unimpi_get_library_path();
    if (rank == 0) {
        printf("Backend: %s\n", backend_name ? backend_name : "unknown");
        printf("Library: %s\n", library_path ? library_path : "unknown");
        printf("MPI standard reported by backend: %d.%d\n",
               mpi_version, mpi_subversion);
        printf("MPI library version: %s\n", version_string);
    }

    ret = unimpi_finalize();
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "unimpi finalize failed: %s\n",
                unimpi_error_string(ret));
        return 1;
    }
    return 0;
}
