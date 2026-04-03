/* src/backends/msmpi.c */
#include "tftk_mpi_vtable.h"
#include "tftk_mpi_platform.h"
#include "tftk_mpi.h"

/* MS-MPI is MPICH-derived, reuse MPICH implementation */
int tftk_mpi_vtable_init_msmpi(tftk_mpi_lib_handle_t handle) {
    /* Forward to MPICH initialization */
    extern int tftk_mpi_vtable_init_mpich(tftk_mpi_lib_handle_t);
    return tftk_mpi_vtable_init_mpich(handle);
}
