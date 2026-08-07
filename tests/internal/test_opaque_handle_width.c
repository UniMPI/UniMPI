/* Regression: integer-handle backends bind full-width facade opaque handles
 * through the production binder against real native int ABI signatures.
 *
 * Paths exercised after each real MPICH / Intel / MS-MPI initializer against
 * a dedicated fake integer-handle DSO:
 *   Info_create full-width store
 *   Comm_dup full-width store
 *   Failed create leaves caller cell poisoned (0xff) untouched
 *   Info_free -> full-width UNIMPI_INFO_NULL
 *   Comm_free -> full-width from_native(FAKE_NATIVE_COMM_NULL)
 *   Type_get_contents N full-width datatype cells; neighbor poison untouched
 *   Comm_spawn_multiple root (info array converted; intercomm full-width)
 *   Comm_spawn_multiple non-root (count>0, array_of_info==NULL)
 *   Missing symbols leave opaque slots NULL
 *   ialltoallw remains NULL on integer binders
 *
 * Usage:
 *   test_opaque_handle_width <integer_opaque_api_fake> <missing_symbol_fake>
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_errors.h"
#include "unimpi_loader.h"
#include "unimpi_platform.h"
#include "unimpi_vtable.h"

enum {
    FAKE_NATIVE_COMM = (int)0xa1000001,
    FAKE_NATIVE_COMM_NULL = (int)0x04000000,
    FAKE_NATIVE_INFO = (int)0xb2000002,
    FAKE_NATIVE_INFO_NULL = (int)0x1c000000,
    FAKE_NATIVE_TYPE0 = (int)0xc3000003,
    FAKE_NATIVE_TYPE1 = (int)0xc4000004,
    FAKE_NATIVE_INTERCOMM = (int)0xd5000005,
    FAKE_ERR_OTHER = 15,
    FAKE_NATIVE_INFO_IN0 = (int)0xe1000010,
    FAKE_NATIVE_INFO_IN1 = (int)0xe2000020
};

int unimpi_vtable_init_mpich(unimpi_lib_handle_t handle);
int unimpi_vtable_init_intelmpi(unimpi_lib_handle_t handle);
int unimpi_vtable_init_msmpi(unimpi_lib_handle_t handle);

typedef int (UNIMPI_MPI_CALL *fake_set_fail_fn)(int);
typedef int (UNIMPI_MPI_CALL *fake_query_fn)(void);

static unimpi_lib_handle_t g_active_handle;

static MPI_Comm facade_comm(int native) {
    return (MPI_Comm)(intptr_t)native;
}

static MPI_Info facade_info(int native) {
    return (MPI_Info)(intptr_t)native;
}

static MPI_Datatype facade_datatype(int native) {
    return (MPI_Datatype)(intptr_t)native;
}

static void set_fail(const char *symbol, int enable) {
    fake_set_fail_fn fn;

    assert(g_active_handle != NULL);
    fn = (fake_set_fail_fn)unimpi_platform_dlsym(g_active_handle, symbol);
    assert(fn != NULL);
    assert(fn(enable) == 0);
}

static int query_fake(const char *symbol) {
    fake_query_fn fn;

    assert(g_active_handle != NULL);
    fn = (fake_query_fn)unimpi_platform_dlsym(g_active_handle, symbol);
    assert(fn != NULL);
    return fn();
}

static void assert_present_opaque_slots(const char *backend_name) {
    assert(unimpi.comm_dup != NULL);
    assert(unimpi.comm_free != NULL);
    assert(unimpi.info_create != NULL);
    assert(unimpi.info_free != NULL);
    assert(unimpi.type_get_contents != NULL);
    assert(unimpi.comm_spawn_multiple != NULL);
    assert(unimpi.ialltoallw == NULL);
    printf("  %s binder installed expected opaque slots (ialltoallw NULL)\n",
           backend_name);
}

static void assert_missing_opaque_slots_null(const char *backend_name) {
    assert(unimpi.comm_dup == NULL);
    assert(unimpi.comm_free == NULL);
    assert(unimpi.info_create == NULL);
    assert(unimpi.info_free == NULL);
    assert(unimpi.type_get_contents == NULL);
    assert(unimpi.comm_spawn_multiple == NULL);
    assert(unimpi.ialltoallw == NULL);
    printf("  %s missing optional opaque symbols remain NULL\n",
           backend_name);
}

static void test_info_create_full_width(void) {
    MPI_Info info = 0;

    assert(unimpi.info_create(&info) == 0);
    /* Full-width facade equality: high-nibble native constants sign-extend
     * into the upper bits of intptr_t; raw 4-byte stores leave garbage. */
    assert(info == facade_info(FAKE_NATIVE_INFO));
    assert(info == (MPI_Info)(intptr_t)FAKE_NATIVE_INFO);
    assert((int)(intptr_t)info == FAKE_NATIVE_INFO);
    printf("    Info_create full-width store passed\n");
}

static void test_comm_dup_full_width(void) {
    MPI_Comm newcomm = 0;

    assert(unimpi.comm_dup(facade_comm(1), &newcomm) == 0);
    assert(newcomm == facade_comm(FAKE_NATIVE_COMM));
    assert(newcomm == (MPI_Comm)(intptr_t)FAKE_NATIVE_COMM);
    assert((int)(intptr_t)newcomm == FAKE_NATIVE_COMM);
    printf("    Comm_dup full-width store passed\n");
}

static void test_failure_does_not_clobber(void) {
    MPI_Info info;
    MPI_Comm newcomm;
    unsigned char poison[sizeof(MPI_Info)];

    memset(poison, 0xff, sizeof(poison));
    memcpy(&info, poison, sizeof(info));
    set_fail("unimpi_fake_set_fail_next_info_create", 1);
    assert(unimpi.info_create(&info) == FAKE_ERR_OTHER);
    assert(memcmp(&info, poison, sizeof(info)) == 0);

    memset(poison, 0xff, sizeof(poison));
    memcpy(&newcomm, poison, sizeof(newcomm));
    set_fail("unimpi_fake_set_fail_next_comm_dup", 1);
    assert(unimpi.comm_dup(facade_comm(1), &newcomm) == FAKE_ERR_OTHER);
    assert(memcmp(&newcomm, poison, sizeof(newcomm)) == 0);

    printf("    failure paths leave caller cells unclobbered passed\n");
}

static void test_info_free_full_width_null(void) {
    MPI_Info info = facade_info(FAKE_NATIVE_INFO);

    assert(unimpi.info_free(&info) == 0);
    assert(info == UNIMPI_INFO_NULL);
    assert(info == facade_info(FAKE_NATIVE_INFO_NULL));
    printf("    Info_free full-width NULL passed\n");
}

static void test_comm_free_full_width_null(void) {
    MPI_Comm comm = facade_comm(FAKE_NATIVE_COMM);

    assert(unimpi.comm_free(&comm) == 0);
    assert(comm == facade_comm(FAKE_NATIVE_COMM_NULL));
    assert((int)(intptr_t)comm == FAKE_NATIVE_COMM_NULL);
    printf("    Comm_free full-width native COMM_NULL passed\n");
}

static void test_type_get_contents_neighbor_poison(void) {
    MPI_Datatype types[3];
    MPI_Datatype poison_neighbor;
    int integers[2];
    MPI_Aint addresses[2];
    unsigned char poison_bytes[sizeof(MPI_Datatype)];

    memset(poison_bytes, 0xab, sizeof(poison_bytes));
    memcpy(&poison_neighbor, poison_bytes, sizeof(poison_neighbor));
    types[0] = 0;
    types[1] = 0;
    types[2] = poison_neighbor;

    assert(unimpi.type_get_contents(
               facade_datatype(1), 2, 2, 2,
               integers, addresses, types) == 0);
    assert(types[0] == facade_datatype(FAKE_NATIVE_TYPE0));
    assert(types[1] == facade_datatype(FAKE_NATIVE_TYPE1));
    /* Neighbor cell must remain poison: proves no over-stride write. */
    assert(memcmp(&types[2], poison_bytes, sizeof(types[2])) == 0);
    assert(integers[0] == 10);
    assert(integers[1] == 11);
    assert(addresses[0] == (MPI_Aint)100);
    assert(addresses[1] == (MPI_Aint)101);

    /* Failure must not mutate facade datatype array. */
    types[0] = facade_datatype(0x11110001);
    types[1] = facade_datatype(0x22220002);
    types[2] = poison_neighbor;
    set_fail("unimpi_fake_set_fail_next_type_get_contents", 1);
    assert(unimpi.type_get_contents(
               facade_datatype(1), 0, 0, 2,
               NULL, NULL, types) == FAKE_ERR_OTHER);
    assert(types[0] == facade_datatype(0x11110001));
    assert(types[1] == facade_datatype(0x22220002));
    assert(memcmp(&types[2], poison_bytes, sizeof(types[2])) == 0);

    printf("    Type_get_contents full-width + neighbor poison passed\n");
}

static void test_spawn_multiple_root_info_array(void) {
    MPI_Info infos[2];
    MPI_Comm intercomm = 0;
    int maxprocs[2] = {1, 1};
    int errcodes[2] = {-1, -1};
    char *commands[2] = {"cmd0", "cmd1"};
    char **argv_null[2] = {NULL, NULL};

    infos[0] = facade_info(FAKE_NATIVE_INFO_IN0);
    infos[1] = facade_info(FAKE_NATIVE_INFO_IN1);

    assert(unimpi.comm_spawn_multiple(
               2, commands, argv_null, maxprocs, infos,
               0, facade_comm(1), &intercomm, errcodes) == 0);
    assert(intercomm == facade_comm(FAKE_NATIVE_INTERCOMM));
    assert((int)(intptr_t)intercomm == FAKE_NATIVE_INTERCOMM);
    assert(query_fake("unimpi_fake_last_spawn_info_was_null") == 0);
    assert(query_fake("unimpi_fake_last_spawn_count") == 2);
    assert(query_fake("unimpi_fake_last_spawn_info0") == FAKE_NATIVE_INFO_IN0);
    assert(query_fake("unimpi_fake_last_spawn_info1") == FAKE_NATIVE_INFO_IN1);
    assert(errcodes[0] == 0);
    assert(errcodes[1] == 0);
    printf("    Comm_spawn_multiple root info convert + intercomm passed\n");
}

static void test_spawn_multiple_nonroot_null_info(void) {
    MPI_Comm intercomm = 0;
    int maxprocs[1] = {1};
    int errcodes[1] = {-1};
    char *commands[1] = {"cmd0"};
    char **argv_null[1] = {NULL};

    /* Non-root shape: count > 0 with array_of_info == NULL must succeed
     * without adapter rejection; native sees NULL info pointer. */
    assert(unimpi.comm_spawn_multiple(
               1, commands, argv_null, maxprocs, NULL,
               1, facade_comm(1), &intercomm, errcodes) == 0);
    assert(intercomm == facade_comm(FAKE_NATIVE_INTERCOMM));
    assert(query_fake("unimpi_fake_last_spawn_info_was_null") == 1);
    assert(query_fake("unimpi_fake_last_spawn_count") == 1);
    assert(errcodes[0] == 0);
    printf("    Comm_spawn_multiple non-root NULL info array passed\n");
}

static void test_backend_on_integer_api(
    const char *path,
    int (*init_fn)(unimpi_lib_handle_t),
    const char *backend_name) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    g_active_handle = handle;
    assert(init_fn(handle) == UNIMPI_OK);
    assert_present_opaque_slots(backend_name);

    assert(MPI_ERR_ARG == 12);
    assert(MPI_ERR_NO_MEM == 34);
    assert(UNIMPI_INFO_NULL == facade_info(FAKE_NATIVE_INFO_NULL));

    printf("  %s production binder path checks...\n", backend_name);
    test_info_create_full_width();
    test_comm_dup_full_width();
    test_failure_does_not_clobber();
    test_info_free_full_width_null();
    test_comm_free_full_width_null();
    test_type_get_contents_neighbor_poison();
    test_spawn_multiple_root_info_array();
    test_spawn_multiple_nonroot_null_info();

    g_active_handle = NULL;
    unimpi_loader_unload(handle);
}

static void test_missing_symbols(
    const char *path,
    int (*init_fn)(unimpi_lib_handle_t),
    const char *backend_name) {
    unimpi_lib_handle_t handle = NULL;

    assert(unimpi_loader_load(path, &handle) == UNIMPI_OK);
    assert(init_fn(handle) == UNIMPI_OK);
    assert_missing_opaque_slots_null(backend_name);
    unimpi_loader_unload(handle);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <integer_opaque_api_fake> <missing_symbol_fake>\n",
                argv[0]);
        return 2;
    }

    printf("Running opaque-handle width regressions...\n");

    test_backend_on_integer_api(argv[1], unimpi_vtable_init_mpich, "MPICH");
    test_backend_on_integer_api(
        argv[1], unimpi_vtable_init_intelmpi, "Intel MPI");
    test_backend_on_integer_api(argv[1], unimpi_vtable_init_msmpi, "MS-MPI");

    printf("  Missing-symbol fixture checks...\n");
    test_missing_symbols(argv[2], unimpi_vtable_init_mpich, "MPICH");
    test_missing_symbols(argv[2], unimpi_vtable_init_intelmpi, "Intel MPI");
    test_missing_symbols(argv[2], unimpi_vtable_init_msmpi, "MS-MPI");

    printf("Opaque-handle width regressions passed\n");
    return 0;
}
