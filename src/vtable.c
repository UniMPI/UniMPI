#include "unimpi_vtable.h"
#include "unimpi_loader.h"
#include "unimpi_platform.h"
#include "unimpi.h"
#include <stdlib.h>
#include <string.h>

/* Global vtable instance - initialized to zeros */
unimpi_vtable_t unimpi = {0};

/* MPI predefined values - filled at runtime */
MPI_Comm UNIMPI_COMM_WORLD = 0;
MPI_Comm UNIMPI_COMM_SELF = 0;
MPI_Info UNIMPI_INFO_NULL = 0;

/* MPI Datatypes - filled at runtime */
MPI_Datatype UNIMPI_CHAR = (MPI_Datatype)0x4c000101;
MPI_Datatype UNIMPI_SIGNED_CHAR = (MPI_Datatype)0x4c000118;
MPI_Datatype UNIMPI_UNSIGNED_CHAR = (MPI_Datatype)0x4c000102;
MPI_Datatype UNIMPI_BYTE = (MPI_Datatype)0x4c00010d;
MPI_Datatype UNIMPI_SHORT = (MPI_Datatype)0x4c000203;
MPI_Datatype UNIMPI_UNSIGNED_SHORT = (MPI_Datatype)0x4c000204;
MPI_Datatype UNIMPI_INT = (MPI_Datatype)0x4c000405;
MPI_Datatype UNIMPI_UNSIGNED = (MPI_Datatype)0x4c000406;
MPI_Datatype UNIMPI_LONG = (MPI_Datatype)0x4c000807;
MPI_Datatype UNIMPI_UNSIGNED_LONG = (MPI_Datatype)0x4c000808;
MPI_Datatype UNIMPI_FLOAT = (MPI_Datatype)0x4c00040a;
MPI_Datatype UNIMPI_DOUBLE = (MPI_Datatype)0x4c00080b;
MPI_Datatype UNIMPI_LONG_DOUBLE = (MPI_Datatype)0x4c00100c;
MPI_Datatype UNIMPI_LONG_LONG_INT = (MPI_Datatype)0x4c000809;
MPI_Datatype UNIMPI_LONG_LONG = (MPI_Datatype)0x4c000809;
MPI_Datatype UNIMPI_UNSIGNED_LONG_LONG = (MPI_Datatype)0x4c000819;

/* MPI Operations - filled at runtime */
MPI_Op UNIMPI_MAX = (MPI_Op)0x58000001;
MPI_Op UNIMPI_MIN = (MPI_Op)0x58000002;
MPI_Op UNIMPI_SUM = (MPI_Op)0x58000003;
MPI_Op UNIMPI_PROD = (MPI_Op)0x58000004;
MPI_Op UNIMPI_LAND = (MPI_Op)0x58000005;
MPI_Op UNIMPI_BAND = (MPI_Op)0x58000006;
MPI_Op UNIMPI_LOR = (MPI_Op)0x58000007;
MPI_Op UNIMPI_BOR = (MPI_Op)0x58000008;
MPI_Op UNIMPI_LXOR = (MPI_Op)0x58000009;
MPI_Op UNIMPI_BXOR = (MPI_Op)0x5800000a;
MPI_Op UNIMPI_MINLOC = (MPI_Op)0x5800000b;
MPI_Op UNIMPI_MAXLOC = (MPI_Op)0x5800000c;

static unimpi_lib_handle_t g_backend_handle = NULL;
static unimpi_backend_type_t g_backend_type = UNIMPI_BACKEND_UNKNOWN;

/* Forward declarations */
int unimpi_vtable_init_openmpi(unimpi_lib_handle_t handle);
int unimpi_vtable_init_mpich(unimpi_lib_handle_t handle);
int unimpi_vtable_init_intelmpi(unimpi_lib_handle_t handle);
int unimpi_vtable_init_msmpi(unimpi_lib_handle_t handle);

/* Get current backend type */
unimpi_backend_type_t unimpi_get_backend_type(void) {
    return g_backend_type;
}

/* Core symbol loading helper */
static void* load_symbol(unimpi_lib_handle_t handle, const char *name) {
    return unimpi_platform_dlsym(handle, name);
}

/* Validate required symbols exist */
int unimpi_vtable_validate_core(unimpi_lib_handle_t handle) {
    if (!load_symbol(handle, "MPI_Init") ||
        !load_symbol(handle, "MPI_Finalize") ||
        !load_symbol(handle, "MPI_Comm_size") ||
        !load_symbol(handle, "MPI_Comm_rank")) {
        return UNIMPI_ERR_SYMBOL_NOT_FOUND;
    }
    return UNIMPI_OK;
}

/* Initialize vtable based on backend type */
int unimpi_vtable_init(unimpi_lib_handle_t handle) {
    int ret;

    /* Validate core symbols */
    ret = unimpi_vtable_validate_core(handle);
    if (ret != UNIMPI_OK) {
        return ret;
    }

    /* Identify backend */
    g_backend_type = unimpi_loader_identify_backend(handle);
    g_backend_handle = handle;

    /* Initialize based on backend type */
    switch (g_backend_type) {
        case UNIMPI_BACKEND_OPENMPI:
            ret = unimpi_vtable_init_openmpi(handle);
            break;
        case UNIMPI_BACKEND_INTELMPI:
            ret = unimpi_vtable_init_intelmpi(handle);
            break;
        case UNIMPI_BACKEND_MSMPI:
            ret = unimpi_vtable_init_msmpi(handle);
            break;
        case UNIMPI_BACKEND_MPICH:
            ret = unimpi_vtable_init_mpich(handle);
            break;
        default:
            /* Try generic initialization - use MPICH style as default */
            ret = unimpi_vtable_init_mpich(handle);
            break;
    }

    /* Initialize error codes for the detected backend */
    if (ret == UNIMPI_OK) {
        unimpi_init_error_codes(g_backend_type);
    }

    return ret;
}

/* Cleanup vtable */
void unimpi_vtable_cleanup(void) {
    memset(&unimpi, 0, sizeof(unimpi_vtable_t));
    UNIMPI_COMM_WORLD = 0;
    UNIMPI_COMM_SELF = 0;
    g_backend_type = UNIMPI_BACKEND_UNKNOWN;
    g_backend_handle = NULL;
}
