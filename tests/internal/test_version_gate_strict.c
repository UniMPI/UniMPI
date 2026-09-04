#include "unimpi.h"
#include <stdio.h>

/* Compiled by tests/CMakeLists.txt with
 *   -DUNIMPI_USE_STD_NAMES
 *   -DUNIMPI_MPI_TARGET_VERSION=2
 *   -DUNIMPI_MPI_TARGET_SUBVERSION=2
 * This is the negative counterpart of test_version_gate.c: at a 2.2
 * target none of the MPI-3.0 std macros may exist. If any does, the
 * matching #error aborts this TU's build, failing the test at compile
 * time (a green build therefore proves the guards are in place).
 */

/* MPI-3.0 macro clusters -- each must be absent at 2.2 */
#ifdef MPI_Mprobe
#error "MPI_Mprobe must be absent at target 2.2"
#endif
#ifdef MPI_Ibcast
#error "MPI_Ibcast must be absent at target 2.2"
#endif
#ifdef MPI_Comm_create_group
#error "MPI_Comm_create_group must be absent at target 2.2"
#endif
#ifdef MPI_Win_allocate_shared
#error "MPI_Win_allocate_shared must be absent at target 2.2"
#endif
#ifdef MPI_Fetch_and_op
#error "MPI_Fetch_and_op must be absent at target 2.2"
#endif
#ifdef MPI_Win_sync
#error "MPI_Win_sync must be absent at target 2.2"
#endif
#ifdef MPI_T_init_thread
#error "MPI_T_init_thread must be absent at target 2.2"
#endif
#ifdef MPI_T_cvar_get_num
#error "MPI_T_cvar_get_num must be absent at target 2.2"
#endif
#ifdef MPI_T_pvar_session_create
#error "MPI_T_pvar_session_create must be absent at target 2.2"
#endif
#ifdef MPI_T_BIND_NO_OBJECT
#error "MPI_T_BIND_NO_OBJECT must be absent at target 2.2"
#endif
#ifdef MPI_T_ERR_MEMORY
#error "MPI_T_ERR_MEMORY must be absent at target 2.2"
#endif

int main(void) {
    printf("STRICT_OK\n");
    return 0;
}
