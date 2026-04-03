#ifndef TFTK_MPI_H
#define TFTK_MPI_H

#ifdef __cplusplus
extern "C" {
#endif

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

/* TODO: Add full implementation in subsequent tasks */

#ifdef __cplusplus
}
#endif

#endif /* TFTK_MPI_H */
