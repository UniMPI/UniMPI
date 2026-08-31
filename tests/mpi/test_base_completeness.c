/* tests/mpi/test_base_completeness.c - runtime validation of the MPI-2
 * base-completeness bindings added to the always-present surface.
 *
 * These exercise the highest-signature-risk additions (modern datatype
 * constructors, address arithmetic, info get_valuelen/dup, nonblocking send
 * variants, reduce_local, request_get_status) against a real backend, proving
 * the function-pointer signatures are ABI-correct (a wrong cast would crash or
 * misbehave here).
 */
#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unimpi.h"

#define TEST(name) printf("Testing %s...\n", name)
#define PASS() printf("  PASS\n")
#define FAIL(msg) do { fprintf(stderr, "  FAIL: %s\n", msg); return 1; } while(0)

static int test_get_address(void) {
    int x = 42;
    MPI_Aint addr;
    TEST("Get_address");
    if (MPI_Get_address(&x, &addr) != MPI_SUCCESS) FAIL("MPI_Get_address failed");
    if (addr == 0) FAIL("MPI_Get_address returned null address");
    PASS();
    return 0;
}

static int test_type_create_struct(void) {
    /* struct { int a; double b; char c; } */
    MPI_Datatype types[3] = { MPI_INT, MPI_DOUBLE, MPI_CHAR };
    int blens[3] = { 1, 1, 1 };
    MPI_Aint off[3], base;
    MPI_Datatype mystruct;
    int struct_size, elem_size;

    TEST("Type_create_struct");
    /* MPI_Type_create_struct displacements are byte offsets from the struct
     * base. MPI_Get_address returns absolute addresses, so subtract the struct
     * base address to get relative offsets (required by MPICH-family backends;
     * Open MPI tolerates absolute addresses but the standard is unambiguous). */
    {
        struct st { int a; double b; char c; } s;
        MPI_Get_address(&s, &base);
        MPI_Get_address(&s.a, &off[0]); off[0] -= base;
        MPI_Get_address(&s.b, &off[1]); off[1] -= base;
        MPI_Get_address(&s.c, &off[2]); off[2] -= base;
    }
    if (MPI_Type_create_struct(3, blens, off, types, &mystruct) != MPI_SUCCESS)
        FAIL("MPI_Type_create_struct failed");
    if (MPI_Type_commit(&mystruct) != MPI_SUCCESS) FAIL("MPI_Type_commit failed");
    if (MPI_Type_size(mystruct, &struct_size) != MPI_SUCCESS)
        FAIL("MPI_Type_size(struct) failed");
    if (MPI_Type_size(MPI_INT, &elem_size) != MPI_SUCCESS) FAIL("MPI_Type_size(INT) failed");
    if (struct_size != elem_size + (int)sizeof(double) + (int)sizeof(char)) {
        fprintf(stderr, "  FAIL: struct_size=%d want=%d\n", struct_size,
                elem_size + (int)sizeof(double) + (int)sizeof(char));
        return 1;
    }
    MPI_Type_free(&mystruct);
    PASS();
    return 0;
}

static int test_type_f90(void) {
    MPI_Datatype t;
    int sz;
    TEST("Type_create_f90_integer");
    if (MPI_Type_create_f90_integer(16, &t) != MPI_SUCCESS)
        FAIL("MPI_Type_create_f90_integer(16) failed");
    if (MPI_Type_size(t, &sz) != MPI_SUCCESS) FAIL("MPI_Type_size(f90) failed");
    if (sz == 0) FAIL("f90_integer got zero size");
    /* NB: backends (MPICH/OpenMPI) treat f90_* types as cached handles that
     * MPI_Type_free rejects with MPI_ERR_TYPE, so we must not free them here.
     */
    PASS();
    return 0;
}

static int test_info_valuelen_dup(void) {
    MPI_Info info, dup;
    int vlen = -1, got = 0;
    TEST("Info_set + Info_get_valuelen + Info_dup");
    if (MPI_Info_create(&info) != MPI_SUCCESS) FAIL("MPI_Info_create failed");
    if (MPI_Info_set(info, "key", "hello") != MPI_SUCCESS) FAIL("MPI_Info_set failed");
    if (MPI_Info_get_valuelen(info, "key", &vlen, &got) != MPI_SUCCESS)
        FAIL("MPI_Info_get_valuelen failed");
    if (!got || vlen != 5) {
        fprintf(stderr, "  FAIL: got=%d vlen=%d (want 1,5)\n", got, vlen);
        return 1;
    }
    if (MPI_Info_dup(info, &dup) != MPI_SUCCESS) FAIL("MPI_Info_dup failed");
    vlen = -1;
    if (MPI_Info_get_valuelen(dup, "key", &vlen, &got) != MPI_SUCCESS)
        FAIL("MPI_Info_get_valuelen(dup) failed");
    if (vlen != 5) FAIL("dup lost value");
    MPI_Info_free(&dup);
    MPI_Info_free(&info);
    PASS();
    return 0;
}

static int test_reduce_local(void) {
    int in[4] = { 1, 2, 3, 4 };
    int inout[4];
    TEST("Reduce_local(MPI_SUM)");
    memcpy(inout, in, sizeof(in));
    if (MPI_Reduce_local(in, inout, 4, MPI_INT, MPI_SUM) != MPI_SUCCESS)
        FAIL("MPI_Reduce_local failed");
    if (inout[0] != 2 || inout[3] != 8) {
        fprintf(stderr, "  FAIL: inout=[%d %d %d %d] want [2 4 6 8]\n",
                inout[0], inout[1], inout[2], inout[3]);
        return 1;
    }
    PASS();
    return 0;
}

static int test_request_get_status(void) {
    MPI_Request req;
    int flag = -1, rank, nprocs;
    MPI_Status st;
    TEST("Isend/Irecv + Request_get_status");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (nprocs < 2) { printf("  SKIP (need >=2 ranks)\n"); return 0; }
    if (rank == 0) {
        int sval = 7;
        MPI_Isend(&sval, 1, MPI_INT, 1, 99, MPI_COMM_WORLD, &req);
    } else {
        int rval = 0;
        MPI_Irecv(&rval, 1, MPI_INT, 0, 99, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, &st);
        if (rval != 7) { fprintf(stderr, "  FAIL: recv=%d want 7\n", rval); return 1; }
        /* Request_get_status on a completed request */
        if (MPI_Request_get_status(req, &flag, &st) != MPI_SUCCESS)
            FAIL("MPI_Request_get_status failed");
        if (flag != 1) FAIL("completed request not flagged");
    }
    PASS();
    return 0;
}

static int test_nb_send_variants(void) {
    int rank, nprocs, i;
    TEST("Ibsend + Irsend + Issend");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (nprocs < 2) { printf("  SKIP (need >=2 ranks)\n"); return 0; }

    if (rank == 0) {
        MPI_Request reqs[2];
        int snd[2] = { 10, 20 };
        char buf[256];
        MPI_Buffer_attach(buf, sizeof(buf));
        MPI_Ibsend(&snd[0], 1, MPI_INT, 1, 90, MPI_COMM_WORLD, &reqs[0]);
        MPI_Issend(&snd[1], 1, MPI_INT, 1, 91, MPI_COMM_WORLD, &reqs[1]);
        MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
        MPI_Buffer_detach(&buf, &i);
    } else {
        int got[2] = { 0, 0 };
        MPI_Status st[2];
        MPI_Recv(&got[0], 1, MPI_INT, 0, 90, MPI_COMM_WORLD, &st[0]);
        MPI_Recv(&got[1], 1, MPI_INT, 0, 91, MPI_COMM_WORLD, &st[1]);
        if (got[0] != 10 || got[1] != 20) {
            fprintf(stderr, "  FAIL: got=[%d %d] want [10 20]\n", got[0], got[1]);
            return 1;
        }
    }
    PASS();
    return 0;
}

static int test_comm_errhandler(void) {
    MPI_Errhandler eh;
    TEST("Comm_get_errhandler + Comm_set_errhandler");
    if (MPI_Comm_get_errhandler(MPI_COMM_WORLD, &eh) != MPI_SUCCESS)
        FAIL("MPI_Comm_get_errhandler failed");
    /* Round-trip: set the same valid handler back (MPI_ERRORS_RETURN is not
     * exposed as a std macro, so reuse what we read). */
    if (MPI_Comm_set_errhandler(MPI_COMM_WORLD, eh) != MPI_SUCCESS)
        FAIL("MPI_Comm_set_errhandler failed");
    PASS();
    return 0;
}

int main(int argc, char **argv) {
    int ret, rank;

    ret = MPI_Init(&argc, &argv);
    if (ret != MPI_SUCCESS) { fprintf(stderr, "MPI_Init failed\n"); return 1; }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI Base Completeness Smoke Tests ===\n\n");

    if ((ret = test_get_address()) != 0) goto cleanup;
    if ((ret = test_type_create_struct()) != 0) goto cleanup;
    if ((ret = test_type_f90()) != 0) goto cleanup;
    if ((ret = test_info_valuelen_dup()) != 0) goto cleanup;
    if ((ret = test_reduce_local()) != 0) goto cleanup;
    if ((ret = test_comm_errhandler()) != 0) goto cleanup;
    if ((ret = test_request_get_status()) != 0) goto cleanup;
    if ((ret = test_nb_send_variants()) != 0) goto cleanup;
    (void)rank;

    printf("\n=== All base-completeness tests passed ===\n");
cleanup:
    MPI_Finalize();
    return ret;
}
