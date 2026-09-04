#include "unimpi.h"
#include "unimpi_mt.h"
#include "unimpi_errors.h"
#include "unimpi_vtable.h"
#if UNIMPI_MPI_AT_LEAST(3,0)

/* Ensure the backend is loaded and both vtables are filled. Called only from
 * MPI_T_init_thread, the tool interface's lifecycle boundary -- it lets the
 * interface be (re)entered after MPI_Finalize without a prior MPI_Init. The
 * remaining MPI_T calls are not wrapped here at all -- unimpi_std_macros.h
 * maps them straight onto the unimpi_mt.t_* vtable members (the same
 * zero-cost shape as the main vtable's MPI_Send -> unimpi.send), trusting
 * the caller to have called init_thread first (MPI_T requires it before any
 * other use): calling one on an unloaded backend dereferences a NULL slot
 * rather than adding a runtime protection branch. */
int unimpi_ensure_loaded(void);   /* from src/core.c */

int unimpi_mpit_init_thread(int required, int *provided) {
    int rc = unimpi_ensure_loaded();
    if (rc != UNIMPI_OK) return rc;
    unimpi_mt_ref_acquire();
    unimpi_mt_set_inited(1);
    return unimpi_mt.t_init_thread(required, provided);
}

int unimpi_mpit_finalize(void) {
    int rc = unimpi_mt.t_finalize();
    unimpi_mt_set_inited(0);
    unimpi_mt_ref_release();
    return rc;
}

#endif /* UNIMPI_MPI_AT_LEAST(3,0) */
