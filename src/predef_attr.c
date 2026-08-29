/* src/predef_attr.c - predefined attribute callback VALUES.
 *
 * The 12 predefined attribute callbacks (MPI_*_DUP_FN / MPI_*_NULL_COPY_FN /
 * MPI_*_NULL_DELETE_FN) are not callable MPI functions and not uniformly
 * dlsym-able; each backend resolves them to its own compile-time value when the
 * library initializes (OpenMPI -> real OMPI_C_MPI_*_FN functions, MPICH-family
 * -> shared MPIR_Dup_fn plus NULL). They are exposed exactly like the extern
 * MPI_ERR_* ints: declared in a header, defined here (initially 0 / NULL), and
 * (re)assigned per runtime backend by the backend init routines.
 */
#include "unimpi_vtable.h"

MPI_Comm_copy_attr_function *MPI_COMM_DUP_FN = 0;
MPI_Comm_copy_attr_function *MPI_COMM_NULL_COPY_FN = 0;
MPI_Comm_delete_attr_function *MPI_COMM_NULL_DELETE_FN = 0;
MPI_Type_copy_attr_function *MPI_TYPE_DUP_FN = 0;
MPI_Type_copy_attr_function *MPI_TYPE_NULL_COPY_FN = 0;
MPI_Type_delete_attr_function *MPI_TYPE_NULL_DELETE_FN = 0;
MPI_Win_copy_attr_function *MPI_WIN_DUP_FN = 0;
MPI_Win_copy_attr_function *MPI_WIN_NULL_COPY_FN = 0;
MPI_Win_delete_attr_function *MPI_WIN_NULL_DELETE_FN = 0;
MPI_Copy_function *MPI_DUP_FN = 0;
MPI_Copy_function *MPI_NULL_COPY_FN = 0;
MPI_Delete_function *MPI_NULL_DELETE_FN = 0;
