#ifndef UNIMPI_MT_H
#define UNIMPI_MT_H

#include <stdint.h>
#include "unimpi_vtable.h"   /* MPI_Datatype (intptr_t) for get_info signatures */
#include "unimpi_version.h"

/* MPI-T opens handles (opaque; produced by backend, returned by app).
 * MPI_T_pvar_session is the standard pvar-session handle type; there is no
 * separate "MPI_T_session" in the MPI standard. */
typedef intptr_t MPI_T_pvar_session;
typedef intptr_t MPI_T_pvar_handle;
typedef intptr_t MPI_T_cvar_handle;
typedef intptr_t MPI_T_handle;      /* binding object type = opaque MPI handle */
typedef int MPI_T_enum;             /* standard: int-valued enumeration */

/* Independent MPI-T tools-interface vtable. Lives separately from the main
 * unimpi_vtable_t so MPI_T_* calls stay usable before MPI_Init and after
 * MPI_Finalize: the backend library's dlclose is reference-counted across the
 * two interfaces (see src/core.c unimpi_ensure_loaded / g_refcount). Both
 * vtables are filled by the same backend init for the same library handle. */
#if UNIMPI_MPI_AT_LEAST(3,0)
typedef struct unimpi_mt_vtable {
    int (*t_init_thread)(int required, int *provided);
    int (*t_finalize)(void);
    int (*t_cvar_get_num)(int *num_cvar);
    int (*t_cvar_get_index)(const char *name, int *cvar_index);
    int (*t_cvar_get_info)(int cvar_index, char *name, int *name_len,
                           MPI_Datatype *datatype, MPI_T_enum *enumtype,
                           MPI_T_cvar_handle *cvar_handle, int *verbosity,
                           int *scope, void *var_extra);
    int (*t_cvar_handle_alloc)(int cvar_index, void *obj_handle,
                               MPI_T_cvar_handle *handle, int *count);
    int (*t_cvar_handle_free)(MPI_T_cvar_handle *handle);
    int (*t_cvar_read)(MPI_T_cvar_handle handle, void *cvar_value);
    int (*t_cvar_read_index)(int cvar_index, void *cvar_value);
    int (*t_cvar_write)(MPI_T_cvar_handle handle, const void *cvar_value);
    int (*t_cvar_write_index)(int cvar_index, const void *cvar_value);
    int (*t_pvar_get_num)(int *num_pvar);
    int (*t_pvar_get_index)(const char *name, int var_class, int *pvar_index);
    int (*t_pvar_get_info)(int pvar_index, char *name, int *name_len,
                           MPI_T_enum *enumtype, MPI_T_pvar_session *binding,
                           int *verbosity, int *var_class, void *var_extra);
    int (*t_pvar_session_create)(MPI_T_pvar_session *session);
    int (*t_pvar_session_free)(MPI_T_pvar_session *session);
    int (*t_pvar_handle_alloc)(MPI_T_pvar_session session, int pvar_index,
                               MPI_T_handle obj_handle, MPI_T_pvar_handle *handle,
                               int *count);
    int (*t_pvar_handle_free)(MPI_T_pvar_session session, MPI_T_pvar_handle *handle);
    int (*t_pvar_start)(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
    int (*t_pvar_stop)(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
    int (*t_pvar_read)(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                       void *value);
    int (*t_pvar_write)(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                        const void *value);
    int (*t_pvar_readreset)(MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                            void *value);
    int (*t_pvar_reset)(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
    int (*t_pvar_aggregate)(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
} unimpi_mt_vtable_t;

extern unimpi_mt_vtable_t unimpi_mt;   /* born all-NULL; filled by backend init */

/* Lifecycle coordination helpers (implemented in src/core.c, MPI-T only).
 * The backend library handle is reference-counted: MPI (unimpi_init) and
 * MPI-T (unimpi_mpit_init_thread) each hold one reference; the library is
 * only dlclose'd once both release (both finalize). unimpi_ensure_loaded()
 * lazily dlopens and fills BOTH vtables idempotently; it is a general core
 * mechanism declared in unimpi_vtable.h (ungated, also used by target 2). */
int unimpi_mt_initialized(int *flag);          /* *flag = g_mpit_inited */
int unimpi_mt_ref_acquire(void);               /* +1 library reference */
int unimpi_mt_ref_release(void);               /* -1; dlclose at 0 */
void unimpi_mt_set_inited(int inited);         /* set/clear g_mpit_inited */

/* MPI-T lifecycle wrappers (implemented in src/mpit.c). Only these two carry
 * state beyond the vtable -- ensure_loaded, the reference count and the
 * g_mpit_inited flag -- so only they need to be functions. The remaining
 * MPI_T_* calls have no such logic and are mapped straight onto the
 * unimpi_mt.t_* vtable members by unimpi_std_macros.h (exactly like the main
 * vtable: lifecycle init is a function, plain forwards are unimpi.<slot>). */
int unimpi_mpit_init_thread(int required, int *provided);
int unimpi_mpit_finalize(void);
#endif /* UNIMPI_MPI_AT_LEAST(3,0) */

#endif /* UNIMPI_MT_H */
