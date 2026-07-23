/* test_environment_info.c - Environment, info, memory, op and status APIs */
#define UNIMPI_USE_STD_NAMES
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unimpi.h"
#include "test_mpi_helpers.h"

static void sum_ints(void *input, void *inout, int *length,
                     MPI_Datatype *datatype) {
    int *source = (int *)input;
    int *destination = (int *)inout;

    (void)datatype;
    for (int i = 0; i < *length; i++) {
        destination[i] += source[i];
    }
}

static int use_mpi31_optional(const char *operation, int available,
                              int required, int rank) {
    if (available) {
        return 1;
    }
    if (required) {
        fprintf(stderr, "%s is missing on a backend that requires the "
                        "complete tested communicator-info profile\n",
                operation);
        abort();
    }
    if (rank == 0) {
        printf("SKIP: %s is not exported by this MS-MPI runtime\n",
               operation);
    }
    return 0;
}

static void test_environment(int provided) {
    char library_version[UNIMPI_MAX_LIBRARY_VERSION_STRING];
    char processor_name[UNIMPI_MAX_PROCESSOR_NAME];
    int flag = 0;
    int length = 0;
    int queried = -1;
    int version = 0;
    int subversion = 0;

    TEST_CHECK_SUCCESS(MPI_Initialized(&flag));
    assert(flag == 1);

    TEST_CHECK_SUCCESS(MPI_Get_processor_name(processor_name, &length));
    assert(length >= 0);
    assert(length < UNIMPI_MAX_PROCESSOR_NAME);
    processor_name[length] = '\0';

    TEST_CHECK_SUCCESS(MPI_Get_version(&version, &subversion));
    assert(version >= 2);
    assert(subversion >= 0);

    TEST_CHECK_SUCCESS(
        MPI_Get_library_version(library_version, &length));
    assert(length > 0);
    assert(length < UNIMPI_MAX_LIBRARY_VERSION_STRING);
    library_version[length] = '\0';

    TEST_CHECK_SUCCESS(MPI_Query_thread(&queried));
    assert(queried == provided);
    assert(queried >= MPI_THREAD_SINGLE);
    assert(queried <= MPI_THREAD_MULTIPLE);

    TEST_CHECK_SUCCESS(MPI_Is_thread_main(&flag));
    assert(flag == 1);
}

static void test_info_and_communicators(int rank, int require_optional) {
    char key[MPI_MAX_INFO_KEY + 1];
    char name[MPI_MAX_OBJECT_NAME + 1];
    char value[MPI_MAX_INFO_VAL + 1];
    MPI_Comm duplicate;
    MPI_Comm shared;
    MPI_Info info;
    MPI_Info used;
    int flag = 0;
    int length = 0;
    int nkeys = 0;

    TEST_CHECK_SUCCESS(MPI_Info_create(&info));
    TEST_CHECK_SUCCESS(MPI_Info_set(info, "unimpi.test.key", "value"));
    TEST_CHECK_SUCCESS(
        MPI_Info_get(info, "unimpi.test.key", MPI_MAX_INFO_VAL,
                     value, &flag));
    assert(flag == 1);
    assert(strcmp(value, "value") == 0);

    TEST_CHECK_SUCCESS(MPI_Info_get_nkeys(info, &nkeys));
    assert(nkeys >= 1);
    TEST_CHECK_SUCCESS(MPI_Info_get_nthkey(info, 0, key));
    assert(key[0] != '\0');

    if (use_mpi31_optional(
            "MPI_Comm_dup_with_info",
            unimpi.comm_dup_with_info != NULL, require_optional, rank)) {
        TEST_CHECK_SUCCESS(
            MPI_Comm_dup_with_info(MPI_COMM_WORLD, info, &duplicate));
    } else {
        TEST_CHECK_SUCCESS(MPI_Comm_dup(MPI_COMM_WORLD, &duplicate));
    }
    TEST_CHECK_SUCCESS(MPI_Comm_set_name(duplicate, "unimpi-test-comm"));
    TEST_CHECK_SUCCESS(MPI_Comm_get_name(duplicate, name, &length));
    assert(length == (int)strlen("unimpi-test-comm"));
    assert(strcmp(name, "unimpi-test-comm") == 0);

    if (use_mpi31_optional(
            "MPI_Comm_get_info", unimpi.comm_get_info != NULL,
            require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Comm_get_info(duplicate, &used));
        TEST_CHECK_SUCCESS(MPI_Info_free(&used));
    }
    if (use_mpi31_optional(
            "MPI_Comm_set_info", unimpi.comm_set_info != NULL,
            require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Comm_set_info(duplicate, info));
    }

    if (use_mpi31_optional(
            "MPI_Comm_split_type", unimpi.comm_split_type != NULL,
            require_optional, rank)) {
        TEST_CHECK_SUCCESS(MPI_Comm_split_type(
            MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank,
            MPI_INFO_NULL, &shared));
        TEST_CHECK_SUCCESS(MPI_Comm_free(&shared));
    }
    TEST_CHECK_SUCCESS(MPI_Comm_free(&duplicate));

    TEST_CHECK_SUCCESS(MPI_Info_delete(info, "unimpi.test.key"));
    flag = 1;
    TEST_CHECK_SUCCESS(
        MPI_Info_get(info, "unimpi.test.key", MPI_MAX_INFO_VAL,
                     value, &flag));
    assert(flag == 0);
    TEST_CHECK_SUCCESS(MPI_Info_free(&info));
}

static void test_memory_operation_and_status(void) {
    MPI_Datatype datatype = MPI_INT;
    MPI_Op operation;
    MPI_Status status;
    void *memory = NULL;
    int cancelled = 0;
    int commute = 0;
    int elements = 0;

    TEST_CHECK_SUCCESS(MPI_Alloc_mem(64, MPI_INFO_NULL, &memory));
    assert(memory != NULL);
    memset(memory, 0x5a, 64);
    TEST_CHECK_SUCCESS(MPI_Free_mem(memory));

    TEST_CHECK_SUCCESS(MPI_Op_create(sum_ints, 1, &operation));
    TEST_CHECK_SUCCESS(MPI_Op_commutative(operation, &commute));
    assert(commute == 1);
    TEST_CHECK_SUCCESS(MPI_Op_free(&operation));

    memset(&status, 0, sizeof(status));
    TEST_CHECK_SUCCESS(MPI_Status_set_elements(&status, datatype, 3));
    TEST_CHECK_SUCCESS(MPI_Get_elements(&status, datatype, &elements));
    assert(elements == 3);
    TEST_CHECK_SUCCESS(MPI_Status_set_cancelled(&status, 1));
    TEST_CHECK_SUCCESS(MPI_Test_cancelled(&status, &cancelled));
    assert(cancelled == 1);
}

static void test_user_errors(void) {
    int error_class = 0;
    int error_code = 0;

    TEST_CHECK_SUCCESS(MPI_Add_error_class(&error_class));
    TEST_CHECK_SUCCESS(MPI_Add_error_code(error_class, &error_code));
    TEST_CHECK_SUCCESS(
        MPI_Add_error_string(error_code, "unimpi test error"));
}

int main(int argc, char **argv) {
    int finalized = 0;
    int provided = MPI_THREAD_SINGLE;
    int rank = 0;
    int require_optional;

    TEST_CHECK_SUCCESS(MPI_Init_thread(
        &argc, &argv, MPI_THREAD_FUNNELED, &provided));
    TEST_CHECK_SUCCESS(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    require_optional =
        unimpi_get_backend_type() != UNIMPI_BACKEND_MSMPI;

    test_environment(provided);
    test_info_and_communicators(rank, require_optional);
    test_memory_operation_and_status();
    test_user_errors();

    TEST_CHECK_SUCCESS(MPI_Finalize());
    TEST_CHECK_SUCCESS(MPI_Finalized(&finalized));
    assert(finalized == 1);

    if (rank == 0) {
        printf("Environment, info, memory, op and status scenarios passed\n");
    }
    return 0;
}
