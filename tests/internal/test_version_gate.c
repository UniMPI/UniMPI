#include "unimpi.h"
#include <stdio.h>

/* AT_LEAST 真值表：target=3.1 下 */
#if !UNIMPI_MPI_AT_LEAST(2,2)
#error "3.1 target must satisfy >= 2.2"
#endif
#if !UNIMPI_MPI_AT_LEAST(3,0)
#error "3.1 target must satisfy >= 3.0"
#endif
#if !UNIMPI_MPI_AT_LEAST(3,1)
#error "3.1 target must satisfy >= 3.1"
#endif
#if UNIMPI_MPI_AT_LEAST(4,0)
#error "3.1 target must NOT satisfy >= 4.0"
#endif

int main(void) {
    /* MPI-3.0 宏在默认 target 下必须存在 */
#ifdef MPI_Ibcast
    printf("IBCAST_PRESENT\n");
#else
    printf("IBCAST_ABSENT\n");
#endif
    return 0;
}
