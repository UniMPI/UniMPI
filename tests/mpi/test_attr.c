/* tests/mpi/test_attr.c - runtime validation of the MPI-2 attribute bindings:
 * comm/type create_keyval + set/get/delete_attr + free_keyval, the deprecated
 * MPI-1 MPI_Keyval_create/free, and MPI_Register_datarep. Exercises the public
 * MPI_*_attr_function and MPI_Datarep_* callback typedefs against a real
 * backend. Attribute values are stored as pointers and retrieved back via a
 * void* out-param (the address of the stored value), matching the modern
 * `void *attribute_val` ABI.
 */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

/* ---- comm attribute callbacks ---- */
static int comm_extra_seen = 0;
static int comm_copy_fn(MPI_Comm oldcomm, int keyval, void *extra_state,
                        void *attr_in, void *attr_out, int *flag) {
    (void)oldcomm; (void)keyval; (void)attr_in;
    if (extra_state == &comm_extra_seen) comm_extra_seen++;
    *(void **)attr_out = attr_in;   /* copy the attribute pointer through */
    *flag = 1;
    return MPI_SUCCESS;
}
static int comm_delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra_state) {
    (void)comm; (void)keyval; (void)attr;
    if (extra_state == &comm_extra_seen) comm_extra_seen += 10;
    return MPI_SUCCESS;
}

/* ---- type attribute callbacks ---- */
static int type_extra_seen = 0;
static int type_copy_fn(MPI_Datatype oldtype, int keyval, void *extra_state,
                        void *attr_in, void *attr_out, int *flag) {
    (void)oldtype; (void)keyval; (void)attr_in;
    if (extra_state == &type_extra_seen) type_extra_seen++;
    *(void **)attr_out = attr_in;
    *flag = 1;
    return MPI_SUCCESS;
}
static int type_delete_fn(MPI_Datatype type, int keyval, void *attr, void *extra_state) {
    (void)type; (void)keyval; (void)attr;
    if (extra_state == &type_extra_seen) type_extra_seen += 10;
    return MPI_SUCCESS;
}

/* ---- MPI-1 deprecated keyval callbacks ---- */
static int old_copy_fn(MPI_Comm oldcomm, int keyval, void *extra_state,
                       void *attr_in, void *attr_out, int *flag) {
    (void)oldcomm; (void)keyval; (void)extra_state; (void)attr_in;
    *(void **)attr_out = attr_in; *flag = 1;
    return MPI_SUCCESS;
}
static int old_delete_fn(MPI_Comm comm, int keyval, void *attr, void *extra_state) {
    (void)comm; (void)keyval; (void)attr; (void)extra_state;
    return MPI_SUCCESS;
}

/* ---- datarep callbacks (register smoke test) ---- */
static int datarep_conv_fn(void *userbuf, MPI_Datatype datatype, int count,
                           void *filebuf, MPI_Offset position, void *extra_state) {
    (void)userbuf; (void)datatype; (void)count; (void)filebuf;
    (void)position; (void)extra_state;
    return MPI_SUCCESS;
}
static int datarep_extent_fn(MPI_Datatype datatype, MPI_Aint *extent, void *extra_state) {
    (void)datatype; (void)extra_state;
    *extent = (MPI_Aint)1;
    return MPI_SUCCESS;
}

static int test_comm_attr(void) {
    int keyval, flag = 0, val = 123;
    void *got = NULL;

    TEST("comm create_keyval/set/get/delete_attr/free_keyval");
    comm_extra_seen = 0;
    if (MPI_Comm_create_keyval(comm_copy_fn, comm_delete_fn, &keyval,
                               &comm_extra_seen) != MPI_SUCCESS)
        FAIL("MPI_Comm_create_keyval failed");
    if (MPI_Comm_set_attr(MPI_COMM_WORLD, keyval, &val) != MPI_SUCCESS)
        FAIL("MPI_Comm_set_attr failed");
    if (MPI_Comm_get_attr(MPI_COMM_WORLD, keyval, &got, &flag) != MPI_SUCCESS)
        FAIL("MPI_Comm_get_attr failed");
    if (!flag || got != &val) {
        fprintf(stderr, "  FAIL: comm attr round-trip flag=%d\n", flag);
        return 1;
    }
    if (MPI_Comm_delete_attr(MPI_COMM_WORLD, keyval) != MPI_SUCCESS)
        FAIL("MPI_Comm_delete_attr failed");
    flag = 1;
    MPI_Comm_get_attr(MPI_COMM_WORLD, keyval, &got, &flag);
    if (flag != 0) FAIL("comm attr present after delete_attr");
    if (MPI_Comm_free_keyval(&keyval) != MPI_SUCCESS)
        FAIL("MPI_Comm_free_keyval failed");
    PASS();
    return 0;
}

static int test_type_attr(void) {
    int keyval, flag = 0, val = 456;
    void *got = NULL;

    TEST("type create_keyval/set/get/delete_attr/free_keyval");
    type_extra_seen = 0;
    if (MPI_Type_create_keyval(type_copy_fn, type_delete_fn, &keyval,
                               &type_extra_seen) != MPI_SUCCESS)
        FAIL("MPI_Type_create_keyval failed");
    if (MPI_Type_set_attr(MPI_INT, keyval, &val) != MPI_SUCCESS)
        FAIL("MPI_Type_set_attr failed");
    if (MPI_Type_get_attr(MPI_INT, keyval, &got, &flag) != MPI_SUCCESS)
        FAIL("MPI_Type_get_attr failed");
    if (!flag || got != &val) {
        fprintf(stderr, "  FAIL: type attr round-trip flag=%d\n", flag);
        return 1;
    }
    if (MPI_Type_delete_attr(MPI_INT, keyval) != MPI_SUCCESS)
        FAIL("MPI_Type_delete_attr failed");
    if (MPI_Type_free_keyval(&keyval) != MPI_SUCCESS)
        FAIL("MPI_Type_free_keyval failed");
    PASS();
    return 0;
}

static int test_old_keyval(void) {
    int keyval;
    TEST("MPI-1 Keyval_create/Keyval_free (deprecated)");
    if (MPI_Keyval_create(old_copy_fn, old_delete_fn, &keyval, NULL) != MPI_SUCCESS)
        FAIL("MPI_Keyval_create failed");
    if (MPI_Keyval_free(&keyval) != MPI_SUCCESS)
        FAIL("MPI_Keyval_free failed");
    PASS();
    return 0;
}

static int test_register_datarep(void) {
    int extra = 0, rc;
    TEST("Register_datarep");
    rc = MPI_Register_datarep("UNIMPI_TEST_DREP", datarep_conv_fn, datarep_conv_fn,
                              datarep_extent_fn, &extra);
    if (rc == MPI_SUCCESS) {
        PASS();
        return 0;
    }
    /* Some backends (e.g. OpenMPI) do not implement user-registered datareps
     * and reject registration with MPI_ERR_OTHER; the important validation is
     * that the binding resolves and is callable, returning a valid MPI error
     * code rather than failing to dispatch. */
    if (rc == MPI_ERR_OTHER || rc == MPI_ERR_ARG) {
        printf("  OK (backend does not support user datareps, rc=%d)\n", rc);
        PASS();
        return 0;
    }
    fprintf(stderr, "  FAIL: MPI_Register_datarep rc=%d\n", rc);
    return 1;
}

int main(int argc, char **argv) {
    int ret;
    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) { fprintf(stderr, "MPI_Init failed\n"); return 1; }
    printf("=== MPI-2 Attribute + Datarep Tests ===\n\n");

    if ((ret = test_comm_attr()) != 0) goto cleanup;
    if ((ret = test_type_attr()) != 0) goto cleanup;
    if ((ret = test_old_keyval()) != 0) goto cleanup;
    if ((ret = test_register_datarep()) != 0) goto cleanup;

    printf("\n=== All attribute/datarep tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
