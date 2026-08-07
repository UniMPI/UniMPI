/* Fake integer-handle MPI DSO for opaque OUT/INOUT/array binder regressions.
 *
 * Exports real MPICH / Intel MPI / MS-MPI native C signatures (int handles)
 * for the PR2 debt symbols plus core symbols required by production inits:
 *   MPI_Init, MPI_Finalize, MPI_Comm_size, MPI_Comm_rank, MPI_Error_class
 *   MPI_Comm_dup, MPI_Comm_free, MPI_Info_create, MPI_Info_free
 *   MPI_Type_get_contents, MPI_Comm_spawn_multiple
 *
 * High-nibble native constants prove full-width facade store (not 4-byte
 * truncation luck on little-endian).
 */
#include <stdint.h>
#include <string.h>

#include "unimpi_platform.h"

enum {
    FAKE_NATIVE_COMM = (int)0xa1000001,
    FAKE_NATIVE_COMM_NULL = (int)0x04000000,
    FAKE_NATIVE_INFO = (int)0xb2000002,
    FAKE_NATIVE_INFO_NULL = (int)0x1c000000,
    FAKE_NATIVE_TYPE0 = (int)0xc3000003,
    FAKE_NATIVE_TYPE1 = (int)0xc4000004,
    FAKE_NATIVE_INTERCOMM = (int)0xd5000005,
    FAKE_ERR_OTHER = 15
};

static int g_fail_next_comm_dup;
static int g_fail_next_info_create;
static int g_fail_next_type_get_contents;
static int g_fail_next_spawn_multiple;
static int g_last_spawn_info_was_null;
static int g_last_spawn_info0;
static int g_last_spawn_info1;
static int g_last_spawn_count;

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_comm_dup(int enable) {
    g_fail_next_comm_dup = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_info_create(int enable) {
    g_fail_next_info_create = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_type_get_contents(int enable) {
    g_fail_next_type_get_contents = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_set_fail_next_spawn_multiple(int enable) {
    g_fail_next_spawn_multiple = enable ? 1 : 0;
    return 0;
}

int UNIMPI_MPI_CALL unimpi_fake_last_spawn_info_was_null(void) {
    return g_last_spawn_info_was_null;
}

int UNIMPI_MPI_CALL unimpi_fake_last_spawn_info0(void) {
    return g_last_spawn_info0;
}

int UNIMPI_MPI_CALL unimpi_fake_last_spawn_info1(void) {
    return g_last_spawn_info1;
}

int UNIMPI_MPI_CALL unimpi_fake_last_spawn_count(void) {
    return g_last_spawn_count;
}

/* --- Core symbols required by vtable validation --- */

int UNIMPI_MPI_CALL MPI_Init(int *argc, char ***argv) {
    (void)argc;
    (void)argv;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Finalize(void) {
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_size(int comm, int *size) {
    (void)comm;
    if (size) {
        *size = 1;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_rank(int comm, int *rank) {
    (void)comm;
    if (rank) {
        *rank = 0;
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Error_class(int errorcode, int *error_class) {
    if (!error_class) {
        return 13;
    }
    *error_class = errorcode & 0xff;
    return 0;
}

/* --- Opaque debt symbols (native int handles) --- */

int UNIMPI_MPI_CALL MPI_Comm_dup(int comm, int *newcomm) {
    (void)comm;
    if (!newcomm) {
        return 12;
    }
    if (g_fail_next_comm_dup) {
        g_fail_next_comm_dup = 0;
        *newcomm = (int)0xdeadbeef;
        return FAKE_ERR_OTHER;
    }
    *newcomm = FAKE_NATIVE_COMM;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_free(int *comm) {
    if (!comm) {
        return 12;
    }
    *comm = FAKE_NATIVE_COMM_NULL;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Info_create(int *info) {
    if (!info) {
        return 12;
    }
    if (g_fail_next_info_create) {
        g_fail_next_info_create = 0;
        *info = (int)0xdeadbeef;
        return FAKE_ERR_OTHER;
    }
    *info = FAKE_NATIVE_INFO;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Info_free(int *info) {
    if (!info) {
        return 12;
    }
    *info = FAKE_NATIVE_INFO_NULL;
    return 0;
}

int UNIMPI_MPI_CALL MPI_Type_get_contents(
    int datatype, int max_integers, int max_addresses, int max_datatypes,
    int *array_of_integers, intptr_t *array_of_addresses, int *array_of_datatypes)
{
    int i;

    (void)datatype;
    if (g_fail_next_type_get_contents) {
        g_fail_next_type_get_contents = 0;
        if (array_of_datatypes && max_datatypes > 0) {
            array_of_datatypes[0] = (int)0xdeadbeef;
        }
        return FAKE_ERR_OTHER;
    }

    if (max_integers > 0 && array_of_integers) {
        for (i = 0; i < max_integers; ++i) {
            array_of_integers[i] = 10 + i;
        }
    }
    if (max_addresses > 0 && array_of_addresses) {
        for (i = 0; i < max_addresses; ++i) {
            array_of_addresses[i] = (intptr_t)(100 + i);
        }
    }
    if (max_datatypes > 0) {
        if (!array_of_datatypes) {
            return 12;
        }
        for (i = 0; i < max_datatypes; ++i) {
            array_of_datatypes[i] =
                (i == 0) ? FAKE_NATIVE_TYPE0 : FAKE_NATIVE_TYPE1 + (i - 1);
        }
        if (max_datatypes >= 1) {
            array_of_datatypes[0] = FAKE_NATIVE_TYPE0;
        }
        if (max_datatypes >= 2) {
            array_of_datatypes[1] = FAKE_NATIVE_TYPE1;
        }
    }
    return 0;
}

int UNIMPI_MPI_CALL MPI_Comm_spawn_multiple(
    int count, char *array_of_commands[], char **array_of_argv[],
    const int array_of_maxprocs[], const int array_of_info[],
    int root, int comm, int *intercomm, int array_of_errcodes[])
{
    int i;

    (void)array_of_commands;
    (void)array_of_argv;
    (void)array_of_maxprocs;
    (void)root;
    (void)comm;

    g_last_spawn_count = count;
    g_last_spawn_info_was_null = (array_of_info == NULL) ? 1 : 0;
    g_last_spawn_info0 = 0;
    g_last_spawn_info1 = 0;
    if (array_of_info != NULL && count > 0) {
        g_last_spawn_info0 = array_of_info[0];
        if (count > 1) {
            g_last_spawn_info1 = array_of_info[1];
        }
    }

    if (g_fail_next_spawn_multiple) {
        g_fail_next_spawn_multiple = 0;
        if (intercomm) {
            *intercomm = (int)0xdeadbeef;
        }
        return FAKE_ERR_OTHER;
    }

    if (intercomm) {
        *intercomm = FAKE_NATIVE_INTERCOMM;
    }
    if (array_of_errcodes && count > 0) {
        for (i = 0; i < count; ++i) {
            array_of_errcodes[i] = 0;
        }
    }
    return 0;
}
