/* test_backend_identification.c - Test backend identification via symbols
 *
 * This internal test validates that the loader correctly identifies
 * different MPI backends by examining the symbols exported by the library.
 *
 * Usage: test_backend_identification <openmpi_fake> <mpich_fake> <intelmpi_fake> <unknown_fake>
 *   where each argument is the path to a fake MPI library fixture.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "unimpi.h"
#include "unimpi_loader.h"

#ifdef _WIN32
#define EXPECTED_PRIMARY_BACKEND UNIMPI_BACKEND_MSMPI
#else
#define EXPECTED_PRIMARY_BACKEND UNIMPI_BACKEND_OPENMPI
#endif

static unimpi_lib_handle_t load_and_assert_identity(
    const char *path, unimpi_backend_type_t expected) {
    unimpi_lib_handle_t handle = NULL;
    int ret;

    ret = unimpi_loader_load(path, &handle);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Failed to load library '%s': %s\n",
                path, unimpi_error_string(ret));
        abort();
    }

    unimpi_backend_type_t detected = unimpi_loader_identify_backend(handle);
    if (detected != expected) {
        fprintf(stderr, "Backend identification failed for '%s':\n", path);
        fprintf(stderr, "  Expected: %d\n", expected);
        fprintf(stderr, "  Detected: %d\n", detected);
        abort();
    }

    return handle;
}

int main(int argc, char **argv) {
    unimpi_lib_handle_t handles[4];
    size_t i;

    /* Check arguments */
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <openmpi_fake> <mpich_fake> <intelmpi_fake> <unknown_fake>\n",
                argv[0]);
        fprintf(stderr, "  Each argument should be a path to a fake MPI library fixture.\n");
        return 1;
    }

    printf("Testing backend identification...\n");

    /* Test OpenMPI-style backend identification
     * Should be detected via ompi_mpi_comm_world symbol
     */
    printf("  Testing OpenMPI-style backend...\n");
    handles[0] = load_and_assert_identity(argv[1], UNIMPI_BACKEND_OPENMPI);

    /* Test MPICH-style backend identification
     * Should be detected via MPIR_Comm_world symbol
     */
    printf("  Testing MPICH-style backend...\n");
    handles[1] = load_and_assert_identity(argv[2], UNIMPI_BACKEND_MPICH);

    /* Test IntelMPI-style backend identification
     * Should be detected via MPIR_Comm_world + __I_MPI___cpu_core_type symbols
     */
    printf("  Testing IntelMPI-style backend...\n");
    handles[2] = load_and_assert_identity(argv[3], UNIMPI_BACKEND_INTELMPI);

    /* Test unknown backend identification
     * Should return UNIMPI_BACKEND_UNKNOWN when no identifying symbols found
     */
    printf("  Testing unknown backend...\n");
    handles[3] = load_and_assert_identity(argv[4], UNIMPI_BACKEND_UNKNOWN);

    /* Cleanup: unload all handles in reverse order */
    printf("  Cleaning up...\n");
    for (i = sizeof(handles) / sizeof(handles[0]); i > 0; i--) {
        unimpi_loader_unload(handles[i - 1]);
    }

    printf("All backend identification tests passed!\n");
    return 0;
}
