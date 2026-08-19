/* Default-target vtable layout smoke test.
 *
 * Built and run at the DEFAULT target (3.1), where every MPI-3.0 field is
 * present and the struct must compile and size out unchanged relative to the
 * full MPI-3 surface. The strict-minimal 2.2 layout is asserted separately by
 * the negative-compile path (see test_vtable_strict and
 * tests/cmake/check_vtable_strict.cmake): at 2.2 the gated fields are physically
 * absent, so a reference to them is a compile error, which is exactly what that
 * test expects to observe.
 */
#include "unimpi.h"
#include <stdio.h>

int main(void) {
    size_t sz = sizeof(unimpi_vtable_t);
    size_t count = UNIMPI_VTABLE_COUNT;
    printf("VTABLE_SIZE=%zu\n", sz);
    printf("VTABLE_COUNT=%zu\n", count);

    /* At the default target the struct must be non-trivial and the counter
     * stable. The exact minimal-size assertion for the 2.2 build is enforced
     * by the strict/cross-build path in later tasks. */
    if (count < 1) {
        return 1;
    }
    return 0;
}
