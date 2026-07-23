/* Test MS-MPI actual capability - direct call test */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unimpi.h"

static const char* err_name(int code) {
    if (code == MPI_SUCCESS) return "MPI_SUCCESS";
    if (code == MPI_ERR_SPAWN) return "MPI_ERR_SPAWN";
    if (code == MPI_ERR_SERVICE) return "MPI_ERR_SERVICE";
    if (code == MPI_ERR_NAME) return "MPI_ERR_NAME";
    if (code == MPI_ERR_PORT) return "MPI_ERR_PORT";
    if (code == MPI_ERR_UNSUPPORTED_OPERATION) return "MPI_ERR_UNSUPPORTED_OPERATION";
    static char buf[64];
    sprintf(buf, "code=%d", code);
    return buf;
}

/* Get child executable path from environment or default */
static const char* get_child_path(void) {
    const char *path = getenv("UNIMPI_SPAWN_CHILD_PATH");
    if (!path) {
        return "./test_spawn_child";
    }
    return path;
}

int main(int argc, char **argv) {
    int rank, ret;
    MPI_Comm intercomm, parent;
    MPI_Status status;
    int errcodes[1];
    char port_name[256];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank != 0) {
        MPI_Finalize();
        return 0;
    }

    printf("=== MS-MPI Actual Capability Test ===\n\n");

    /* 1. Port */
    printf("1. MPI_Open_port: ");
    ret = MPI_Open_port(MPI_INFO_NULL, port_name);
    printf("%s", err_name(ret));
    if (ret == MPI_SUCCESS) {
        printf(" (port=%.16s...)", port_name);
    }
    printf("\n");

    if (ret == MPI_SUCCESS) {
        printf("   MPI_Close_port: ");
        ret = MPI_Close_port(port_name);
        printf("%s\n", err_name(ret));
    }

    /* 2. Name service - SKIPPED on MS-MPI as it may abort on invalid names */
    printf("\n2. Name service tests: SKIPPED (may cause fatal abort on MS-MPI)\n");
    printf("   MPI_Publish_name, MPI_Lookup_name, MPI_Unpublish_name\n");
    printf("   require a valid published port and running nameserver.\n");

    /* 3. Spawn */
    printf("\n3. MPI_Comm_spawn (%s): ", get_child_path());
    char *args[] = {(char*)"--child", NULL};
    ret = MPI_Comm_spawn(get_child_path(), args, 1,
                         MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes);
    printf("%s", err_name(ret));
    if (ret == MPI_SUCCESS) {
        printf("\n   child_err=%d", errcodes[0]);
        int rbuf;
        MPI_Recv(&rbuf, 1, MPI_INT, 0, 0, intercomm, &status);
        printf(" recv=%d", rbuf);
        MPI_Comm_disconnect(&intercomm);
    }
    printf("\n");

    /* 4. Comm_get_parent */
    printf("\n4. MPI_Comm_get_parent: ");
    ret = MPI_Comm_get_parent(&parent);
    printf("%s", err_name(ret));
    if (ret == MPI_SUCCESS) {
        int is_null = (parent == (MPI_Comm)0);
        printf(" parent_is_null=%d", is_null);
    }
    printf("\n");

    printf("\n=== Done ===\n");
    MPI_Finalize();
    return 0;
}
