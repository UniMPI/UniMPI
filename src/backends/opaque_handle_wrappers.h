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
 *
 * Quarantine (Class D — leave raw; document in PR4; not ABI-hardened):
 *   comm_spawn, comm_accept, comm_connect, comm_join, comm_get_parent,
 *   comm_disconnect, comm_get_info, remaining type_/group_/win_ constructors,
 *   errhandler OUT, File (pointer-width N/A for int conversion).
 * Integer ialltoallw stays NULL (Class E).
 */
#ifndef UNIMPI_OPAQUE_HANDLE_WRAPPERS_H
#define UNIMPI_OPAQUE_HANDLE_WRAPPERS_H

#include "unimpi_platform.h"
#include "unimpi_vtable.h"

/* Bind opaque OUT/INOUT/array adapters for an integer-handle backend.
 * Sole installer of every field it BIND_OPTIONALs.  Missing symbols leave
 * the corresponding unimpi slot NULL.  Open MPI must not call this.
 */
void unimpi_bind_integer_opaque_apis(unimpi_lib_handle_t handle);

/* Canonical facade conversion matching C conversion of native int to intptr_t. */
MPI_Comm unimpi_comm_from_native(int native);
int unimpi_comm_to_native(MPI_Comm facade);
MPI_Info unimpi_info_from_native(int native);
int unimpi_info_to_native(MPI_Info facade);
MPI_Datatype unimpi_datatype_from_native(int native);
int unimpi_datatype_to_native(MPI_Datatype facade);
MPI_Group unimpi_group_from_native(int native);
int unimpi_group_to_native(MPI_Group facade);
MPI_Win unimpi_win_from_native(int native);
int unimpi_win_to_native(MPI_Win facade);
MPI_Op unimpi_op_from_native(int native);
int unimpi_op_to_native(MPI_Op facade);

/* Wrappers installed by unimpi_bind_integer_opaque_apis (PR2 debt). */
int unimpi_wrap_comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int unimpi_wrap_comm_free(MPI_Comm *comm);
int unimpi_wrap_info_create(MPI_Info *info);
int unimpi_wrap_info_free(MPI_Info *info);
int unimpi_wrap_type_get_contents(
    MPI_Datatype datatype, int max_integers, int max_addresses,
    int max_datatypes, int *array_of_integers,
    MPI_Aint *array_of_addresses, MPI_Datatype *array_of_datatypes);
int unimpi_wrap_comm_spawn_multiple(
    int count, char *array_of_commands[], char **array_of_argv[],
    const int array_of_maxprocs[], const MPI_Info array_of_info[],
    int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);

/* Class C — matrix-exercised create/OUT/INOUT (frozen table, issue #2). */
int unimpi_wrap_comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);
int unimpi_wrap_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int unimpi_wrap_comm_split_type(MPI_Comm comm, int split_type, int key,
                                MPI_Info info, MPI_Comm *newcomm);
int unimpi_wrap_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
int unimpi_wrap_comm_group(MPI_Comm comm, MPI_Group *group);
int unimpi_wrap_type_contiguous(int count, MPI_Datatype oldtype,
                                MPI_Datatype *newtype);
int unimpi_wrap_type_vector(int count, int blocklength, int stride,
                            MPI_Datatype oldtype, MPI_Datatype *newtype);
int unimpi_wrap_type_indexed(int count, const int *array_of_blocklengths,
                             const int *array_of_displacements,
                             MPI_Datatype oldtype, MPI_Datatype *newtype);
int unimpi_wrap_type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype);
int unimpi_wrap_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb,
                                    MPI_Aint extent, MPI_Datatype *newtype);
int unimpi_wrap_type_commit(MPI_Datatype *datatype);
int unimpi_wrap_type_free(MPI_Datatype *datatype);
int unimpi_wrap_group_incl(MPI_Group group, int n, const int *ranks,
                           MPI_Group *newgroup);
int unimpi_wrap_group_excl(MPI_Group group, int n, const int *ranks,
                           MPI_Group *newgroup);
int unimpi_wrap_group_free(MPI_Group *group);
int unimpi_wrap_op_create(
    void (*user_fn)(void *, void *, int *, MPI_Datatype *), int commute,
    MPI_Op *op);
int unimpi_wrap_op_free(MPI_Op *op);
int unimpi_wrap_win_create(void *base, MPI_Aint size, int disp_unit,
                           MPI_Info info, MPI_Comm comm, MPI_Win *win);
int unimpi_wrap_win_free(MPI_Win *win);
int unimpi_wrap_intercomm_create(MPI_Comm local_comm, int local_leader,
                                 MPI_Comm peer_comm, int remote_leader, int tag,
                                 MPI_Comm *newintercomm);
int unimpi_wrap_intercomm_merge(MPI_Comm intercomm, int high,
                                MPI_Comm *newintracomm);

#endif /* UNIMPI_OPAQUE_HANDLE_WRAPPERS_H */
