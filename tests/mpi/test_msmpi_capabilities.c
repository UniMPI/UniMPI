/* Test MS-MPI dynamic process capabilities */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { printf("  FAIL: %s\n", msg); } while(0)
#define SKIP(reason) do { printf("  SKIP: %s\n", reason); } while(0)

/* MS-MPI specific constants */
#ifndef MPI_COMM_NULL
#define MPI_COMM_NULL ((MPI_Comm)0)
#endif

int main(int argc, char **argv) {
    int ret;
    int rank;
    char port_name[256];
    MPI_Comm intercomm;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) {
        fprintf(stderr, "Failed to initialize MPI: %d\n", ret);
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("=== MS-MPI Capability Test ===\n");
    printf("Running on rank %d\n\n", rank);

    /* Test 1: MPI_Open_Port */
    TEST("MPI_Open_port");
    ret = MPI_Open_port(MPI_INFO_NULL, port_name);
    if (ret == MPI_SUCCESS) {
        PASS();
        printf("  Port name: %s\n", port_name);

        /* Test 2: MPI_Close_port */
        TEST("MPI_Close_port");
        ret = MPI_Close_port(port_name);
        if (ret == MPI_SUCCESS) {
            PASS();
        } else {
            FAIL("MPI_Close_port failed");
        }
    } else {
        FAIL("MPI_Open_port failed");
    }

    /* Test 3: MPI_Publish_name */
    TEST("MPI_Publish_name");
    ret = MPI_Open_port(MPI_INFO_NULL, port_name);
    if (ret == MPI_SUCCESS) {
        ret = MPI_Publish_name("test_service", MPI_INFO_NULL, port_name);
        if (ret == MPI_SUCCESS) {
            PASS();
            MPI_Unpublish_name("test_service", MPI_INFO_NULL, port_name);
        } else {
            /* Expected to fail on MS-MPI */
            printf("  Error code: %d\n", ret);
        }
        MPI_Close_port(port_name);
    }

    /* Test 4: MPI_Comm_spawn */
    TEST("MPI_Comm_spawn");
    int errcodes[1];
    char *spawn_argv[] = {NULL};

    /* Try to spawn a simple process */
    ret = MPI_Comm_spawn("test_spawn_child", spawn_argv, 1, MPI_INFO_NULL, 0,
                         MPI_COMM_WORLD, &intercomm, errcodes);
    if (ret == MPI_SUCCESS) {
        PASS();
        MPI_Comm_disconnect(&intercomm);
    } else {
        /* Expected to fail or be limited on MS-MPI */
        printf("  Error code: %d\n", ret);
        if (ret == MPI_ERR_SPAWN) {
            SKIP("MPI_Comm_spawn not fully supported on MS-MPI");
        } else {
            FAIL("Unexpected error");
        }
    }

    /* Test 5: MPI_Comm_get_parent */
    TEST("MPI_Comm_get_parent");
    MPI_Comm parent;
    ret = MPI_Comm_get_parent(&parent);
    if (ret == MPI_SUCCESS) {
        if (parent == MPI_COMM_NULL) {
            PASS();
            printf("  No parent (expected for non-spawned process)\n");
        } else {
            PASS();
            printf("  Has parent communicator\n");
        }
    } else {
        FAIL("MPI_Comm_get_parent failed");
    }

    printf("\n=== Test Complete ===\n");
    MPI_Finalize();
    return 0;
}
