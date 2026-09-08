/* Negative-compile gate proof for the MPI-3.0 vtable layout.
 *
 * This TU is intentionally compiled by tests/cmake/check_vtable_strict.cmake
 * with -DUNIMPI_MPI_TARGET_VERSION=2 -DUNIMPI_MPI_TARGET_SUBVERSION=2 (target
 * 2.2). At that target every MPI-3.0 field is gated OUT of the struct, so each
 * .ibcast / .comm_join / etc. reference below is a hard compile error.
 *
 * The CTest script inverts the compiler's exit code: compilation SUCCEEDING
 * here (fields still present at 2.2) is a FAILURE of the check, and compilation
 * FAILING (fields absent) is the expected PASS.
 *
 * This file is never built or linked as part of the normal target set.
 */
#include "unimpi.h"

/* One representative probe per MPI-3.0 cluster. None of these may exist at a
 * 2.2 target; if any of them compiles, the corresponding guard is broken. */
static void *p_matched_probe = (void *)&unimpi.imrecv;
static void *p_nonblocking = (void *)&unimpi.ibcast;
static void *p_alltoallw = (void *)&unimpi.alltoallw;
static void *p_comm_3x = (void *)&unimpi.comm_split_type;
static void *p_win_alloc_shared = (void *)&unimpi.win_create_dynamic;
static void *p_rma_atomics = (void *)&unimpi.compare_and_swap;
static void *p_rma_sync_3x = (void *)&unimpi.win_sync;
static void *p_comm_join = (void *)&unimpi.comm_join;
static void *p_op_commutative = (void *)&unimpi.op_commutative;
/* MPI-T tool-interface vtable: none of these may exist at a 2.2 target. */
static void *p_mt_init = (void *)&unimpi_mt.t_init_thread;
static void *p_mt_cvar = (void *)&unimpi_mt.t_cvar_read;
static void *p_mt_pvar = (void *)&unimpi_mt.t_pvar_aggregate;
/* MPI-3.1 additions: address arithmetic, nonblocking collective file I/O and
 * the MPI-T get_index lookups. None may exist at a 2.2 target either. */
static void *p_aint_add = (void *)&unimpi.aint_add;
static void *p_aint_diff = (void *)&unimpi.aint_diff;
static void *p_file_iread_all = (void *)&unimpi.file_iread_all;
static void *p_file_iwrite_all = (void *)&unimpi.file_iwrite_all;
static void *p_file_iread_at_all = (void *)&unimpi.file_iread_at_all;
static void *p_file_iwrite_at_all = (void *)&unimpi.file_iwrite_at_all;
static void *p_mt_cvar_get_index = (void *)&unimpi_mt.t_cvar_get_index;
static void *p_mt_pvar_get_index = (void *)&unimpi_mt.t_pvar_get_index;
static void *p_mt_category_get_index = (void *)&unimpi_mt.t_category_get_index;

int main(void) {
    (void)p_matched_probe;
    (void)p_nonblocking;
    (void)p_alltoallw;
    (void)p_comm_3x;
    (void)p_win_alloc_shared;
    (void)p_rma_atomics;
    (void)p_rma_sync_3x;
    (void)p_comm_join;
    (void)p_op_commutative;
    (void)p_mt_init;
    (void)p_mt_cvar;
    (void)p_mt_pvar;
    (void)p_aint_add;
    (void)p_aint_diff;
    (void)p_file_iread_all;
    (void)p_file_iwrite_all;
    (void)p_file_iread_at_all;
    (void)p_file_iwrite_at_all;
    (void)p_mt_cvar_get_index;
    (void)p_mt_pvar_get_index;
    (void)p_mt_category_get_index;
    return 0;
}
