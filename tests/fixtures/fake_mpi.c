#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define FAKE_MPI_CALL __stdcall
typedef int fake_comm_t;
#else
#define FAKE_MPI_CALL
typedef void *fake_comm_t;
#endif

int ompi_mpi_comm_null;
int ompi_mpi_comm_world;
int ompi_mpi_comm_self;
int ompi_mpi_datatype_null;
int ompi_mpi_char;
int ompi_mpi_signed_char;
int ompi_mpi_unsigned_char;
int ompi_mpi_byte;
int ompi_mpi_short;
int ompi_mpi_unsigned_short;
int ompi_mpi_int;
int ompi_mpi_unsigned;
int ompi_mpi_long;
int ompi_mpi_unsigned_long;
int ompi_mpi_long_long_int;
int ompi_mpi_unsigned_long_long;
int ompi_mpi_float;
int ompi_mpi_double;
int ompi_mpi_long_double;
int ompi_mpi_op_null;
int ompi_mpi_op_sum;
int ompi_mpi_op_min;
int ompi_mpi_op_max;
int ompi_mpi_op_prod;
int ompi_mpi_op_band;
int ompi_mpi_op_bor;
int ompi_mpi_op_bxor;
int ompi_mpi_op_land;
int ompi_mpi_op_lor;
int ompi_mpi_op_lxor;
int ompi_mpi_op_minloc;
int ompi_mpi_op_maxloc;
int ompi_mpi_op_replace;
int ompi_mpi_op_no_op;
int ompi_request_null;
int ompi_mpi_info_null;
int ompi_mpi_win_null;

static int fake_initialized;

static int configured_result(const char *name) {
    const char *value = getenv(name);

    return value ? atoi(value) : 0;
}

int FAKE_MPI_CALL MPI_Init(int *argc, char ***argv) {
    int result;

    (void)argc;
    (void)argv;
    result = configured_result("UNIMPI_FAKE_INIT_RESULT");
    if (result == 0) {
        fake_initialized = 1;
    }
    return result;
}

int FAKE_MPI_CALL MPI_Init_thread(int *argc, char ***argv, int required,
                                  int *provided) {
    int result;

    (void)argc;
    (void)argv;
    if (provided) {
        *provided = required;
    }
    result = configured_result("UNIMPI_FAKE_INIT_THREAD_RESULT");
    if (result == 0) {
        fake_initialized = 1;
    }
    return result;
}

int FAKE_MPI_CALL MPI_Finalize(void) {
    int result = configured_result("UNIMPI_FAKE_FINALIZE_RESULT");

    if (result == 0) {
        fake_initialized = 0;
    }
    return result;
}

int FAKE_MPI_CALL MPI_Comm_size(fake_comm_t comm, int *size) {
    (void)comm;
    *size = 1;
    return 0;
}

int FAKE_MPI_CALL MPI_Comm_rank(fake_comm_t comm, int *rank) {
    (void)comm;
    *rank = 0;
    return 0;
}

int FAKE_MPI_CALL MPI_Get_processor_name(char *name, int *resultlen) {
    static const char value[] = "fake-host";

    if (!name || !resultlen) {
        return 13;
    }
    memcpy(name, value, sizeof(value));
    *resultlen = (int)sizeof(value) - 1;
    return 0;
}

int FAKE_MPI_CALL MPI_Get_version(int *version, int *subversion) {
    if (!version || !subversion) {
        return 13;
    }
    *version = 4;
    *subversion = 1;
    return 0;
}

int FAKE_MPI_CALL MPI_Get_library_version(char *version, int *resultlen) {
#ifdef _WIN32
    static const char value[] = "Microsoft MPI fake backend";
#else
    static const char value[] = "Open MPI fake backend";
#endif

    if (!version || !resultlen) {
        return 13;
    }
    memcpy(version, value, sizeof(value));
    *resultlen = (int)sizeof(value) - 1;
    return 0;
}

#ifdef _WIN32
int FAKE_MPI_CALL MSMPI_Get_version(void) {
    return 0x0a00;
}
#endif

int FAKE_MPI_CALL MPI_Error_class(int errorcode, int *error_class) {
    if (!fake_initialized &&
        configured_result("UNIMPI_FAKE_ABORT_PREINIT_ERROR_CLASS")) {
        abort();
    }
    if (!error_class) {
        return 13;
    }
    *error_class = errorcode;
    return 0;
}

#ifndef UNIMPI_FAKE_OMIT_SEND
int MPI_Send(void) {
    return 0;
}
#endif

#define FAKE_MPI_SYMBOL(name) \
    int name(void) { \
        return 0; \
    }
#include "fake_mpi_symbols.def"
#undef FAKE_MPI_SYMBOL
