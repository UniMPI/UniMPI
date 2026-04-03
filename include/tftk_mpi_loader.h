#ifndef TFTK_MPI_LOADER_H
#define TFTK_MPI_LOADER_H

#include "tftk_mpi_platform.h"

/* Backend types */
typedef enum {
    TFTK_MPI_BACKEND_UNKNOWN = 0,
    TFTK_MPI_BACKEND_OPENMPI,
    TFTK_MPI_BACKEND_MPICH,
    TFTK_MPI_BACKEND_INTELMPI,
    TFTK_MPI_BACKEND_MSMPI
} tftk_mpi_backend_type_t;


/* Backend info */
typedef struct {
    tftk_mpi_backend_type_t type;
    const char *name;
    const char *lib_name;
    int priority;
} tftk_mpi_backend_info_t;

/* Backend detection and loading API */
const char* tftk_mpi_loader_get_env_backend(void);
int tftk_mpi_loader_detect_backend(const char **out_lib_path);
int tftk_mpi_loader_load(const char *lib_path, tftk_mpi_lib_handle_t *out_handle);
void tftk_mpi_loader_unload(tftk_mpi_lib_handle_t handle);
tftk_mpi_backend_type_t tftk_mpi_loader_identify_backend(tftk_mpi_lib_handle_t handle);

/* Known backends */
#define TFTK_MPI_MAX_BACKENDS 4
extern const tftk_mpi_backend_info_t tftk_mpi_backends[TFTK_MPI_MAX_BACKENDS];

#endif /* TFTK_MPI_LOADER_H */
