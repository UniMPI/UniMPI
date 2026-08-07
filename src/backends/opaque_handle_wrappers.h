/* Opaque-handle ABI adapters for integer-handle MPI backends.
 *
 * UniMPI facade handles are intptr_t.  MPICH, Intel MPI, and MS-MPI use native
 * int for Comm/Info/Datatype/Group/Win/Op (and pointer-width File).  Adapters
 * convert by-value handles at the call boundary, use local native temps for
 * pointer arguments that write handles, and store full-width facade values
 * only when the native call defines the output.
 *
 * Request/Message adapters remain in request_handle_wrappers.h.
 * Datatype-array Alltoallw remains in datatype_array_wrappers.h.
 * Open MPI keeps direct pointer-handle assignment in its backend.
 *
 * Sole installer of every vtable field it BIND_OPTIONALs.  Missing symbols
 * leave the corresponding unimpi slot NULL.  Open MPI must not call this.
 */
#ifndef UNIMPI_OPAQUE_HANDLE_WRAPPERS_H
#define UNIMPI_OPAQUE_HANDLE_WRAPPERS_H

#include "unimpi_platform.h"
#include "unimpi_vtable.h"

/* Bind opaque OUT/INOUT/array adapters for an integer-handle backend.
 * Sole installer of every field it BIND_OPTIONALs.  Missing symbols leave
 * the corresponding unimpi slot NULL.  Open MPI must not call this.
 *
 * PR1: empty body (zero field ownership).  Later PRs add BIND_OPTIONAL
 * installs and delete matching raw dlsym assigns in the same change.
 */
void unimpi_bind_integer_opaque_apis(unimpi_lib_handle_t handle);

/* Canonical facade conversion matching C conversion of native int to intptr_t. */
MPI_Comm unimpi_comm_from_native(int native);
int unimpi_comm_to_native(MPI_Comm facade);
MPI_Info unimpi_info_from_native(int native);
int unimpi_info_to_native(MPI_Info facade);
MPI_Datatype unimpi_datatype_from_native(int native);
int unimpi_datatype_to_native(MPI_Datatype facade);

#endif /* UNIMPI_OPAQUE_HANDLE_WRAPPERS_H */
