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
 *
 * Class C (frozen): matrix-exercised create/OUT/INOUT opaque paths only.
 * Quarantine list: see opaque_handle_wrappers.h header comment.
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

MPI_Group unimpi_group_from_native(int native) {
    return (MPI_Group)(intptr_t)native;
}

int unimpi_group_to_native(MPI_Group facade) {
    return (int)(intptr_t)facade;
}

MPI_Win unimpi_win_from_native(int native) {
    return (MPI_Win)(intptr_t)native;
}

int unimpi_win_to_native(MPI_Win facade) {
    return (int)(intptr_t)facade;
}

MPI_Op unimpi_op_from_native(int native) {
    return (MPI_Op)(intptr_t)native;
}

int unimpi_op_to_native(MPI_Op facade) {
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

/* Class C */
typedef int (UNIMPI_MPI_CALL *native_comm_dup_with_info_fn)(
    unimpi_ih_comm_t, unimpi_ih_info_t, unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_comm_split_fn)(
    unimpi_ih_comm_t, int, int, unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_comm_split_type_fn)(
    unimpi_ih_comm_t, int, int, unimpi_ih_info_t, unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_comm_create_fn)(
    unimpi_ih_comm_t, unimpi_ih_group_t, unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_comm_group_fn)(
    unimpi_ih_comm_t, unimpi_ih_group_t *);
typedef int (UNIMPI_MPI_CALL *native_type_contiguous_fn)(
    int, unimpi_ih_datatype_t, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_type_vector_fn)(
    int, int, int, unimpi_ih_datatype_t, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_type_indexed_fn)(
    int, const int *, const int *, unimpi_ih_datatype_t, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_type_dup_fn)(
    unimpi_ih_datatype_t, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_type_create_resized_fn)(
    unimpi_ih_datatype_t, unimpi_ih_aint_t, unimpi_ih_aint_t, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_type_commit_fn)(unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_type_free_fn)(unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_group_incl_fn)(
    unimpi_ih_group_t, int, const int *, unimpi_ih_group_t *);
typedef int (UNIMPI_MPI_CALL *native_group_excl_fn)(
    unimpi_ih_group_t, int, const int *, unimpi_ih_group_t *);
typedef int (UNIMPI_MPI_CALL *native_group_free_fn)(unimpi_ih_group_t *);
/* Native Op_create user_fn sees int* datatype (not facade MPI_Datatype*). */
typedef void (UNIMPI_MPI_CALL *native_op_user_fn)(
    void *, void *, int *, unimpi_ih_datatype_t *);
typedef int (UNIMPI_MPI_CALL *native_op_create_fn)(
    native_op_user_fn, int, unimpi_ih_op_t *);
typedef int (UNIMPI_MPI_CALL *native_op_free_fn)(unimpi_ih_op_t *);
typedef int (UNIMPI_MPI_CALL *native_win_create_fn)(
    void *, unimpi_ih_aint_t, int, unimpi_ih_info_t, unimpi_ih_comm_t,
    unimpi_ih_win_t *);
typedef int (UNIMPI_MPI_CALL *native_win_free_fn)(unimpi_ih_win_t *);
typedef int (UNIMPI_MPI_CALL *native_intercomm_create_fn)(
    unimpi_ih_comm_t, int, unimpi_ih_comm_t, int, int, unimpi_ih_comm_t *);
typedef int (UNIMPI_MPI_CALL *native_intercomm_merge_fn)(
    unimpi_ih_comm_t, int, unimpi_ih_comm_t *);

/* ------------------------------------------------------------------------- */
/* Native storage                                                            */
/* ------------------------------------------------------------------------- */

static native_comm_dup_fn real_comm_dup;
static native_comm_free_fn real_comm_free;
static native_info_create_fn real_info_create;
static native_info_free_fn real_info_free;
static native_type_get_contents_fn real_type_get_contents;
static native_comm_spawn_multiple_fn real_comm_spawn_multiple;

static native_comm_dup_with_info_fn real_comm_dup_with_info;
static native_comm_split_fn real_comm_split;
static native_comm_split_type_fn real_comm_split_type;
static native_comm_create_fn real_comm_create;
static native_comm_group_fn real_comm_group;
static native_type_contiguous_fn real_type_contiguous;
static native_type_vector_fn real_type_vector;
static native_type_indexed_fn real_type_indexed;
static native_type_dup_fn real_type_dup;
static native_type_create_resized_fn real_type_create_resized;
static native_type_commit_fn real_type_commit;
static native_type_free_fn real_type_free;
static native_group_incl_fn real_group_incl;
static native_group_excl_fn real_group_excl;
static native_group_free_fn real_group_free;
static native_op_create_fn real_op_create;
static native_op_free_fn real_op_free;
static native_win_create_fn real_win_create;
static native_win_free_fn real_win_free;
static native_intercomm_create_fn real_intercomm_create;
static native_intercomm_merge_fn real_intercomm_merge;

/* ------------------------------------------------------------------------- */
/* Shared OUT / INOUT recipes (keep wrappers short and uniform)              */
/* ------------------------------------------------------------------------- */

/* OUT: store facade only on success. */
#define OPAQUE_OUT_CALL(out_ptr, native_var, from_native, call_expr)         \
    do {                                                                      \
        int _opaque_result;                                                   \
        if (!(out_ptr)) {                                                     \
            return arg_error();                                               \
        }                                                                     \
        (native_var) = 0;                                                     \
        _opaque_result = (call_expr);                                         \
        if (call_succeeded(_opaque_result)) {                                 \
            *(out_ptr) = (from_native)(native_var);                           \
        }                                                                     \
        return _opaque_result;                                                \
    } while (0)

/* INOUT free/commit: convert in, call, store native result on success. */
#define OPAQUE_INOUT_CALL(inout_ptr, native_type, to_native, from_native,    \
                          call_expr)                                          \
    do {                                                                      \
        native_type _opaque_native;                                           \
        int _opaque_result;                                                   \
        if (!(inout_ptr)) {                                                   \
            return arg_error();                                               \
        }                                                                     \
        _opaque_native = (native_type)(to_native)(*(inout_ptr));              \
        _opaque_result = (call_expr);                                         \
        if (call_succeeded(_opaque_result)) {                                 \
            *(inout_ptr) = (from_native)(_opaque_native);                     \
        }                                                                     \
        return _opaque_result;                                                \
    } while (0)

/* ------------------------------------------------------------------------- */
/* Wrappers — PR2 debt                                                       */
/* ------------------------------------------------------------------------- */

int unimpi_wrap_comm_dup(MPI_Comm comm, MPI_Comm *newcomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(newcomm, native, unimpi_comm_from_native,
                    real_comm_dup(unimpi_comm_to_native(comm), &native));
}

int unimpi_wrap_comm_free(MPI_Comm *comm) {
    OPAQUE_INOUT_CALL(comm, unimpi_ih_comm_t, unimpi_comm_to_native,
                      unimpi_comm_from_native, real_comm_free(&_opaque_native));
}

int unimpi_wrap_info_create(MPI_Info *info) {
    unimpi_ih_info_t native;
    OPAQUE_OUT_CALL(info, native, unimpi_info_from_native,
                    real_info_create(&native));
}

int unimpi_wrap_info_free(MPI_Info *info) {
    OPAQUE_INOUT_CALL(info, unimpi_ih_info_t, unimpi_info_to_native,
                      unimpi_info_from_native, real_info_free(&_opaque_native));
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
/* Wrappers — Class C frozen table                                           */
/* ------------------------------------------------------------------------- */

int unimpi_wrap_comm_dup_with_info(MPI_Comm comm, MPI_Info info,
                                   MPI_Comm *newcomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(
        newcomm, native, unimpi_comm_from_native,
        real_comm_dup_with_info(unimpi_comm_to_native(comm),
                                unimpi_info_to_native(info), &native));
}

int unimpi_wrap_comm_split(MPI_Comm comm, int color, int key,
                           MPI_Comm *newcomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(
        newcomm, native, unimpi_comm_from_native,
        real_comm_split(unimpi_comm_to_native(comm), color, key, &native));
}

int unimpi_wrap_comm_split_type(MPI_Comm comm, int split_type, int key,
                                MPI_Info info, MPI_Comm *newcomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(
        newcomm, native, unimpi_comm_from_native,
        real_comm_split_type(unimpi_comm_to_native(comm), split_type, key,
                             unimpi_info_to_native(info), &native));
}

int unimpi_wrap_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(
        newcomm, native, unimpi_comm_from_native,
        real_comm_create(unimpi_comm_to_native(comm),
                         unimpi_group_to_native(group), &native));
}

int unimpi_wrap_comm_group(MPI_Comm comm, MPI_Group *group) {
    unimpi_ih_group_t native;
    OPAQUE_OUT_CALL(
        group, native, unimpi_group_from_native,
        real_comm_group(unimpi_comm_to_native(comm), &native));
}

int unimpi_wrap_type_contiguous(int count, MPI_Datatype oldtype,
                                MPI_Datatype *newtype) {
    unimpi_ih_datatype_t native;
    OPAQUE_OUT_CALL(
        newtype, native, unimpi_datatype_from_native,
        real_type_contiguous(count, unimpi_datatype_to_native(oldtype),
                             &native));
}

int unimpi_wrap_type_vector(int count, int blocklength, int stride,
                            MPI_Datatype oldtype, MPI_Datatype *newtype) {
    unimpi_ih_datatype_t native;
    OPAQUE_OUT_CALL(
        newtype, native, unimpi_datatype_from_native,
        real_type_vector(count, blocklength, stride,
                         unimpi_datatype_to_native(oldtype), &native));
}

int unimpi_wrap_type_indexed(int count, const int *array_of_blocklengths,
                             const int *array_of_displacements,
                             MPI_Datatype oldtype, MPI_Datatype *newtype) {
    unimpi_ih_datatype_t native;
    OPAQUE_OUT_CALL(
        newtype, native, unimpi_datatype_from_native,
        real_type_indexed(count, array_of_blocklengths, array_of_displacements,
                          unimpi_datatype_to_native(oldtype), &native));
}

int unimpi_wrap_type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype) {
    unimpi_ih_datatype_t native;
    OPAQUE_OUT_CALL(
        newtype, native, unimpi_datatype_from_native,
        real_type_dup(unimpi_datatype_to_native(oldtype), &native));
}

int unimpi_wrap_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb,
                                    MPI_Aint extent, MPI_Datatype *newtype) {
    unimpi_ih_datatype_t native;
    OPAQUE_OUT_CALL(
        newtype, native, unimpi_datatype_from_native,
        real_type_create_resized(unimpi_datatype_to_native(oldtype),
                                 (unimpi_ih_aint_t)lb, (unimpi_ih_aint_t)extent,
                                 &native));
}

int unimpi_wrap_type_commit(MPI_Datatype *datatype) {
    OPAQUE_INOUT_CALL(datatype, unimpi_ih_datatype_t, unimpi_datatype_to_native,
                      unimpi_datatype_from_native,
                      real_type_commit(&_opaque_native));
}

int unimpi_wrap_type_free(MPI_Datatype *datatype) {
    OPAQUE_INOUT_CALL(datatype, unimpi_ih_datatype_t, unimpi_datatype_to_native,
                      unimpi_datatype_from_native,
                      real_type_free(&_opaque_native));
}

int unimpi_wrap_group_incl(MPI_Group group, int n, const int *ranks,
                           MPI_Group *newgroup) {
    unimpi_ih_group_t native;
    OPAQUE_OUT_CALL(
        newgroup, native, unimpi_group_from_native,
        real_group_incl(unimpi_group_to_native(group), n, ranks, &native));
}

int unimpi_wrap_group_excl(MPI_Group group, int n, const int *ranks,
                           MPI_Group *newgroup) {
    unimpi_ih_group_t native;
    OPAQUE_OUT_CALL(
        newgroup, native, unimpi_group_from_native,
        real_group_excl(unimpi_group_to_native(group), n, ranks, &native));
}

int unimpi_wrap_group_free(MPI_Group *group) {
    OPAQUE_INOUT_CALL(group, unimpi_ih_group_t, unimpi_group_to_native,
                      unimpi_group_from_native, real_group_free(&_opaque_native));
}

int unimpi_wrap_op_create(
    void (*user_fn)(void *, void *, int *, MPI_Datatype *), int commute,
    MPI_Op *op)
{
    unimpi_ih_op_t native;
    /* Pass user_fn through with native datatype pointer width (int*).
     * Callback ABI for user_fn is out of Class C scope; only OUT Op is wrapped. */
    OPAQUE_OUT_CALL(
        op, native, unimpi_op_from_native,
        real_op_create((native_op_user_fn)user_fn, commute, &native));
}

int unimpi_wrap_op_free(MPI_Op *op) {
    OPAQUE_INOUT_CALL(op, unimpi_ih_op_t, unimpi_op_to_native,
                      unimpi_op_from_native, real_op_free(&_opaque_native));
}

int unimpi_wrap_win_create(void *base, MPI_Aint size, int disp_unit,
                           MPI_Info info, MPI_Comm comm, MPI_Win *win) {
    unimpi_ih_win_t native;
    OPAQUE_OUT_CALL(
        win, native, unimpi_win_from_native,
        real_win_create(base, (unimpi_ih_aint_t)size, disp_unit,
                        unimpi_info_to_native(info), unimpi_comm_to_native(comm),
                        &native));
}

int unimpi_wrap_win_free(MPI_Win *win) {
    OPAQUE_INOUT_CALL(win, unimpi_ih_win_t, unimpi_win_to_native,
                      unimpi_win_from_native, real_win_free(&_opaque_native));
}

int unimpi_wrap_intercomm_create(MPI_Comm local_comm, int local_leader,
                                 MPI_Comm peer_comm, int remote_leader, int tag,
                                 MPI_Comm *newintercomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(
        newintercomm, native, unimpi_comm_from_native,
        real_intercomm_create(unimpi_comm_to_native(local_comm), local_leader,
                              unimpi_comm_to_native(peer_comm), remote_leader,
                              tag, &native));
}

int unimpi_wrap_intercomm_merge(MPI_Comm intercomm, int high,
                                MPI_Comm *newintracomm) {
    unimpi_ih_comm_t native;
    OPAQUE_OUT_CALL(
        newintracomm, native, unimpi_comm_from_native,
        real_intercomm_merge(unimpi_comm_to_native(intercomm), high, &native));
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
    /* PR2: primary debt + free companions + array paths */
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

    /* PR3: frozen Class C matrix-exercised OUT/INOUT (no expansion beyond). */
    BIND_OPTIONAL(comm_dup_with_info, unimpi_wrap_comm_dup_with_info,
                  native_comm_dup_with_info_fn, "MPI_Comm_dup_with_info");
    BIND_OPTIONAL(comm_split, unimpi_wrap_comm_split, native_comm_split_fn,
                  "MPI_Comm_split");
    BIND_OPTIONAL(comm_split_type, unimpi_wrap_comm_split_type,
                  native_comm_split_type_fn, "MPI_Comm_split_type");
    BIND_OPTIONAL(comm_create, unimpi_wrap_comm_create, native_comm_create_fn,
                  "MPI_Comm_create");
    BIND_OPTIONAL(comm_group, unimpi_wrap_comm_group, native_comm_group_fn,
                  "MPI_Comm_group");
    BIND_OPTIONAL(type_contiguous, unimpi_wrap_type_contiguous,
                  native_type_contiguous_fn, "MPI_Type_contiguous");
    BIND_OPTIONAL(type_vector, unimpi_wrap_type_vector, native_type_vector_fn,
                  "MPI_Type_vector");
    BIND_OPTIONAL(type_indexed, unimpi_wrap_type_indexed, native_type_indexed_fn,
                  "MPI_Type_indexed");
    BIND_OPTIONAL(type_dup, unimpi_wrap_type_dup, native_type_dup_fn,
                  "MPI_Type_dup");
    BIND_OPTIONAL(type_create_resized, unimpi_wrap_type_create_resized,
                  native_type_create_resized_fn, "MPI_Type_create_resized");
    BIND_OPTIONAL(type_commit, unimpi_wrap_type_commit, native_type_commit_fn,
                  "MPI_Type_commit");
    BIND_OPTIONAL(type_free, unimpi_wrap_type_free, native_type_free_fn,
                  "MPI_Type_free");
    BIND_OPTIONAL(group_incl, unimpi_wrap_group_incl, native_group_incl_fn,
                  "MPI_Group_incl");
    BIND_OPTIONAL(group_excl, unimpi_wrap_group_excl, native_group_excl_fn,
                  "MPI_Group_excl");
    BIND_OPTIONAL(group_free, unimpi_wrap_group_free, native_group_free_fn,
                  "MPI_Group_free");
    BIND_OPTIONAL(op_create, unimpi_wrap_op_create, native_op_create_fn,
                  "MPI_Op_create");
    BIND_OPTIONAL(op_free, unimpi_wrap_op_free, native_op_free_fn,
                  "MPI_Op_free");
    BIND_OPTIONAL(win_create, unimpi_wrap_win_create, native_win_create_fn,
                  "MPI_Win_create");
    BIND_OPTIONAL(win_free, unimpi_wrap_win_free, native_win_free_fn,
                  "MPI_Win_free");
    BIND_OPTIONAL(intercomm_create, unimpi_wrap_intercomm_create,
                  native_intercomm_create_fn, "MPI_Intercomm_create");
    BIND_OPTIONAL(intercomm_merge, unimpi_wrap_intercomm_merge,
                  native_intercomm_merge_fn, "MPI_Intercomm_merge");
}

#undef BIND_OPTIONAL
#undef OPAQUE_OUT_CALL
#undef OPAQUE_INOUT_CALL
