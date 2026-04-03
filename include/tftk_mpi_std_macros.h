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
#define MPI_Sendrecv_replace tftk_mpi.sendrecv_replace

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
#define MPI_Comm_dup_with_info tftk_mpi.comm_dup_with_info
#define MPI_Comm_split tftk_mpi.comm_split
#define MPI_Comm_split_type tftk_mpi.comm_split_type
#define MPI_Comm_free tftk_mpi.comm_free

/* Predefined values */
#define MPI_COMM_WORLD TFTK_MPI_COMM_WORLD
#define MPI_COMM_SELF TFTK_MPI_COMM_SELF

/* Point-to-point - Sync/Buffered/Ready */
#define MPI_Ssend tftk_mpi.ssend
#define MPI_Ssend_init tftk_mpi.ssend_init
#define MPI_Bsend tftk_mpi.bsend
#define MPI_Bsend_init tftk_mpi.bsend_init
#define MPI_Buffer_attach tftk_mpi.buffer_attach
#define MPI_Buffer_detach tftk_mpi.buffer_detach
#define MPI_Rsend tftk_mpi.rsend
#define MPI_Rsend_init tftk_mpi.rsend_init

/* Non-blocking test and wait */
#define MPI_Test tftk_mpi.test
#define MPI_Testany tftk_mpi.testany
#define MPI_Testsome tftk_mpi.testsome
#define MPI_Testall tftk_mpi.testall
#define MPI_Waitany tftk_mpi.waitany
#define MPI_Waitsome tftk_mpi.waitsome

/* Message probing */
#define MPI_Probe tftk_mpi.probe
#define MPI_Iprobe tftk_mpi.iprobe

/* MPI-3 Matched probe operations */
#define MPI_Mprobe tftk_mpi.mprobe
#define MPI_Improbe tftk_mpi.improbe
#define MPI_Mrecv tftk_mpi.mrecv
#define MPI_Imrecv tftk_mpi.imrecv

/* Persistent communication */
#define MPI_Send_init tftk_mpi.send_init
#define MPI_Recv_init tftk_mpi.recv_init
#define MPI_Start tftk_mpi.start
#define MPI_Startall tftk_mpi.startall
#define MPI_Request_free tftk_mpi.request_free

/* Cancel and status */
#define MPI_Cancel tftk_mpi.cancel
#define MPI_Test_cancelled tftk_mpi.test_cancelled
#define MPI_Get_count tftk_mpi.get_count
#define MPI_Get_elements tftk_mpi.get_elements

/* Collectives - Variable length */
#define MPI_Gatherv tftk_mpi.gatherv
#define MPI_Allgatherv tftk_mpi.allgatherv
#define MPI_Scatterv tftk_mpi.scatterv
#define MPI_Alltoall tftk_mpi.alltoall
#define MPI_Alltoallv tftk_mpi.alltoallv

/* MPI-3 Alltoallw */
#define MPI_Alltoallw tftk_mpi.alltoallw

/* Collectives - Reduce-scatter and scan */
#define MPI_Reduce_scatter tftk_mpi.reduce_scatter
#define MPI_Reduce_scatter_block tftk_mpi.reduce_scatter_block
#define MPI_Scan tftk_mpi.scan
#define MPI_Exscan tftk_mpi.exscan

/* Datatypes - Creation */
#define MPI_Type_commit tftk_mpi.type_commit
#define MPI_Type_free tftk_mpi.type_free
#define MPI_Type_contiguous tftk_mpi.type_contiguous
#define MPI_Type_vector tftk_mpi.type_vector
#define MPI_Type_indexed tftk_mpi.type_indexed
#define MPI_Type_create_indexed_block tftk_mpi.type_create_indexed_block
#define MPI_Type_create_subarray tftk_mpi.type_create_subarray
#define MPI_Type_dup tftk_mpi.type_dup

/* MPI-3 Extended datatypes */
#define MPI_Type_hvector tftk_mpi.type_hvector
#define MPI_Type_hindexed tftk_mpi.type_hindexed
#define MPI_Type_create_darray tftk_mpi.type_create_darray

/* Datatypes - Query */
#define MPI_Type_get_extent tftk_mpi.type_get_extent
#define MPI_Type_get_size tftk_mpi.type_get_size
#define MPI_Type_get_name tftk_mpi.type_get_name
#define MPI_Type_set_name tftk_mpi.type_set_name

/* MPI-3 Extended datatype query */
#define MPI_Type_get_true_extent tftk_mpi.type_get_true_extent
#define MPI_Type_size tftk_mpi.type_size
#define MPI_Type_extent tftk_mpi.type_extent
#define MPI_Type_lb tftk_mpi.type_lb
#define MPI_Type_ub tftk_mpi.type_ub

/* Pack/Unpack */
#define MPI_Pack tftk_mpi.pack
#define MPI_Unpack tftk_mpi.unpack
#define MPI_Pack_size tftk_mpi.pack_size

/* MPI-3 External pack/unpack */
#define MPI_Pack_external tftk_mpi.pack_external
#define MPI_Unpack_external tftk_mpi.unpack_external
#define MPI_Pack_external_size tftk_mpi.pack_external_size

/* MPI-3 Non-blocking Collectives */
#define MPI_Ibarrier tftk_mpi.ibarrier
#define MPI_Ibcast tftk_mpi.ibcast
#define MPI_Igather tftk_mpi.igather
#define MPI_Igatherv tftk_mpi.igatherv
#define MPI_Iscatter tftk_mpi.iscatter
#define MPI_Iscatterv tftk_mpi.iscatterv
#define MPI_Iallgather tftk_mpi.iallgather
#define MPI_Iallgatherv tftk_mpi.iallgatherv
#define MPI_Ialltoall tftk_mpi.ialltoall
#define MPI_Ialltoallv tftk_mpi.ialltoallv
#define MPI_Ialltoallw tftk_mpi.ialltoallw
#define MPI_Ireduce tftk_mpi.ireduce
#define MPI_Iallreduce tftk_mpi.iallreduce
#define MPI_Ireduce_scatter tftk_mpi.ireduce_scatter
#define MPI_Ireduce_scatter_block tftk_mpi.ireduce_scatter_block
#define MPI_Iscan tftk_mpi.iscan
#define MPI_Iexscan tftk_mpi.iexscan

/* Group operations */
#define MPI_Group_size tftk_mpi.group_size
#define MPI_Group_rank tftk_mpi.group_rank
#define MPI_Group_translate_ranks tftk_mpi.group_translate_ranks
#define MPI_Group_compare tftk_mpi.group_compare
#define MPI_Group_union tftk_mpi.group_union
#define MPI_Group_intersection tftk_mpi.group_intersection
#define MPI_Group_difference tftk_mpi.group_difference
#define MPI_Group_incl tftk_mpi.group_incl
#define MPI_Group_excl tftk_mpi.group_excl
#define MPI_Group_range_incl tftk_mpi.group_range_incl
#define MPI_Group_range_excl tftk_mpi.group_range_excl
#define MPI_Group_free tftk_mpi.group_free

/* Communicator extended */
#define MPI_Comm_create tftk_mpi.comm_create
#define MPI_Comm_group tftk_mpi.comm_group
#define MPI_Comm_set_name tftk_mpi.comm_set_name
#define MPI_Comm_get_name tftk_mpi.comm_get_name

/* MPI-3 Extended communicator */
#define MPI_Comm_create_group tftk_mpi.comm_create_group
#define MPI_Comm_compare tftk_mpi.comm_compare
#define MPI_Comm_get_info tftk_mpi.comm_get_info
#define MPI_Comm_set_info tftk_mpi.comm_set_info

/* RMA - Window creation */
#define MPI_Win_create tftk_mpi.win_create
#define MPI_Win_allocate tftk_mpi.win_allocate
#define MPI_Win_free tftk_mpi.win_free

/* MPI-3 Extended RMA window */
#define MPI_Win_allocate_shared tftk_mpi.win_allocate_shared
#define MPI_Win_create_dynamic tftk_mpi.win_create_dynamic
#define MPI_Win_set_name tftk_mpi.win_set_name
#define MPI_Win_get_name tftk_mpi.win_get_name

/* RMA Operations */
#define MPI_Put tftk_mpi.put
#define MPI_Get tftk_mpi.get
#define MPI_Accumulate tftk_mpi.accumulate
#define MPI_Get_accumulate tftk_mpi.get_accumulate
#define MPI_Fetch_and_op tftk_mpi.fetch_and_op
#define MPI_Compare_and_swap tftk_mpi.compare_and_swap
#define MPI_Rput tftk_mpi.rput
#define MPI_Rget tftk_mpi.rget

/* MPI-3 Extended RMA operations */
#define MPI_Raccumulate tftk_mpi.raccumulate
#define MPI_Rget_accumulate tftk_mpi.rget_accumulate

/* RMA Synchronization */
#define MPI_Win_fence tftk_mpi.win_fence
#define MPI_Win_lock tftk_mpi.win_lock
#define MPI_Win_unlock tftk_mpi.win_unlock
#define MPI_Win_lock_all tftk_mpi.win_lock_all
#define MPI_Win_unlock_all tftk_mpi.win_unlock_all
#define MPI_Win_flush tftk_mpi.win_flush
#define MPI_Win_flush_all tftk_mpi.win_flush_all
#define MPI_Win_sync tftk_mpi.win_sync

/* MPI-3 Extended RMA synchronization */
#define MPI_Win_start tftk_mpi.win_start
#define MPI_Win_complete tftk_mpi.win_complete
#define MPI_Win_post tftk_mpi.win_post
#define MPI_Win_wait tftk_mpi.win_wait
#define MPI_Win_test tftk_mpi.win_test
#define MPI_Win_flush_local tftk_mpi.win_flush_local

/* Parallel I/O - File Operations */
#define MPI_File_open tftk_mpi.file_open
#define MPI_File_close tftk_mpi.file_close
#define MPI_File_delete tftk_mpi.file_delete
#define MPI_File_set_size tftk_mpi.file_set_size
#define MPI_File_preallocate tftk_mpi.file_preallocate
#define MPI_File_get_size tftk_mpi.file_get_size
#define MPI_File_get_group tftk_mpi.file_get_group
#define MPI_File_get_amode tftk_mpi.file_get_amode

/* MPI-3 Extended file operations */
#define MPI_File_get_info tftk_mpi.file_get_info
#define MPI_File_set_info tftk_mpi.file_set_info
#define MPI_File_seek tftk_mpi.file_seek
#define MPI_File_get_position tftk_mpi.file_get_position
#define MPI_File_get_byte_offset tftk_mpi.file_get_byte_offset

/* Parallel I/O - Read/Write */
#define MPI_File_read tftk_mpi.file_read
#define MPI_File_read_all tftk_mpi.file_read_all
#define MPI_File_write tftk_mpi.file_write
#define MPI_File_write_all tftk_mpi.file_write_all
#define MPI_File_read_at tftk_mpi.file_read_at
#define MPI_File_read_at_all tftk_mpi.file_read_at_all
#define MPI_File_write_at tftk_mpi.file_write_at
#define MPI_File_write_at_all tftk_mpi.file_write_at_all

/* MPI-3 Extended file read/write */
#define MPI_File_read_shared tftk_mpi.file_read_shared
#define MPI_File_write_shared tftk_mpi.file_write_shared
#define MPI_File_read_ordered tftk_mpi.file_read_ordered
#define MPI_File_write_ordered tftk_mpi.file_write_ordered

/* Parallel I/O - Non-blocking */
#define MPI_File_iread tftk_mpi.file_iread
#define MPI_File_iwrite tftk_mpi.file_iwrite
#define MPI_File_iread_at tftk_mpi.file_iread_at
#define MPI_File_iwrite_at tftk_mpi.file_iwrite_at

/* Parallel I/O - Views */
#define MPI_File_set_view tftk_mpi.file_set_view
#define MPI_File_get_view tftk_mpi.file_get_view

/* Dynamic Process Management */
#define MPI_Comm_spawn tftk_mpi.comm_spawn
#define MPI_Comm_spawn_multiple tftk_mpi.comm_spawn_multiple
#define MPI_Comm_accept tftk_mpi.comm_accept
#define MPI_Comm_connect tftk_mpi.comm_connect
#define MPI_Comm_disconnect tftk_mpi.comm_disconnect

/* MPI-3 Comm join */
#define MPI_Comm_join tftk_mpi.comm_join

/* Port and Name Service */
#define MPI_Open_port tftk_mpi.open_port
#define MPI_Close_port tftk_mpi.close_port
#define MPI_Publish_name tftk_mpi.publish_name
#define MPI_Unpublish_name tftk_mpi.unpublish_name
#define MPI_Lookup_name tftk_mpi.lookup_name

/* Info Operations */
#define MPI_Info_create tftk_mpi.info_create
#define MPI_Info_free tftk_mpi.info_free
#define MPI_Info_set tftk_mpi.info_set
#define MPI_Info_get tftk_mpi.info_get
#define MPI_Info_delete tftk_mpi.info_delete
#define MPI_Info_get_nkeys tftk_mpi.info_get_nkeys
#define MPI_Info_get_nthkey tftk_mpi.info_get_nthkey

/* Thread Support */
#define MPI_Init_thread tftk_mpi.init_thread
#define MPI_Query_thread tftk_mpi.query_thread
#define MPI_Is_thread_main tftk_mpi.is_thread_main

/* Memory Allocation */
#define MPI_Alloc_mem tftk_mpi.alloc_mem
#define MPI_Free_mem tftk_mpi.free_mem

/* MPI-3 Reduction operations */
#define MPI_Op_create tftk_mpi.op_create
#define MPI_Op_free tftk_mpi.op_free
#define MPI_Op_commutative tftk_mpi.op_commutative

/* MPI-3 Status manipulation */
#define MPI_Status_set_elements tftk_mpi.status_set_elements
#define MPI_Status_set_cancelled tftk_mpi.status_set_cancelled

/* MPI-3 Error handling */
#define MPI_Errhandler_create tftk_mpi.errhandler_create
#define MPI_Errhandler_free tftk_mpi.errhandler_free
#define MPI_Errhandler_set tftk_mpi.errhandler_set
#define MPI_Errhandler_get tftk_mpi.errhandler_get
#define MPI_Comm_create_errhandler tftk_mpi.comm_create_errhandler
#define MPI_Comm_call_errhandler tftk_mpi.comm_call_errhandler
#define MPI_Win_create_errhandler tftk_mpi.win_create_errhandler
#define MPI_File_create_errhandler tftk_mpi.file_create_errhandler
#define MPI_Add_error_class tftk_mpi.add_error_class
#define MPI_Add_error_code tftk_mpi.add_error_code
#define MPI_Add_error_string tftk_mpi.add_error_string

/* MPI-3 Attributes */
#define MPI_Attr_put tftk_mpi.attr_put
#define MPI_Attr_get tftk_mpi.attr_get
#define MPI_Attr_delete tftk_mpi.attr_delete

#endif /* TFTK_MPI_STD_MACROS_H */
