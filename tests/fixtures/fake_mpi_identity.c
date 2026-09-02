/* fake_mpi_identity.c - Fixture library for testing backend identification
 *
 * This fixture creates fake MPI libraries that export different symbols
 * to test backend identification without needing actual MPI implementations.
 */
#include <stdlib.h>
#include <string.h>

/* Compile with different defines to create different fake backends:
 * - UNIMPI_FAKE_OPENMPI: Exports ompi_mpi_comm_world
 * - UNIMPI_FAKE_MPICH: Exports MPIR_Comm_world
 * - UNIMPI_FAKE_INTELMPI: Exports MPIR_Comm_world and __I_MPI___cpu_core_type
 */

#ifdef _WIN32
#define FAKE_MPI_EXPORT __declspec(dllexport)
#else
#define FAKE_MPI_EXPORT __attribute__((visibility("default")))
#endif

#ifdef UNIMPI_FAKE_OPENMPI
/* OpenMPI-style fake backend */
FAKE_MPI_EXPORT int ompi_mpi_comm_world;
FAKE_MPI_EXPORT int ompi_mpi_comm_self;
static const char fake_library_version[] = "Open MPI fake backend";
#elif defined(UNIMPI_FAKE_INTELMPI)
/* IntelMPI-style fake backend (MPICH-based but with Intel symbol) */
FAKE_MPI_EXPORT int MPIR_Comm_world;
FAKE_MPI_EXPORT int MPIR_Comm_self;
static const char fake_library_version[] = "Intel(R) MPI Library 2021.16";
#elif defined(UNIMPI_FAKE_MPICH)
/* MPICH-style fake backend */
FAKE_MPI_EXPORT int MPIR_Comm_world;
FAKE_MPI_EXPORT int MPIR_Comm_self;
static const char fake_library_version[] = "MPICH Version: 4.3.1";
#else
/* Unknown backend - no identifying symbols */
static const char fake_library_version[] = "Acme Message Passing Runtime 1.0";
#endif

/* MPI functions for all fake backends */
int MPI_Init(int *argc, char ***argv) {
    (void)argc;
    (void)argv;
    return 0;
}

int MPI_Finalize(void) {
    return 0;
}

int MPI_Comm_size(int comm, int *size) {
    (void)comm;
    *size = 1;
    return 0;
}

int MPI_Comm_rank(int comm, int *rank) {
    (void)comm;
    *rank = 0;
    return 0;
}

int MPI_Get_library_version(char *version, int *resultlen) {
    size_t length = strlen(fake_library_version);
    memcpy(version, fake_library_version, length + 1);
    *resultlen = (int)length;
    return 0;
}

/* IntelMPI-specific symbol. Must be explicitly exported on Windows (where
 * WINDOWS_EXPORT_ALL_SYMBOLS does not reliably capture leading-underscore
 * text symbols); the loader identifies Intel MPI by this symbol. */
#if defined(UNIMPI_FAKE_INTELMPI)
FAKE_MPI_EXPORT int __I_MPI___cpu_core_type(void) {
    return 0;
}
#endif
