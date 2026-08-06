/* Public facade argument and cleanup tests using the fake MPI DSO. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_vtable.h"

static void set_fake_library(const char *library_path) {
#ifdef _WIN32
    assert(_putenv_s("UNIMPI_BACKEND", "") == 0);
    assert(_putenv_s("UNIMPI_LIBRARY", library_path) == 0);
#else
    assert(unsetenv("UNIMPI_BACKEND") == 0);
    assert(setenv("UNIMPI_LIBRARY", library_path, 1) == 0);
#endif
}

static void test_null_pointer_handling(void) {
    char version[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    int flag = -1;
    int result_length = -1;
    int version_major = -1;
    int version_minor = -1;

    assert(unimpi_mpi_initialized(NULL) == UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_mpi_finalized(NULL) == UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_mpi_get_version(NULL, &version_minor) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_mpi_get_version(&version_major, NULL) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_mpi_get_library_version(NULL, &result_length) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_mpi_get_library_version(version, NULL) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_init_thread(NULL, NULL, UNIMPI_THREAD_SINGLE, NULL) ==
           UNIMPI_ERR_INVALID_ARGUMENT);
    assert(unimpi_init_thread(NULL, NULL, -1, &flag) ==
           UNIMPI_ERR_INVALID_ARGUMENT);

    assert(unimpi_mpi_initialized(&flag) == UNIMPI_OK);
    assert(flag == 0);
    assert(unimpi_mpi_finalized(&flag) == UNIMPI_OK);
    assert(flag == 0);
    assert(unimpi_is_initialized() == 0);
}

static void test_version_info(void) {
    char library_version[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    int result_length = -1;
    int version_major = -1;
    int version_minor = -1;

    assert(unimpi_mpi_get_version(&version_major, &version_minor) == UNIMPI_OK);
    assert(version_major == UNIMPI_MPI_VERSION);
    assert(version_minor == UNIMPI_MPI_SUBVERSION);
    assert(unimpi_mpi_get_library_version(library_version, &result_length) ==
           UNIMPI_OK);
    assert(result_length == (int)strlen(library_version));
    assert(strstr(library_version, "no active MPI backend") != NULL);
}

static void test_init_finalize_cleanup(const char *library_path) {
    assert(unimpi_init(NULL, NULL) == UNIMPI_OK);
    assert(unimpi_is_initialized() == 1);
    assert(unimpi_get_backend_name() != NULL);
    assert(unimpi_get_library_path() != NULL);
    assert(strcmp(unimpi_get_library_path(), library_path) == 0);
    assert(unimpi.init != NULL);
    assert(unimpi.finalize != NULL);
    assert(unimpi.get_library_version != NULL);

    assert(unimpi_finalize() == UNIMPI_OK);
    assert(unimpi_is_initialized() == 0);
    assert(unimpi_get_backend_name() == NULL);
    assert(unimpi_get_library_path() == NULL);
    assert(unimpi.init == NULL);
    assert(unimpi.finalize == NULL);
    assert(unimpi.get_library_version == NULL);
    assert(UNIMPI_COMM_WORLD == 0);
    assert(UNIMPI_COMM_SELF == 0);
    assert(UNIMPI_STATUS_IGNORE == NULL);
    assert(unimpi_init(NULL, NULL) == UNIMPI_ERR_FINALIZED);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fake-mpi-library>\n", argv[0]);
        return 2;
    }

    set_fake_library(argv[1]);
    test_null_pointer_handling();
    test_version_info();
    test_init_finalize_cleanup(argv[1]);

    printf("Memory-safety facade tests passed\n");
    return 0;
}
