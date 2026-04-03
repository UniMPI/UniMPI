#include "tftk_mpi.h"

const char* tftk_mpi_error_string(int error_code) {
    switch (error_code) {
        case TFTK_MPI_OK:
            return "Success";
        case TFTK_MPI_ERR_NO_BACKEND:
            return "No MPI backend found";
        case TFTK_MPI_ERR_BACKEND_LOAD:
            return "Failed to load MPI backend library";
        case TFTK_MPI_ERR_ABI_MISMATCH:
            return "MPI ABI version mismatch";
        case TFTK_MPI_ERR_NOT_INITIALIZED:
            return "TFTK-MPI not initialized";
        case TFTK_MPI_ERR_ALREADY_INITIALIZED:
            return "TFTK-MPI already initialized";
        case TFTK_MPI_ERR_SYMBOL_NOT_FOUND:
            return "Required MPI symbol not found in backend";
        case TFTK_MPI_ERR_OUT_OF_MEMORY:
            return "Out of memory";
        default:
            return "Unknown error";
    }
}

int tftk_mpi_error_class(int error_code, int *error_class) {
    /* For now, map all TFTK errors to MPI_ERR_OTHER (15) */
    /* In full implementation, would call actual MPI_Error_class */
    *error_class = 15; /* MPI_ERR_OTHER */
    return TFTK_MPI_OK;
}
