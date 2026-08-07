/*
 * Opaque-handle adapters for MPICH, Intel MPI, and MS-MPI.
 *
 * Facade handles are intptr_t. Integer backends use native C types:
 *   int for Comm, Info, Datatype, Group, Win, Op, Errhandler
 *   struct ADIOI_FileD * for File
 *   intptr_t-sized Aint on supported 64-bit ABIs
 *
 * Every by-value handle is converted at the call boundary so dlsym-bound
 * function types match the real library, not the UniMPI facade aliases.
 * Array OUT paths allocate native-width temps and convert only on success.
 */
#include "opaque_handle_wrappers.h"
#include "unimpi.h"

#include <stdint.h>
#include <stdlib.h>

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
/* Error helpers (same ternary pattern as request_handle_wrappers.c)         */
/* ------------------------------------------------------------------------- */

static int arg_error(void) {
    return MPI_ERR_ARG != MPI_SUCCESS ? MPI_ERR_ARG : 12;
}

static int no_memory_error(void) {
    return MPI_ERR_NO_MEM != MPI_SUCCESS ? MPI_ERR_NO_MEM : 39;
}

static int call_succeeded(int result) {
    return result == MPI_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* Native function pointer types (integer-backend ABI)                       */
/* ------------------------------------------------------------------------- */

typedef int (UNIMPI_MPI_CALL *native_comm_dup_fn)(
    unimpi_ih_comm_t, unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_comm_free_fn)(unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_info_create_fn)(unimpi_ih_info_t *);
typedef int (UNIMPI_MPI_CALL *native_info_free_fn)(unimpi_ih_info_t *);
typedef int (UNIMPI_MPI_CALL *native_type_get_contents_fn)(
    unimpi_ih_datatype_t, int, int, int,
    int *, unimpi_ih_aint_t *, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_comm_spawn_multiple_fn)(
    int, char *[], char **[], const int [],
    const unimpi_ih_info_t [], int, unimpi_ih_comm_t,
    unimpi_ih_comm_t *, int []);

/* ------------------------------------------------------------------------- */
/* Native storage                                                            */
/* ------------------------------------------------------------------------- */

static native_comm_dup_fn real_comm_dup;
static native_comm_free_fn real_comm_free;
static native_info_create_fn real_info_create;
static native_info_free_fn real_info_free;
static native_type_get_contents_fn real_type_get_contents;
static native_comm_spawn_multiple_fn real_comm_spawn_multiple;

/* ------------------------------------------------------------------------- */
/* Wrappers                                                                  */
/* ------------------------------------------------------------------------- */

int unimpi_wrap_comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
    unimpi_ih_comm_t native = 0;
    int result;

    if (!newcomm) {
        return arg_error();
    }
    result = real_comm_dup(unimpi_comm_to_native(comm), &native);
    if (call_succeeded(result)) {
        *newcomm = unimpi_comm_from_native(native);
    }
    return result;
}

int unimpi_wrap_comm_free(MPI_Comm *comm) {
    unimpi_ih_comm_t native;
    int result;

    if (!comm) {
        return arg_error();
    }
    native = unimpi_comm_to_native(*comm);
    result = real_comm_free(&native);
    if (call_succeeded(result)) {
        *comm = unimpi_comm_from_native(native);
    }
    return result;
}

int unimpi_wrap_info_create(MPI_Info *info) {
    unimpi_ih_info_t native = 0;
    int result;

    if (!info) {
        return arg_error();
    }
    result = real_info_create(&native);
    if (call_succeeded(result)) {
        *info = unimpi_info_from_native(native);
    }
    return result;
}

int unimpi_wrap_info_free(MPI_Info *info) {
    unimpi_ih_info_t native;
    int result;

    if (!info) {
        return arg_error();
    }
    native = unimpi_info_to_native(*info);
    result = real_info_free(&native);
    if (call_succeeded(result)) {
        *info = unimpi_info_from_native(native);
    }
    return result;
}

int unimpi_wrap_type_get_contents(
    MPI_Datatype datatype,
    int max_integers, int max_addresses, int max_datatypes,
    int *array_of_integers, MPI_Aint *array_of_addresses,
    MPI_Datatype *array_of_datatypes)
{
    unimpi_ih_datatype_t *native_types = NULL;
    int result;
    int i;

    if (max_datatypes < 0) {
        return arg_error();
    }
    if (max_datatypes > 0) {
        if (!array_of_datatypes) {
            return arg_error();
        }
        native_types = (unimpi_ih_datatype_t *)calloc(
            (size_t)max_datatypes, sizeof(*native_types));
        if (!native_types) {
            return no_memory_error();
        }
    }

    /* array_of_integers is int* on both sides — pass through.
     * MPI_Aint is intptr_t on supported ABIs; cast to unimpi_ih_aint_t *.
     * NULL sub-arrays when max_integers/max_addresses == 0 are passed through.
     * max_datatypes == 0: native_types stays NULL. */
    result = real_type_get_contents(
        unimpi_datatype_to_native(datatype),
        max_integers, max_addresses, max_datatypes,
        array_of_integers,
        (unimpi_ih_aint_t *)array_of_addresses,
        native_types);

    if (call_succeeded(result) && native_types) {
        for (i = 0; i < max_datatypes; ++i) {
            array_of_datatypes[i] =
                unimpi_datatype_from_native(native_types[i]);
        }
    }
    free(native_types);
    return result;
}

int unimpi_wrap_comm_spawn_multiple(
    int count, char *array_of_commands[], char **array_of_argv[],
    const int array_of_maxprocs[], const MPI_Info array_of_info[],
    int root, MPI_Comm comm, MPI_Comm *intercomm,
    int array_of_errcodes[])
{
    unimpi_ih_info_t *native_infos = NULL;
    unimpi_ih_comm_t native_inter = 0;
    int result;
    int i;

    /* Convert info array only when the caller provided one (root path).
     * Non-root may pass array_of_info == NULL with count > 0 — pass NULL.
     * commands / argv / maxprocs are also root-only and pass through. */
    if (array_of_info != NULL) {
        if (count < 0) {
            return arg_error();
        }
        if (count > 0) {
            native_infos = (unimpi_ih_info_t *)calloc(
                (size_t)count, sizeof(*native_infos));
            if (!native_infos) {
                return no_memory_error();
            }
            for (i = 0; i < count; ++i) {
                native_infos[i] = unimpi_info_to_native(array_of_info[i]);
            }
        }
    }
    /* count < 0 with NULL info: pass through to native for error class */

    result = real_comm_spawn_multiple(
        count, array_of_commands, array_of_argv, array_of_maxprocs,
        native_infos, /* may be NULL for non-root */
        root, unimpi_comm_to_native(comm),
        intercomm ? &native_inter : NULL,
        array_of_errcodes);

    if (call_succeeded(result) && intercomm) {
        *intercomm = unimpi_comm_from_native(native_inter);
    }
    free(native_infos);
    return result;
}

/* ------------------------------------------------------------------------- */
/* Binding                                                                   */
/* ------------------------------------------------------------------------- */

/* Sole installer of every field it binds: when a field is added here, the
 * matching raw dlsym assign must be deleted from mpich.c / intelmpi.c /
 * msmpi.c in the same change. */
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
    BIND_OPTIONAL(comm_dup, unimpi_wrap_comm_dup, native_comm_dup_fn,
                  "MPI_Comm_dup");
    BIND_OPTIONAL(comm_free, unimpi_wrap_comm_free, native_comm_free_fn,
                  "MPI_Comm_free");
    BIND_OPTIONAL(info_create, unimpi_wrap_info_create, native_info_create_fn,
                  "MPI_Info_create");
    BIND_OPTIONAL(info_free, unimpi_wrap_info_free, native_info_free_fn,
                  "MPI_Info_free");
    BIND_OPTIONAL(type_get_contents, unimpi_wrap_type_get_contents,
                  native_type_get_contents_fn, "MPI_Type_get_contents");
    BIND_OPTIONAL(comm_spawn_multiple, unimpi_wrap_comm_spawn_multiple,
                  native_comm_spawn_multiple_fn, "MPI_Comm_spawn_multiple");
}

#undef BIND_OPTIONAL
