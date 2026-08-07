/*
 * Opaque-handle adapters for MPICH, Intel MPI, and MS-MPI.
 *
 * Facade handles are intptr_t. Integer backends use native C types:
 *   int for Comm, Info, Datatype, Group, Win, Op, Errhandler
 *   struct ADIOI_FileD * for File
 *   intptr_t-sized Aint on supported 64-bit ABIs
 *
 * PR1 skeleton: native typedefs, conversion helpers, BIND_OPTIONAL macro,
 * and empty unimpi_bind_integer_opaque_apis (zero field ownership).
 * Wrappers and BIND_OPTIONAL field installs land in later PRs together with
 * deletion of the matching raw dlsym assigns in mpich/intelmpi/msmpi.
 */
#include "opaque_handle_wrappers.h"

#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Integer-backend native ABI aliases (mirror request_handle_wrappers.c)     */
/* ------------------------------------------------------------------------- */

typedef int unimpi_ih_comm_t;
typedef int unimpi_ih_info_t;
typedef int unimpi_ih_datatype_t;
typedef int unimpi_ih_group_t;
typedef int unimpi_ih_win_t;
typedef int unimpi_ih_op_t;
typedef int unimpi_ih_errhandler_t;
struct ADIOI_FileD;
typedef struct ADIOI_FileD *unimpi_ih_file_t;
/* LP64: matches MPI_Aint as pointer-sized integer across the three backends. */
typedef intptr_t unimpi_ih_aint_t;

/* ------------------------------------------------------------------------- */
/* Canonical opaque-handle conversion                                        */
/* ------------------------------------------------------------------------- */

MPI_Comm unimpi_comm_from_native(int native) {
    return (MPI_Comm)(intptr_t)native;
}

int unimpi_comm_to_native(MPI_Comm facade) {
    return (int)(intptr_t)facade;
}

MPI_Info unimpi_info_from_native(int native) {
    return (MPI_Info)(intptr_t)native;
}

int unimpi_info_to_native(MPI_Info facade) {
    return (int)(intptr_t)facade;
}

MPI_Datatype unimpi_datatype_from_native(int native) {
    return (MPI_Datatype)(intptr_t)native;
}

int unimpi_datatype_to_native(MPI_Datatype facade) {
    return (int)(intptr_t)facade;
}

/* ------------------------------------------------------------------------- */
/* Binding                                                                   */
/* ------------------------------------------------------------------------- */

/* Ready for PR2+ field installs.  Sole installer of every field it binds:
 * when a field is added here, the matching raw dlsym assign must be deleted
 * from mpich.c / intelmpi.c / msmpi.c in the same change. */
#define BIND_OPTIONAL(field, wrap, native_type, symbol)                        \
    do {                                                                      \
        native_type fn =                                                      \
            (native_type)unimpi_platform_dlsym(handle, symbol);               \
        if (fn) {                                                             \
            real_##field = fn;                                                \
            unimpi.field = wrap;                                              \
        } else {                                                              \
            real_##field = NULL;                                              \
            unimpi.field = NULL;                                              \
        }                                                                     \
    } while (0)

void unimpi_bind_integer_opaque_apis(unimpi_lib_handle_t handle) {
    /* PR1: empty — zero field ownership.  Raw dlsym lines in integer
     * backends remain sole installers until a later PR BIND_OPTIONALs a
     * field and deletes the matching raw assign in the same change. */
    (void)handle;
    /* (void)BIND_OPTIONAL: macro is defined for PR2+; not used yet. */
}

#undef BIND_OPTIONAL
