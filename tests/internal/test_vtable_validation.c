/* Vtable core-symbol validation and cleanup tests using fake MPI DSOs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_loader.h"
#include "unimpi_vtable.h"

static void assert_core_vtable_cleared(void) {
    assert(unimpi.init == NULL);
    assert(unimpi.finalize == NULL);
    assert(unimpi.comm_size == NULL);
    assert(unimpi.comm_rank == NULL);
    assert(unimpi.get_library_version == NULL);
    assert(UNIMPI_COMM_WORLD == 0);
    assert(UNIMPI_COMM_SELF == 0);
    assert(unimpi_get_backend_type() == UNIMPI_BACKEND_UNKNOWN);
}

static void test_missing_core_symbol(const char *library_path) {
    unimpi_lib_handle_t handle = NULL;

    unimpi_vtable_cleanup();
    assert_core_vtable_cleared();
    assert(unimpi_loader_load(library_path, &handle) == UNIMPI_OK);
    assert(handle != NULL);
    assert(unimpi_vtable_init(handle) == UNIMPI_ERR_SYMBOL_NOT_FOUND);
    assert_core_vtable_cleared();
    unimpi_loader_unload(handle);
}

static void test_success_profile_and_cleanup(const char *library_path) {
    char library_version[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    int argc_value = 1;
    char program_name[] = "test_vtable_validation";
    char *argv_values[] = {program_name, NULL};
    char **argv_pointer = argv_values;
    int library_version_length = -1;
    int rank = -1;
    int size = -1;
    int version_major = -1;
    int version_minor = -1;
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(library_path, &handle) == UNIMPI_OK);
    assert(handle != NULL);
    assert(unimpi_vtable_init(handle) == UNIMPI_OK);
    assert(unimpi_get_backend_type() != UNIMPI_BACKEND_UNKNOWN);

    assert(unimpi.init != NULL);
    assert(unimpi.init_thread != NULL);
    assert(unimpi.finalize != NULL);
    assert(unimpi.comm_size != NULL);
    assert(unimpi.comm_rank != NULL);
    assert(unimpi.get_version != NULL);
    assert(unimpi.get_library_version != NULL);
    assert(UNIMPI_COMM_WORLD != 0);
    assert(UNIMPI_COMM_SELF != 0);

    assert(unimpi.init(&argc_value, &argv_pointer) == 0);
    assert(unimpi.comm_size(UNIMPI_COMM_WORLD, &size) == 0);
    assert(size == 1);
    assert(unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank) == 0);
    assert(rank == 0);
    assert(unimpi.get_version(&version_major, &version_minor) == 0);
    assert(version_major == 4);
    assert(version_minor == 1);
    assert(unimpi.get_library_version(library_version,
                                      &library_version_length) == 0);
    assert(library_version_length > 0);
    assert(strstr(library_version, "fake backend") != NULL);
    assert(unimpi.finalize() == 0);

    unimpi_vtable_cleanup();
    assert_core_vtable_cleared();
    unimpi_loader_unload(handle);
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr,
                "Usage: %s <complete> <missing-init> <missing-finalize> "
                "<missing-comm-size> <missing-comm-rank>\n",
                argv[0]);
        return 2;
    }

    test_success_profile_and_cleanup(argv[1]);
    test_missing_core_symbol(argv[2]);
    test_missing_core_symbol(argv[3]);
    test_missing_core_symbol(argv[4]);
    test_missing_core_symbol(argv[5]);

    printf("Vtable core-symbol validation tests passed\n");
    return 0;
}
