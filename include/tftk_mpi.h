#ifndef TFTK_MPI_H
#define TFTK_MPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tftk_mpi_vtable.h"

/* Version */
#define TFTK_MPI_VERSION_MAJOR 0
#define TFTK_MPI_VERSION_MINOR 1
#define TFTK_MPI_VERSION_PATCH 0

/* Error codes */
#define TFTK_MPI_OK 0
#define TFTK_MPI_ERR_NO_BACKEND -1
#define TFTK_MPI_ERR_BACKEND_LOAD -2
#define TFTK_MPI_ERR_ABI_MISMATCH -3
#define TFTK_MPI_ERR_NOT_INITIALIZED -4
#define TFTK_MPI_ERR_ALREADY_INITIALIZED -5
#define TFTK_MPI_ERR_SYMBOL_NOT_FOUND -6
#define TFTK_MPI_ERR_OUT_OF_MEMORY -7

/* Initialization and finalization */
int tftk_mpi_init(int *argc, char ***argv);
int tftk_mpi_init_with(const char *backend_name);
int tftk_mpi_finalize(void);

/* Query functions */
int tftk_mpi_is_initialized(void);
const char* tftk_mpi_get_backend_name(void);

/* Error handling */
const char* tftk_mpi_error_string(int error_code);
int tftk_mpi_error_class(int error_code, int *error_class);

/* Optional standard MPI macros */
#ifdef TFTK_MPI_USE_STD_NAMES
#include "tftk_mpi_std_macros.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* TFTK_MPI_H */
