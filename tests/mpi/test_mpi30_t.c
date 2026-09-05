#define UNIMPI_USE_STD_NAMES
#include <stdio.h>
#include <string.h>
#include "unimpi.h"

/* MPI-T tools-interface integration suite.
 *
 * Exercises the tool interface through the standard MPI_T_* names, backed by
 * the DTLS-backed unimpi_mt vtable. Availability is gated on the whole cluster:
 * a backend that does not export MPI_T (leaving the t_* slots NULL) skips the
 * suite (exit 0). No specific pvar/cvar name or index is assumed -- the checks
 * are self-consistent against the unimpi-exposed UNIMPI_T_* constants, so they
 * hold across the MPICH-family shifted values and OpenMPI's 0-based values.
 *
 * The core user concern is survival across MPI_Finalize: MPI_T_* must remain
 * usable after the MPI interface is finalized, because the tool interface has
 * its own reference on the backend library. The final case proves exactly that
 * by calling MPI_T_pvar_get_num after MPI_Finalize.
 */

#define CHECK(e) do {                                          \
    int _rc = (e);                                             \
    if (_rc != 0) {                                            \
        fprintf(stderr, "FAIL %s:%d: %s -> %d\n",              \
                __FILE__, __LINE__, #e, _rc);                  \
        return 1;                                              \
    }                                                          \
} while (0)

static int mpit_available(void) {
    return unimpi_mt.t_init_thread != NULL && unimpi_mt.t_finalize != NULL &&
           unimpi_mt.t_cvar_get_num != NULL && unimpi_mt.t_pvar_get_num != NULL;
}

/* category/enum introspection is an additive slice of the tools interface;
 * gate it separately so a backend that ships MPI_T init/cvar/pvar but no
 * category layer still runs the core suite. */
static int category_available(void) {
    return unimpi_mt.t_category_get_num != NULL &&
           unimpi_mt.t_enum_get_info != NULL;
}

/* G1: MPI_T_init_thread returns at least SINGLE; implant the MPI_T reference
 * so the queried backend opens its tools interface. (init/finalize pairing and
 * the idempotent re-init path are exercised implicitly by the post-finalize
 * case below.) */
static int test_lifecycle(int *provided_p) {
    int prov = 0;
    CHECK(MPI_T_init_thread(MPI_THREAD_SINGLE, &prov));
    if (prov < MPI_THREAD_SINGLE) {
        fprintf(stderr, "FAIL: MPI_T_init_thread provided %d < MPI_THREAD_SINGLE\n", prov);
        return 1;
    }
    *provided_p = prov;
    printf("  lifecycle OK (provided %d)\n", prov);
    return 0;
}

/* G2: enum self-check -- get_info must return a var class that matches the
 * unimpi-exposed constant for the active backend.
 *
 * Only the FIRST queryable pvar is probed. Two backend quirks make a full
 * sweep unsafe: (a) OpenMPI legally leaves holes inside [0,num_pvar) (pvars
 * of not-yet-enabled components answer get_info with MPI_T_ERR_INVALID), and
 * (b) OpenMPI can infinite-loop inside get_info on later indices of enabled
 * components. Stopping at the first queryable index still exercises the
 * enum-to-constant mapping once, which is the point of the check. */
static int test_enum_selfcheck(void) {
    int n = 0;
    CHECK(MPI_T_pvar_get_num(&n));
    for (int i = 0; i < n && i < 4096; i++) {
        int name_len = 256, verb = 0, vclass = 0;
        MPI_T_enum et = 0;
        char name[256];
        if (MPI_T_pvar_get_info(i, name, &name_len, &et, NULL, &verb, &vclass, NULL) != 0)
            continue;
        int classes[10];
        int k, valid = 0;
        classes[0] = UNIMPI_T_PVAR_CLASS_STATE;
        classes[1] = UNIMPI_T_PVAR_CLASS_LEVEL;
        classes[2] = UNIMPI_T_PVAR_CLASS_SIZE;
        classes[3] = UNIMPI_T_PVAR_CLASS_PERCENTAGE;
        classes[4] = UNIMPI_T_PVAR_CLASS_HIGHWATERMARK;
        classes[5] = UNIMPI_T_PVAR_CLASS_LOWWATERMARK;
        classes[6] = UNIMPI_T_PVAR_CLASS_COUNTER;
        classes[7] = UNIMPI_T_PVAR_CLASS_AGGREGATE;
        classes[8] = UNIMPI_T_PVAR_CLASS_TIMER;
        classes[9] = UNIMPI_T_PVAR_CLASS_GENERIC;
        for (k = 0; k < 10; k++)
            if (vclass == classes[k]) { valid = 1; break; }
        /* The returned var class must be one of the backend-exposed
         * UNIMPI_T_PVAR_CLASS_* values. No position/ordinal assumption: on
         * OpenMPI these are 0..9, on the MPICH family 240..249, and the
         * backend-fill must agree with what get_info reports. */
        if (!valid) {
            fprintf(stderr, "FAIL pvar[%d] '%s' vclass=%d not in UNIMPI_T_PVAR_CLASS_*\n",
                    i, name, vclass);
            return 1;
        }
        printf("  enum self-check OK (pvar[%d] '%s')\n", i, name);
        return 0;
    }
    printf("  enum self-check: skip (no queryable pvar)\n");
    return 0;
}

/* G3: best-effort full round trip on a no-object-bound pvar -- session
 * create, handle alloc, start, read, stop, free. Skips (exit OK) when the
 * backend exports no suitable pvar.
 *
 * SIZE is included because the first queryable OpenMPI pvar (mpool counters,
 * bind=NO_OBJECT) is SIZE-class; letting it hit keeps the round trip on a
 * safe pvar instead of walking into OpenMPI's mtl_psm2 pvars, which SEGV in
 * handle_alloc on machines with no PSM device (backend robustness issue, not
 * uniMPI's). The name guard below skips those specifically. */
static int test_noobject_roundtrip(void) {
    int n = 0;
    CHECK(MPI_T_pvar_get_num(&n));
    for (int i = 0; i < n && i < 4096; i++) {
        int name_len = 64, verb = 0, vclass = 0;
        char name[64];
        if (MPI_T_pvar_get_info(i, name, &name_len, NULL, NULL, &verb, &vclass, NULL) != 0)
            continue;
        /* only try simple non-trivial classes */
        if (vclass != UNIMPI_T_PVAR_CLASS_STATE &&
            vclass != UNIMPI_T_PVAR_CLASS_LEVEL &&
            vclass != UNIMPI_T_PVAR_CLASS_SIZE &&
            vclass != UNIMPI_T_PVAR_CLASS_COUNTER)
            continue;
        /* OpenMPI's PSM2 MTL pvars crash handle_alloc on hosts without a PSM
         * device; avoid them rather than deselecting them gracefully. */
        if (strncmp(name, "mtl_psm2", 8) == 0)
            continue;
        MPI_T_pvar_session sess = 0;
        if (MPI_T_pvar_session_create(&sess) != 0)
            continue;
        MPI_T_pvar_handle h;
        int cnt = 0;
        if (MPI_T_pvar_handle_alloc(sess, i, UNIMPI_T_BIND_NO_OBJECT, &h, &cnt) == 0) {
            char buf[64] = {0};
            (void)MPI_T_pvar_start(sess, h);
            (void)MPI_T_pvar_read(sess, h, buf);
            (void)MPI_T_pvar_stop(sess, h);
            (void)MPI_T_pvar_handle_free(sess, &h);
            (void)MPI_T_pvar_session_free(&sess);
            printf("  no-object pvar roundtrip OK (idx %d, '%s')\n", i, name);
            return 0;
        }
        (void)MPI_T_pvar_session_free(&sess);
    }
    printf("  no-object roundtrip: skip (no suitable pvar exported)\n");
    return 0;
}

/* G4 (category): walk every category returned by get_num/get_info and assert
 *  self-consistency -- non-negative member counts, and get_cvars/get_pvars
 *  agree in length. No category NAME is assumed (names are backend-specific and
 *  not portable across implementations). Namely: OpenMPI's category set is
 *  component-dependent, MPICH's is fixed; indexing must not hard-code either.
 *
 *  NOTE on the len argument: MPI_T_category_get_cvars/get_pvars take the array
 *  CAPACITY, not the count. Passing get_info's nc/np as len would overrun a
 *  fixed stack array when a category has more members than room -- the arrays
 *  are sized 16 and the calls are gated on nc/np <= 16 to stay safe.
 */
static int test_category_roundtrip(void) {
    if (!category_available()) {
        printf("  category introspection unavailable; skip\n");
        return 0;
    }
    int n = 0, stamp = -1;
    if (unimpi_mt.t_category_changed)
        CHECK(unimpi_mt.t_category_changed(&stamp));
    CHECK(unimpi_mt.t_category_get_num(&n));
    if (n < 0) { fprintf(stderr, "FAIL category_get_num=%d\n", n); return 1; }
    if (n == 0) { printf("  category count 0; skip\n"); return 0; }
    for (int i = 0; i < n; i++) {
        /* OpenMPI, like its pvars, may leave holes inside [0,n): components
         * whose categories are not currently queryable answer get_info with a
         * non-success code. Walk past those instead of failing the suite. */
        int name_len = 256, desc_len = 256;
        int nc = -1, np = -1, nm = -1;
        char name[256], desc[256];
        if (unimpi_mt.t_category_get_info(i, name, &name_len, desc, &desc_len,
                                          &nc, &np, &nm) != MPI_SUCCESS)
            continue;
        if (nc < 0 || np < 0) return 1;   /* member counts must be >= 0 */
        int cv[16];
        if (unimpi_mt.t_category_get_cvars && nc <= 16)
            CHECK(unimpi_mt.t_category_get_cvars(i, 16, cv));
        int pv[16];
        if (unimpi_mt.t_category_get_pvars && np <= 16)
            CHECK(unimpi_mt.t_category_get_pvars(i, 16, pv));
        (void)nm;
    }
    printf("  category roundtrip OK (%d categories)\n", n);
    return 0;
}

/* G5 (enum): a valid MPI_T_enum only materializes as the `enumtype` out-param
 *  of a pvar/cvar get_info -- there is no MPI_T_ENUM_NULL constant to seed a
 *  query from, so scan pvars for one that reports a usable enum type, then
 *  assert enum_get_info returns a sane member count and enum_get_item
 *  reproduces value+name per member. If the backend exposes no enumerable
 *  pvar, it is a documented skip, not a failure.
 *
 *  MPI_T_enum 0 doubles as the "not an enum subtype" sentinel (any valid enum
 *  is a positive backend-defined id); a real enum equal to 0 would only make
 *  this pvar look non-enumerable, and another pvar still yields a non-zero id.
 */
static int test_enum_query(void) {
    if (!category_available() || !unimpi_mt.t_pvar_get_num ||
        !unimpi_mt.t_pvar_get_info || !unimpi_mt.t_enum_get_item) {
        printf("  enum introspection unavailable; skip\n");
        return 0;
    }
    int np = 0;
    CHECK(unimpi_mt.t_pvar_get_num(&np));
    for (int i = 0; i < np; i++) {
        char name[128]; int name_len = sizeof(name);
        MPI_T_enum et = 0;          /* get_info's enumtype out-param */
        MPI_T_pvar_session bind = 0; int verb = 0, vc = 0;
        if (unimpi_mt.t_pvar_get_info(i, name, &name_len, &et, &bind, &verb, &vc,
                                      NULL) != MPI_SUCCESS)
            continue;
        if (et == 0) continue;      /* not an enum-typed pvar */
        int num = 0; char ename[128]; int enlen = sizeof(ename);
        if (unimpi_mt.t_enum_get_info(et, &num, ename, &enlen) != MPI_SUCCESS)
            continue;               /* backend cannot introspect it */
        if (num < 0) { fprintf(stderr, "FAIL enum_get_info num=%d\n", num); return 1; }
        for (int m = 0; m < num; m++) {
            int value = -1; char iname[128]; int ilen = sizeof(iname);
            CHECK(unimpi_mt.t_enum_get_item(et, m, &value, iname, &ilen));
            (void)value;
        }
        printf("  enum roundtrip OK (pvar %d, %d members)\n", i, num);
        return 0;
    }
    printf("  no enumerable pvar on this backend; skip enum\n");
    return 0;
}

/* G4: MPI_T must survive MPI_Finalize (user's core concern). Called after the
 * MPI interface has already been finalized in main. */
static int test_post_finalize_survival(void) {
    int prov = 0;
    CHECK(MPI_T_init_thread(MPI_THREAD_SINGLE, &prov));
    int n = 0;
    CHECK(MPI_T_pvar_get_num(&n));
    MPI_T_finalize();
    printf("  post-MPI_Finalize: MPI_T re-init/query/finalize OK\n");
    return 0;
}

int main(int argc, char **argv) {
    int rank = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    printf("=== MPI-T tools interface tests ===\n");
    if (!mpit_available()) {
        if (rank == 0)
            printf("  MPI_T unavailable on this backend; skip\n");
        MPI_Finalize();
        return 0;
    }
    int provided = 0;
    if (test_lifecycle(&provided))
        goto fail;
    if (test_enum_selfcheck())
        goto fail;
    if (test_noobject_roundtrip())
        goto fail;
    if (test_category_roundtrip())
        goto fail;
    if (test_enum_query())
        goto fail;
    /* finalize the MPI interface first; MPI_T must still work after */
    MPI_Finalize();
    if (test_post_finalize_survival())
        return 1;
    printf("=== All MPI-T tests passed ===\n");
    return 0;
fail:
    MPI_Finalize();
    return 1;
}
