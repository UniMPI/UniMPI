/* tests/internal/test_version.c - validate the UniMPI library version surface.
 *
 * Checks:
 *   1. compile-time UNIMPI_VERSION_MAJOR/MINOR/PATCH macros have expected values;
 *   2. UNIMPI_VERSION_STRING and UNIMPI_VERSION_PRERELEASE match the numeric form;
 *   3. UNIMPI_VERSION_NUMERICAL encodes to (MAJOR<<24)|(MINOR<<16)|PATCH;
 *   4. unimpi_get_version() returns the canonical string (runtime parity with
 *      the compile-time macro - no silent drift between header and core.c).
 *
 * Does not require MPI_Init or a backend.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "unimpi.h"
#include "unimpi_version.h"

int main(void) {
    const char *rt;

    /* 1. numeric components */
    assert(UNIMPI_VERSION_MAJOR == 0);
    assert(UNIMPI_VERSION_MINOR == 1);
    assert(UNIMPI_VERSION_PATCH == 0);
    printf("compile-time: %d.%d.%d\n",
           UNIMPI_VERSION_MAJOR, UNIMPI_VERSION_MINOR, UNIMPI_VERSION_PATCH);

    /* 2. prerelease tag present (alpha phase) */
    assert(strcmp(UNIMPI_VERSION_PRERELEASE, "alpha") == 0);

    /* 3. canonical string matches <major>.<minor>.<patch>-<prerelease> */
    assert(strcmp(UNIMPI_VERSION_STRING, "0.1.0-alpha") == 0);
    printf("string: %s\n", UNIMPI_VERSION_STRING);

    /* 4. numeric encoding */
    assert(UNIMPI_VERSION_NUMERICAL ==
           ((0u << 24) | (1u << 16) | (0u << 0)));
    printf("numerical: 0x%08x\n", UNIMPI_VERSION_NUMERICAL);

    /* 5. runtime parity with the compile-time macro (single source of truth) */
    rt = unimpi_get_version();
    assert(rt != NULL);
    assert(strcmp(rt, UNIMPI_VERSION_STRING) == 0);
    printf("runtime: %s\n", rt);

    printf("ALL VERSION CHECKS PASSED\n");
    return 0;
}
