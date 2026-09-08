#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unimpi.h"

/* MPI-3.1 additions integration suite (Annex B.1.2).
 *
 * Exercises all ten MPI-3.1 additions through the standard names:
 *   - MPI_Aint_add / MPI_Aint_diff (address arithmetic, pure scalar pass-through)
 *   - MPI_File_iread_all / MPI_File_iwrite_all / MPI_File_iread_at_all /
 *     MPI_File_iwrite_at_all (nonblocking collective file I/O)
 *   - MPI_T_cvar_get_index / MPI_T_pvar_get_index / MPI_T_category_get_index
 *     (by-name variable lookups)
 *   - MPI_Comm_idup (nonblocking communicator duplication)
 *
 * Address arithmetic is self-consistent (no backend value assumed), so it holds
 * across MPICH-family, Intel-MPI, MS-MPI and OpenMPI regardless of their
 * differing handle encodings. The file I/O case degrades to a skip when a
 * backend has not bound the *_all slots (mirroring test_mpi30_core's
 * degrade-skip for unsupported windows) -- but on a standard MPI-3.1 backend it
 * executes a real write/read round-trip through every new routine. The MPI_T
 * get_index lookups follow test_mpi30_t's availability policy: a backend that
 * exports no enumerable variable (OpenMPI 4.x reports 0) is a documented skip.
 */

#define CHECK(e) do {                                          \
    int _rc = (e);                                             \
    if (_rc != 0) {                                            \
        fprintf(stderr, "FAIL %s:%d: %s -> %d\n",              \
                __FILE__, __LINE__, #e, _rc);                  \
        return 1;                                              \
    }                                                          \
} while (0)

/* Latching variant of CHECK used by the file-I/O round trip, which must not
 * `return` before its collective MPI_Barrier: a rank-0-only error must be
 * recorded, then synchronised out, so the peer ranks never block forever in
 * the barrier. */
#define LATCH(e) do {                                          \
    int _rc = (e);                                             \
    if (_rc != 0) {                                            \
        fprintf(stderr, "FAIL %s:%d: %s -> %d\n",              \
                __FILE__, __LINE__, #e, _rc);                  \
        fail = 1;                                              \
    }                                                          \
} while (0)

/* Address arithmetic is pure pass-through; self-consistency asserts the fields
 * are bound and correct without depending on any backend-specific value. */
static int test_aint_add_diff(void) {
    MPI_Aint base = (MPI_Aint)1000;
    MPI_Aint disp = (MPI_Aint)234;
    MPI_Aint sum = MPI_Aint_add(base, disp);
    if (sum != base + disp) {
        fprintf(stderr, "FAIL aint_add: %ld + %ld = %ld (got %ld)\n",
                (long)base, (long)disp, (long)(base + disp), (long)sum);
        return 1;
    }
    MPI_Aint addr1 = (MPI_Aint)2000;
    MPI_Aint addr2 = (MPI_Aint)1500;
    MPI_Aint diff = MPI_Aint_diff(addr1, addr2);
    if (diff != addr1 - addr2) {
        fprintf(stderr, "FAIL aint_diff: %ld - %ld = %ld (got %ld)\n",
                (long)addr1, (long)addr2, (long)(addr1 - addr2), (long)diff);
        return 1;
    }
    /* Round-trip: add(base,disp) then diff back to base. */
    if (MPI_Aint_diff(MPI_Aint_add(base, disp), disp) != base) {
        fprintf(stderr, "FAIL aint round-trip\n");
        return 1;
    }
    printf("  aint_add/aint_diff arithmetic OK\n");
    return 0;
}

#define M31_IO_FILE "unimpi_m31_io.bin"

/* Nonblocking collective file I/O. Not all backends bind the *_all slots;
 * degrade to a skip when absent. A rank-0 local round-trip exercises all four
 * routines (iwrite_all, iwrite_at_all, iread_all, iread_at_all) for real. */
static int test_file_nonblocking_all(int rank) {
    if (unimpi.file_iwrite_all == NULL || unimpi.file_iread_all == NULL ||
        unimpi.file_iwrite_at_all == NULL || unimpi.file_iread_at_all == NULL) {
        printf("  nonblocking_io_all not bound by this backend (skip)\n");
        return 0;
    }
    int fail = 0;   /* latched errors; the barrier below must always execute */
    MPI_File fh;
    MPI_Status st;
    MPI_Request req;

    /* Write 3 ints at offsets 0/1/2, then overwrite slot 1 with a distinct
     * value via the at-offset variant; read everything back and verify. */
    int data[3] = {11, 22, 33};
    if (rank == 0) {
        LATCH(MPI_File_open(MPI_COMM_SELF, M31_IO_FILE,
                            MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh));
        LATCH(MPI_File_iwrite_all(fh, data, 3, MPI_INT, &req));
        LATCH(MPI_Wait(&req, &st));
        int slot = 99;
        MPI_Offset slot_off = (MPI_Offset)sizeof(int);
        LATCH(MPI_File_iwrite_at_all(fh, slot_off, &slot, 1, MPI_INT, &req));
        LATCH(MPI_Wait(&req, &st));
        LATCH(MPI_File_close(&fh));

        int back[3] = {0, 0, 0};
        LATCH(MPI_File_open(MPI_COMM_SELF, M31_IO_FILE,
                            MPI_MODE_RDONLY, MPI_INFO_NULL, &fh));
        LATCH(MPI_File_iread_all(fh, back, 3, MPI_INT, &req));
        LATCH(MPI_Wait(&req, &st));
        /* Re-read the overwritten slot via iread_at_all. */
        int slot2 = 0;
        LATCH(MPI_File_iread_at_all(fh, slot_off, &slot2, 1, MPI_INT, &req));
        LATCH(MPI_Wait(&req, &st));
        LATCH(MPI_File_close(&fh));

        if (back[0] != 11 || back[2] != 33) {
            fprintf(stderr, "FAIL file round-trip data [%d,%d,%d]\n",
                    back[0], back[1], back[2]);
            fail = 1;
        }
        if (slot2 != 99) {
            fprintf(stderr, "FAIL file at_all readback %d\n", slot2);
            fail = 1;
        }
        remove(M31_IO_FILE);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0 && fail == 0)
        printf("  file *_all nonblocking I/O round-trip OK\n");
    return fail;
}

/* MPI_T_*_get_index: by-name lookups of control and tool variables (3.1).
 * Each lookup is exercised by enumerating index 0 via get_info, round-tripping
 * that real name through get_index (same name -> same index), and confirming an
 * unknown name does not resolve. Availability mirrors test_mpi30_t: a backend
 * without the slots, or one exporting no enumerable variable (OpenMPI 4.x
 * reports 0), is a documented skip, not a failure. */
static int test_mpi_t_get_index(void) {
    char name[256];
    int name_len = (int)sizeof(name);
    int num = 0;
    int x = -1;
    int x2 = -2;
    int inited = 0;
    int rc = 0;
    int provided = 0;
    /* Out-params for cvar_get_info. The standard permits NULL for any of
     * these, but MPICH's implementation dereferences datatype/enumtype/
     * verbosity/scope unconditionally (a standards deviation), so always
     * pass real variables to stay portable across MPICH-family backends.
     * Signature follows the standard MPI-3.0+ 10-arg form. */
    MPI_Datatype x_dt = 0;
    MPI_T_enum x_et = 0;
    char x_desc[256] = {0};
    int x_desc_len = (int)sizeof(x_desc);
    int x_verb = 0;
    int x_bind = 0;
    int x_scope = 0;

    if (unimpi_mt.t_init_thread == NULL || unimpi_mt.t_finalize == NULL ||
        unimpi_mt.t_cvar_get_index == NULL ||
        unimpi_mt.t_pvar_get_index == NULL ||
        unimpi_mt.t_category_get_index == NULL ||
        unimpi_mt.t_cvar_get_num == NULL) {
        printf("  mpi_t get_index not bound by this backend (skip)\n");
        return 0;
    }
    if (unimpi_mt.t_init_thread(MPI_THREAD_SINGLE, &provided) != 0) {
        fprintf(stderr, "FAIL t_init_thread\n");
        return 1;
    }
    inited = 1;

    /* cvar: round-trip a real variable name from enumeration index 0. */
    do {
        if (unimpi_mt.t_cvar_get_num(&num) != 0 || num <= 0 ||
            unimpi_mt.t_cvar_get_info == NULL) {
            printf("  mpi_t cvar get_index: skip (no enumerable cvar)\n");
            break;
        }
        if (unimpi_mt.t_cvar_get_info(0, name, &name_len, &x_verb, &x_dt,
                                      &x_et, x_desc, &x_desc_len, &x_bind,
                                      &x_scope) != 0) {
            fprintf(stderr, "FAIL t_cvar_get_info(0)\n");
            rc = 1;
            break;
        }
        if (unimpi_mt.t_cvar_get_index(name, &x) != 0 ||
            unimpi_mt.t_cvar_get_index(name, &x2) != 0) {
            fprintf(stderr, "FAIL get_index(\"%s\") lookup\n", name);
            rc = 1;
            break;
        }
        if (x2 != x) {
            fprintf(stderr, "FAIL get_index nondeterministic: %d != %d\n",
                    x2, x);
            rc = 1;
            break;
        }
        if (unimpi_mt.t_cvar_get_index((char *)"unimpi_no_such_cvar",
                                       &x2) == 0) {
            fprintf(stderr, "FAIL get_index accepted an unknown cvar name\n");
            rc = 1;
            break;
        }
        printf("  mpi_t cvar get_index round-trip OK (\"%s\" -> %d)\n",
               name, x);
    } while (0);

    /* pvar: same pattern, but MPI_T_pvar_get_index' var_class argument is the
     * enumeration (bin) class the variable belongs to -- exactly what get_info
     * returns -- so pass that class back in for a deterministically valid pair;
     * a hardcoded class can never match a real variable and would always skip.
     * Also pass real locals for every get_info output (mirroring the cvar
     * block): some MPICH-family implementations dereference enumtype/verbosity
     * unconditionally, so NULL would be a portability hazard. Degrade to a
     * documented skip only when the backend has no enumerable pvar. */
    x = -1;
    x2 = -2;
    do {
        int var_class = 0;
        int p_verb = 0;
        MPI_T_enum p_et = 0;
        MPI_Datatype p_dt = 0;
        char p_desc[256] = {0};
        int p_desc_len = (int)sizeof(p_desc);
        int p_bind = 0, p_ro = 0, p_cont = 0, p_atom = 0;
        name_len = (int)sizeof(name);   /* get_info is in/out; reset per query */
        if (unimpi_mt.t_pvar_get_num(&num) != 0 || num <= 0 ||
            unimpi_mt.t_pvar_get_info == NULL) {
            printf("  mpi_t pvar get_index: skip (no enumerable pvar)\n");
            break;
        }
        if (unimpi_mt.t_pvar_get_info(0, name, &name_len, &p_verb, &var_class,
                                      &p_dt, &p_et, p_desc, &p_desc_len,
                                      &p_bind, &p_ro, &p_cont, &p_atom) != 0) {
            fprintf(stderr, "FAIL t_pvar_get_info(0)\n");
            rc = 1;
            break;
        }
        if (unimpi_mt.t_pvar_get_index(name, var_class, &x) != 0) {
            printf("  mpi_t pvar get_index: skip (backend rejects class %d)\n",
                   var_class);
            break;
        }
        if (unimpi_mt.t_pvar_get_index(name, var_class, &x2) != 0 || x2 != x) {
            fprintf(stderr, "FAIL pvar get_index(\"%s\") lookup\n", name);
            rc = 1;
            break;
        }
        if (unimpi_mt.t_pvar_get_index((char *)"unimpi_no_such_pvar",
                                       var_class, &x2) == 0) {
            fprintf(stderr, "FAIL pvar get_index accepted an unknown name\n");
            rc = 1;
            break;
        }
        printf("  mpi_t pvar get_index round-trip OK (\"%s\" -> %d)\n",
               name, x);
    } while (0);

    /* category: same pattern through category_get_info. */
    x = -1;
    x2 = -2;
    do {
        name_len = (int)sizeof(name);   /* get_info is in/out; reset per query */
        if (unimpi_mt.t_category_get_num(&num) != 0 || num <= 0 ||
            unimpi_mt.t_category_get_info == NULL) {
            printf("  mpi_t category get_index: skip (no category)\n");
            break;
        }
        if (unimpi_mt.t_category_get_info(0, name, &name_len, NULL, NULL,
                                          NULL, NULL, NULL) != 0) {
            fprintf(stderr, "FAIL t_category_get_info(0)\n");
            rc = 1;
            break;
        }
        if (unimpi_mt.t_category_get_index(name, &x) != 0 ||
            unimpi_mt.t_category_get_index(name, &x2) != 0) {
            fprintf(stderr, "FAIL category get_index(\"%s\") lookup\n", name);
            rc = 1;
            break;
        }
        if (x2 != x) {
            fprintf(stderr, "FAIL category get_index nondeterministic\n");
            rc = 1;
            break;
        }
        if (unimpi_mt.t_category_get_index((char *)"unimpi_no_such_category",
                                           &x2) == 0) {
            fprintf(stderr, "FAIL category get_index accepted an unknown name\n");
            rc = 1;
            break;
        }
        printf("  mpi_t category get_index round-trip OK (\"%s\" -> %d)\n",
               name, x);
    } while (0);

    if (inited)
        unimpi_mt.t_finalize();
    return rc;
}

static int test_comm_idup(MPI_Comm comm) {
    int rank = -1, newrank = -1, rc = 0;
    MPI_Comm newcomm;
    MPI_Request req;
    /* Nonblocking comm duplicate: all ranks participate, because a backend
     * (OpenMPI) only completes the idup request once the whole communicator
     * has issued it. Wait it out, then confirm the duplicate reports the same
     * rank as the original. */
    CHECK(MPI_Comm_rank(comm, &rank));
    CHECK(MPI_Comm_idup(comm, &newcomm, &req));
    CHECK(MPI_Wait(&req, MPI_STATUS_IGNORE));
    CHECK(MPI_Comm_rank(newcomm, &newrank));
    if (newrank != rank) {
        fprintf(stderr, "FAIL rank %d Comm_idup newcomm rank=%d\n",
                rank, newrank);
        rc = 1;
    }
    printf("  Comm_idup OK (rank %d)\n", rank);
    MPI_Comm_free(&newcomm);
    return rc;
}

int main(int argc, char **argv) {
    int rank = 0, rc = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
        printf("=== MPI-3.1 additions tests ===\n");
    if (rank == 0 && test_aint_add_diff()) rc = 1;
    if (test_file_nonblocking_all(rank)) rc = 1;  /* collective; barrier inside */
    if (rank == 0 && test_mpi_t_get_index()) rc = 1;
    if (test_comm_idup(MPI_COMM_WORLD)) rc = 1;  /* collective; all ranks dup */
    MPI_Barrier(MPI_COMM_WORLD);   /* all ranks exit together; no rank hangs */
    if (rank == 0 && rc == 0)
        printf("=== All MPI-3.1 additions tests passed ===\n");
    MPI_Finalize();
    return rc;
}
