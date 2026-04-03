/* src/backends/intelmpi.c */
#include "tftk_mpi_vtable.h"
#include "tftk_mpi_platform.h"
#include "tftk_mpi.h"

/* Intel-MPI is OpenMPI-compatible, reuse OpenMPI implementation */
int tftk_mpi_vtable_init_intelmpi(tftk_mpi_lib_handle_t handle) {
    /* Forward to OpenMPI initialization */
    extern int tftk_mpi_vtable_init_openmpi(tftk_mpi_lib_handle_t);
    return tftk_mpi_vtable_init_openmpi(handle);
}
