/* src/backends/msmpi.c */
#include "unimpi_vtable.h"
#include "unimpi_platform.h"
#include "unimpi.h"

/* MS-MPI is MPICH-derived, reuse MPICH implementation */
int unimpi_vtable_init_msmpi(unimpi_lib_handle_t handle) {
    /* Forward to MPICH initialization */
    extern int unimpi_vtable_init_mpich(unimpi_lib_handle_t);
    return unimpi_vtable_init_mpich(handle);
}
