#ifndef TFTK_MPI_STD_MACROS_H
#define TFTK_MPI_STD_MACROS_H

/* Standard MPI naming macros */
/* Include this header or define TFTK_MPI_USE_STD_NAMES to use */

/* Environment */
#define MPI_Init tftk_mpi.init
#define MPI_Finalize tftk_mpi.finalize
#define MPI_Initialized tftk_mpi.initialized
#define MPI_Finalized tftk_mpi.finalized
#define MPI_Abort tftk_mpi.abort
#define MPI_Get_processor_name tftk_mpi.get_processor_name
#define MPI_Get_version tftk_mpi.get_version
#define MPI_Wtime tftk_mpi.wtime
#define MPI_Wtick tftk_mpi.wtick
#define MPI_Barrier tftk_mpi.barrier

/* Point-to-point */
#define MPI_Send tftk_mpi.send
#define MPI_Recv tftk_mpi.recv
#define MPI_Isend tftk_mpi.isend
#define MPI_Irecv tftk_mpi.irecv
#define MPI_Wait tftk_mpi.wait
#define MPI_Waitall tftk_mpi.waitall
#define MPI_Sendrecv tftk_mpi.sendrecv

/* Collectives */
#define MPI_Bcast tftk_mpi.bcast
#define MPI_Reduce tftk_mpi.reduce
#define MPI_Allreduce tftk_mpi.allreduce
#define MPI_Gather tftk_mpi.gather
#define MPI_Allgather tftk_mpi.allgather
#define MPI_Scatter tftk_mpi.scatter

/* Communicator */
#define MPI_Comm_size tftk_mpi.comm_size
#define MPI_Comm_rank tftk_mpi.comm_rank
#define MPI_Comm_dup tftk_mpi.comm_dup
#define MPI_Comm_split tftk_mpi.comm_split
#define MPI_Comm_free tftk_mpi.comm_free

/* Predefined values */
#define MPI_COMM_WORLD TFTK_MPI_COMM_WORLD
#define MPI_COMM_SELF TFTK_MPI_COMM_SELF

/* Status */
#define MPI_Status TFTK_MPI_Status

#endif /* TFTK_MPI_STD_MACROS_H */
