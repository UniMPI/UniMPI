#include "unimpi_mt.h"
#if UNIMPI_MPI_AT_LEAST(3,0)
unimpi_mt_vtable_t unimpi_mt = { 0 };   /* born all-NULL, filled by backend init */
#endif
