#ifndef UNIMPI_STD_MACROS_H
#define UNIMPI_STD_MACROS_H

/* Standard MPI naming macros */
/* Include this header or define UNIMPI_USE_STD_NAMES to use */

/* MPI Constants */
#define MPI_INFO_NULL UNIMPI_INFO_NULL

/* MPI Error codes - now runtime variables in unimpi_errors.h */

/* MPI Datatypes */
#define MPI_CHAR UNIMPI_CHAR
#define MPI_SIGNED_CHAR UNIMPI_SIGNED_CHAR
#define MPI_UNSIGNED_CHAR UNIMPI_UNSIGNED_CHAR
#define MPI_BYTE UNIMPI_BYTE
#define MPI_SHORT UNIMPI_SHORT
#define MPI_UNSIGNED_SHORT UNIMPI_UNSIGNED_SHORT
#define MPI_INT UNIMPI_INT
#define MPI_UNSIGNED UNIMPI_UNSIGNED
#define MPI_LONG UNIMPI_LONG
#define MPI_UNSIGNED_LONG UNIMPI_UNSIGNED_LONG
#define MPI_FLOAT UNIMPI_FLOAT
#define MPI_DOUBLE UNIMPI_DOUBLE
#define MPI_LONG_DOUBLE UNIMPI_LONG_DOUBLE
#define MPI_LONG_LONG_INT UNIMPI_LONG_LONG_INT
#define MPI_LONG_LONG UNIMPI_LONG_LONG
#define MPI_UNSIGNED_LONG_LONG UNIMPI_UNSIGNED_LONG_LONG

/* MPI Operations */
#define MPI_MAX UNIMPI_MAX
#define MPI_MIN UNIMPI_MIN
#define MPI_SUM UNIMPI_SUM
#define MPI_PROD UNIMPI_PROD
#define MPI_LAND UNIMPI_LAND
#define MPI_BAND UNIMPI_BAND
#define MPI_LOR UNIMPI_LOR
#define MPI_BOR UNIMPI_BOR
#define MPI_LXOR UNIMPI_LXOR
#define MPI_BXOR UNIMPI_BXOR
#define MPI_MINLOC UNIMPI_MINLOC
#define MPI_MAXLOC UNIMPI_MAXLOC

/* Predefined communicators */
#define MPI_COMM_WORLD UNIMPI_COMM_WORLD
#define MPI_COMM_SELF UNIMPI_COMM_SELF

/* Predefined request constant */
#define MPI_REQUEST_NULL UNIMPI_REQUEST_NULL

/* Status ignore sentinels (singular and plural; same facade pointer value) */
#define MPI_STATUS_IGNORE UNIMPI_STATUS_IGNORE
#define MPI_STATUSES_IGNORE UNIMPI_STATUSES_IGNORE

/* Topology types */
#define MPI_CART UNIMPI_CART
#define MPI_GRAPH UNIMPI_GRAPH
#define MPI_DIST_GRAPH UNIMPI_DIST_GRAPH

/* Environment */
#define MPI_Init unimpi_init
#define MPI_Init_thread unimpi_init_thread
#define MPI_Finalize unimpi_finalize
#define MPI_Initialized unimpi_mpi_initialized
#define MPI_Finalized unimpi_mpi_finalized
#define MPI_Abort unimpi.abort
#define MPI_Get_processor_name unimpi.get_processor_name
#define MPI_Get_version unimpi.get_version
#define MPI_Get_library_version unimpi.get_library_version
#define MPI_Wtime unimpi.wtime
#define MPI_Wtick unimpi.wtick
#define MPI_Barrier unimpi.barrier

/* Point-to-point */
#define MPI_Send unimpi.send
#define MPI_Recv unimpi.recv
#define MPI_Isend unimpi.isend
#define MPI_Irecv unimpi.irecv
#define MPI_Ibsend unimpi.ibsend
#define MPI_Irsend unimpi.irsend
#define MPI_Issend unimpi.issend
#define MPI_Wait unimpi.wait
#define MPI_Waitall unimpi.waitall
#define MPI_Sendrecv unimpi.sendrecv
#define MPI_Sendrecv_replace unimpi.sendrecv_replace

/* Collectives */
#define MPI_Bcast unimpi.bcast
#define MPI_Reduce unimpi.reduce
#define MPI_Reduce_local unimpi.reduce_local
#define MPI_Allreduce unimpi.allreduce
#define MPI_Gather unimpi.gather
#define MPI_Allgather unimpi.allgather
#define MPI_Scatter unimpi.scatter

/* Communicator */
#define MPI_Comm_size unimpi.comm_size
#define MPI_Comm_rank unimpi.comm_rank
#define MPI_Comm_dup unimpi.comm_dup
#define MPI_Comm_split unimpi.comm_split
#define MPI_Comm_free unimpi.comm_free

/* Predefined values */
#define MPI_COMM_WORLD UNIMPI_COMM_WORLD
#define MPI_COMM_SELF UNIMPI_COMM_SELF

/* Point-to-point - Sync/Buffered/Ready */
#define MPI_Ssend unimpi.ssend
#define MPI_Ssend_init unimpi.ssend_init
#define MPI_Bsend unimpi.bsend
#define MPI_Bsend_init unimpi.bsend_init
#define MPI_Buffer_attach unimpi.buffer_attach
#define MPI_Buffer_detach unimpi.buffer_detach
#define MPI_Rsend unimpi.rsend
#define MPI_Rsend_init unimpi.rsend_init

/* Non-blocking test and wait */
#define MPI_Test unimpi.test
#define MPI_Testany unimpi.testany
#define MPI_Testsome unimpi.testsome
#define MPI_Testall unimpi.testall
#define MPI_Waitany unimpi.waitany
#define MPI_Waitsome unimpi.waitsome

/* Message probing */
#define MPI_Probe unimpi.probe
#define MPI_Iprobe unimpi.iprobe

/* MPI-3 Matched probe operations */
#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI-3.0 matched_probe */
#define MPI_Mprobe unimpi.mprobe
#define MPI_Improbe unimpi.improbe
#define MPI_Mrecv unimpi.mrecv
#define MPI_Imrecv unimpi.imrecv
#endif

/* Persistent communication */
#define MPI_Send_init unimpi.send_init
#define MPI_Recv_init unimpi.recv_init
#define MPI_Start unimpi.start
#define MPI_Startall unimpi.startall
#define MPI_Request_free unimpi.request_free
#define MPI_Request_get_status unimpi.request_get_status
#define MPI_Grequest_start(query_fn, free_fn, cancel_fn, extra_state, request) \
    unimpi.grequest_start(query_fn, free_fn, cancel_fn, extra_state, request)
#define MPI_Grequest_complete(request) unimpi.grequest_complete(request)

/* Cancel and status */
#define MPI_Cancel unimpi.cancel
#define MPI_Test_cancelled unimpi.test_cancelled
#define MPI_Get_count unimpi.get_count
#define MPI_Get_elements unimpi.get_elements

/* Collectives - Variable length */
#define MPI_Gatherv unimpi.gatherv
#define MPI_Allgatherv unimpi.allgatherv
#define MPI_Scatterv unimpi.scatterv
#define MPI_Alltoall unimpi.alltoall
#define MPI_Alltoallv unimpi.alltoallv

/* MPI-3 Alltoallw */
#define MPI_Alltoallw unimpi.alltoallw

/* Collectives - Reduce-scatter and scan */
#define MPI_Reduce_scatter unimpi.reduce_scatter
#define MPI_Reduce_scatter_block unimpi.reduce_scatter_block
#define MPI_Scan unimpi.scan
#define MPI_Exscan unimpi.exscan

/* Datatypes - Creation */
#define MPI_Type_commit unimpi.type_commit
#define MPI_Type_free unimpi.type_free
#define MPI_Type_contiguous unimpi.type_contiguous
#define MPI_Type_vector unimpi.type_vector
#define MPI_Type_indexed unimpi.type_indexed
#define MPI_Type_create_indexed_block unimpi.type_create_indexed_block
#define MPI_Type_create_subarray unimpi.type_create_subarray
#define MPI_Type_dup unimpi.type_dup
#define MPI_Type_create_resized(oldtype, lb, extent, newtype) \
    unimpi.type_create_resized(oldtype, lb, extent, newtype)
#define MPI_Type_create_hvector unimpi.type_create_hvector
#define MPI_Type_create_hindexed unimpi.type_create_hindexed
#define MPI_Type_create_struct unimpi.type_create_struct
#define MPI_Type_struct unimpi.type_struct
#define MPI_Type_match_size unimpi.type_match_size
#define MPI_Type_create_f90_integer unimpi.type_create_f90_integer
#define MPI_Type_create_f90_real unimpi.type_create_f90_real
#define MPI_Type_create_f90_complex unimpi.type_create_f90_complex

/* MPI-3 Extended datatypes */
#define MPI_Type_hvector unimpi.type_hvector
#define MPI_Type_hindexed unimpi.type_hindexed
#define MPI_Type_create_darray unimpi.type_create_darray

/* Datatypes - Query */
#define MPI_Type_get_extent unimpi.type_get_extent
#define MPI_Type_get_size unimpi.type_get_size
#define MPI_Type_get_name unimpi.type_get_name
#define MPI_Type_set_name unimpi.type_set_name

/* MPI-3 Extended datatype query */
#define MPI_Type_get_true_extent unimpi.type_get_true_extent
#define MPI_Type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner) \
    unimpi.type_get_envelope(datatype, num_integers, num_addresses, num_datatypes, combiner)
#define MPI_Type_get_contents(datatype, max_integers, max_addresses, max_datatypes, \
                               array_of_integers, array_of_addresses, array_of_datatypes) \
    unimpi.type_get_contents(datatype, max_integers, max_addresses, max_datatypes, \
                              array_of_integers, array_of_addresses, array_of_datatypes)
#define MPI_Type_size unimpi.type_size
#define MPI_Type_extent unimpi.type_extent
#define MPI_Type_lb unimpi.type_lb
#define MPI_Type_ub unimpi.type_ub

/* Address arithmetic */
#define MPI_Get_address unimpi.get_address
#define MPI_Address unimpi.address

/* Pack/Unpack */
#define MPI_Pack unimpi.pack
#define MPI_Unpack unimpi.unpack
#define MPI_Pack_size unimpi.pack_size

/* MPI-3 External pack/unpack */
#define MPI_Pack_external unimpi.pack_external
#define MPI_Unpack_external unimpi.unpack_external
#define MPI_Pack_external_size unimpi.pack_external_size

/* MPI-3 Non-blocking Collectives */
#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI-3.0 nonblocking_collectives */
#define MPI_Ibarrier unimpi.ibarrier
#define MPI_Ibcast unimpi.ibcast
#define MPI_Igather unimpi.igather
#define MPI_Igatherv unimpi.igatherv
#define MPI_Iscatter unimpi.iscatter
#define MPI_Iscatterv unimpi.iscatterv
#define MPI_Iallgather unimpi.iallgather
#define MPI_Iallgatherv unimpi.iallgatherv
#define MPI_Ialltoall unimpi.ialltoall
#define MPI_Ialltoallv unimpi.ialltoallv
#define MPI_Ialltoallw unimpi.ialltoallw
#define MPI_Ireduce unimpi.ireduce
#define MPI_Iallreduce unimpi.iallreduce
#define MPI_Ireduce_scatter unimpi.ireduce_scatter
#define MPI_Ireduce_scatter_block unimpi.ireduce_scatter_block
#define MPI_Iscan unimpi.iscan
#define MPI_Iexscan unimpi.iexscan
#endif

/* Group operations */
#define MPI_Group_size unimpi.group_size
#define MPI_Group_rank unimpi.group_rank
#define MPI_Group_translate_ranks unimpi.group_translate_ranks
#define MPI_Group_compare unimpi.group_compare
#define MPI_Group_union unimpi.group_union
#define MPI_Group_intersection unimpi.group_intersection
#define MPI_Group_difference unimpi.group_difference
#define MPI_Group_incl unimpi.group_incl
#define MPI_Group_excl unimpi.group_excl
#define MPI_Group_range_incl unimpi.group_range_incl
#define MPI_Group_range_excl unimpi.group_range_excl
#define MPI_Group_free unimpi.group_free

/* Communicator extended */
#define MPI_Comm_create unimpi.comm_create
#define MPI_Comm_group unimpi.comm_group
#define MPI_Comm_set_name unimpi.comm_set_name
#define MPI_Comm_get_name unimpi.comm_get_name

/* MPI-3 Extended communicator */
#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI-3.0 comm_3x */
#define MPI_Comm_dup_with_info unimpi.comm_dup_with_info
#define MPI_Comm_split_type unimpi.comm_split_type
#define MPI_Comm_create_group unimpi.comm_create_group
#define MPI_Comm_get_info unimpi.comm_get_info
#define MPI_Comm_set_info unimpi.comm_set_info
#endif
#define MPI_Comm_compare unimpi.comm_compare

/* Intercommunicator Operations (MPI-2.2) */
#define MPI_Intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm) \
    unimpi.intercomm_create(local_comm, local_leader, peer_comm, remote_leader, tag, newintercomm)
#define MPI_Intercomm_merge(intercomm, high, newintracomm) \
    unimpi.intercomm_merge(intercomm, high, newintracomm)
#define MPI_Comm_remote_size(comm, size) \
    unimpi.comm_remote_size(comm, size)
#define MPI_Comm_remote_group(comm, group) \
    unimpi.comm_remote_group(comm, group)
#define MPI_Comm_test_inter(comm, flag) \
    unimpi.comm_test_inter(comm, flag)

/* Process Topologies - Cartesian */
#define MPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart) \
    unimpi.cart_create(comm_old, ndims, dims, periods, reorder, comm_cart)
#define MPI_Cartdim_get(comm, ndims) \
    unimpi.cartdim_get(comm, ndims)
#define MPI_Cart_get(comm, maxdims, dims, periods, coords) \
    unimpi.cart_get(comm, maxdims, dims, periods, coords)
#define MPI_Cart_rank(comm, coords, rank) \
    unimpi.cart_rank(comm, coords, rank)
#define MPI_Cart_coords(comm, rank, maxdims, coords) \
    unimpi.cart_coords(comm, rank, maxdims, coords)
#define MPI_Cart_shift(comm, direction, disp, rank_source, rank_dest) \
    unimpi.cart_shift(comm, direction, disp, rank_source, rank_dest)
#define MPI_Cart_sub(comm, remain_dims, newcomm) \
    unimpi.cart_sub(comm, remain_dims, newcomm)
#define MPI_Cart_map(comm, ndims, dims, periods, newrank) \
    unimpi.cart_map(comm, ndims, dims, periods, newrank)
#define MPI_Dims_create(nnodes, ndims, dims) \
    unimpi.dims_create(nnodes, ndims, dims)

/* Process Topologies - Graph */
#define MPI_Graph_create(comm_old, nnodes, index, edges, reorder, comm_graph) \
    unimpi.graph_create(comm_old, nnodes, index, edges, reorder, comm_graph)
#define MPI_Graphdims_get(comm, nnodes, nedges) \
    unimpi.graphdims_get(comm, nnodes, nedges)
#define MPI_Graph_get(comm, maxindex, maxedges, index, edges) \
    unimpi.graph_get(comm, maxindex, maxedges, index, edges)
#define MPI_Graph_neighbors_count(comm, rank, nneighbors) \
    unimpi.graph_neighbors_count(comm, rank, nneighbors)
#define MPI_Graph_neighbors(comm, rank, maxneighbors, neighbors) \
    unimpi.graph_neighbors(comm, rank, maxneighbors, neighbors)
#define MPI_Graph_map(comm, nnodes, index, edges, newrank) \
    unimpi.graph_map(comm, nnodes, index, edges, newrank)

/* Process Topologies - Distributed Graph */
#define MPI_Dist_graph_create(comm_old, n, sources, degrees, destinations, weights, info, reorder, comm_dist_graph) \
    unimpi.dist_graph_create(comm_old, n, sources, degrees, destinations, weights, info, reorder, comm_dist_graph)
#define MPI_Dist_graph_create_adjacent(comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph) \
    unimpi.dist_graph_create_adjacent(comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph)
#define MPI_Dist_graph_neighbors_count(comm, indegree, outdegree, weighted) \
    unimpi.dist_graph_neighbors_count(comm, indegree, outdegree, weighted)
#define MPI_Dist_graph_neighbors(comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights) \
    unimpi.dist_graph_neighbors(comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights)

/* Topology Testing */
#define MPI_Topo_test(comm, status) \
    unimpi.topo_test(comm, status)

/* RMA - Window creation */
#define MPI_Win_create unimpi.win_create
#define MPI_Win_allocate unimpi.win_allocate
#define MPI_Win_free unimpi.win_free

#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI-3.0 win_alloc_shared */
#define MPI_Win_allocate_shared unimpi.win_allocate_shared
#define MPI_Win_create_dynamic unimpi.win_create_dynamic
#endif
#define MPI_Win_set_name unimpi.win_set_name
#define MPI_Win_get_name unimpi.win_get_name

/* RMA Operations */
#define MPI_Put unimpi.put
#define MPI_Get unimpi.get
#define MPI_Accumulate unimpi.accumulate

#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI-3.0 rma_atomics */
#define MPI_Get_accumulate unimpi.get_accumulate
#define MPI_Fetch_and_op unimpi.fetch_and_op
#define MPI_Compare_and_swap unimpi.compare_and_swap
#define MPI_Rput unimpi.rput
#define MPI_Rget unimpi.rget
#define MPI_Raccumulate unimpi.raccumulate
#define MPI_Rget_accumulate unimpi.rget_accumulate
#endif

/* RMA Synchronization */
#define MPI_Win_fence unimpi.win_fence
#define MPI_Win_lock unimpi.win_lock
#define MPI_Win_unlock unimpi.win_unlock

#if UNIMPI_MPI_AT_LEAST(3,0)
/* MPI-3.0 rma_sync_3x */
#define MPI_Win_lock_all unimpi.win_lock_all
#define MPI_Win_unlock_all unimpi.win_unlock_all
#define MPI_Win_flush unimpi.win_flush
#define MPI_Win_flush_all unimpi.win_flush_all
#define MPI_Win_sync unimpi.win_sync
#define MPI_Win_flush_local unimpi.win_flush_local
#endif

/* RMA one-sided synchronization (MPI-2 base) */
#define MPI_Win_start unimpi.win_start
#define MPI_Win_complete unimpi.win_complete
#define MPI_Win_post unimpi.win_post
#define MPI_Win_wait unimpi.win_wait
#define MPI_Win_test unimpi.win_test

/* Parallel I/O - File Operations */
#define MPI_File_open unimpi.file_open
#define MPI_File_close unimpi.file_close
#define MPI_File_delete unimpi.file_delete
#define MPI_File_set_size unimpi.file_set_size
#define MPI_File_preallocate unimpi.file_preallocate
#define MPI_File_get_size unimpi.file_get_size
#define MPI_File_get_group unimpi.file_get_group
#define MPI_File_get_amode unimpi.file_get_amode

/* MPI-3 Extended file operations */
#define MPI_File_get_info unimpi.file_get_info
#define MPI_File_set_info unimpi.file_set_info
#define MPI_File_seek unimpi.file_seek
#define MPI_File_get_position unimpi.file_get_position
#define MPI_File_get_byte_offset unimpi.file_get_byte_offset
#define MPI_File_seek_shared unimpi.file_seek_shared
#define MPI_File_get_position_shared unimpi.file_get_position_shared
#define MPI_File_get_type_extent unimpi.file_get_type_extent

/* Parallel I/O - Read/Write */
#define MPI_File_read unimpi.file_read
#define MPI_File_read_all unimpi.file_read_all
#define MPI_File_write unimpi.file_write
#define MPI_File_write_all unimpi.file_write_all
#define MPI_File_read_at unimpi.file_read_at
#define MPI_File_read_at_all unimpi.file_read_at_all
#define MPI_File_write_at unimpi.file_write_at
#define MPI_File_write_at_all unimpi.file_write_at_all

/* MPI-3 Extended file read/write */
#define MPI_File_read_shared unimpi.file_read_shared
#define MPI_File_write_shared unimpi.file_write_shared
#define MPI_File_read_ordered unimpi.file_read_ordered
#define MPI_File_write_ordered unimpi.file_write_ordered

/* MPI-2 Split collective I/O */
#define MPI_File_read_all_begin unimpi.file_read_all_begin
#define MPI_File_read_all_end unimpi.file_read_all_end
#define MPI_File_read_at_all_begin unimpi.file_read_at_all_begin
#define MPI_File_read_at_all_end unimpi.file_read_at_all_end
#define MPI_File_read_ordered_begin unimpi.file_read_ordered_begin
#define MPI_File_read_ordered_end unimpi.file_read_ordered_end
#define MPI_File_write_all_begin unimpi.file_write_all_begin
#define MPI_File_write_all_end unimpi.file_write_all_end
#define MPI_File_write_at_all_begin unimpi.file_write_at_all_begin
#define MPI_File_write_at_all_end unimpi.file_write_at_all_end
#define MPI_File_write_ordered_begin unimpi.file_write_ordered_begin
#define MPI_File_write_ordered_end unimpi.file_write_ordered_end

/* Parallel I/O - Non-blocking */
#define MPI_File_iread unimpi.file_iread
#define MPI_File_iwrite unimpi.file_iwrite
#define MPI_File_iread_at unimpi.file_iread_at
#define MPI_File_iwrite_at unimpi.file_iwrite_at
#define MPI_File_iread_shared unimpi.file_iread_shared
#define MPI_File_iwrite_shared unimpi.file_iwrite_shared

/* Parallel I/O - Views */
#define MPI_File_set_view unimpi.file_set_view
#define MPI_File_get_view unimpi.file_get_view

/* Dynamic Process Management */
#define MPI_Comm_spawn unimpi.comm_spawn
#define MPI_Comm_spawn_multiple unimpi.comm_spawn_multiple
#define MPI_Comm_accept unimpi.comm_accept
#define MPI_Comm_connect unimpi.comm_connect
#define MPI_Comm_disconnect unimpi.comm_disconnect

#define MPI_Comm_join unimpi.comm_join

/* MPI_Comm_get_parent for spawned processes */
#define MPI_Comm_get_parent unimpi.comm_get_parent

/* Port and Name Service */
#define MPI_Open_port unimpi.open_port
#define MPI_Close_port unimpi.close_port
#define MPI_Publish_name unimpi.publish_name
#define MPI_Unpublish_name unimpi.unpublish_name
#define MPI_Lookup_name unimpi.lookup_name

/* Info Operations */
#define MPI_Info_create unimpi.info_create
#define MPI_Info_free unimpi.info_free
#define MPI_Info_set unimpi.info_set
#define MPI_Info_get unimpi.info_get
#define MPI_Info_delete unimpi.info_delete
#define MPI_Info_get_nkeys unimpi.info_get_nkeys
#define MPI_Info_get_nthkey unimpi.info_get_nthkey
#define MPI_Info_dup unimpi.info_dup
#define MPI_Info_get_valuelen unimpi.info_get_valuelen

/* Thread Support */
#define MPI_THREAD_SINGLE UNIMPI_THREAD_SINGLE
#define MPI_THREAD_FUNNELED UNIMPI_THREAD_FUNNELED
#define MPI_THREAD_SERIALIZED UNIMPI_THREAD_SERIALIZED
#define MPI_THREAD_MULTIPLE UNIMPI_THREAD_MULTIPLE
#define MPI_Query_thread unimpi.query_thread
#define MPI_Is_thread_main unimpi.is_thread_main

/* Memory Allocation */
#define MPI_Alloc_mem unimpi.alloc_mem
#define MPI_Free_mem unimpi.free_mem

/* Reduction operations */
#define MPI_Op_create unimpi.op_create
#define MPI_Op_free unimpi.op_free

#define MPI_Op_commutative unimpi.op_commutative

/* MPI-3 Status manipulation */
#define MPI_Status_set_elements unimpi.status_set_elements
#define MPI_Status_set_cancelled unimpi.status_set_cancelled
#define MPI_Status_f2c unimpi.status_f2c
#define MPI_Status_c2f unimpi.status_c2f

/* MPI-3 Error handling */
#define MPI_Errhandler_create unimpi.errhandler_create
#define MPI_Errhandler_free unimpi.errhandler_free
#define MPI_Errhandler_set unimpi.errhandler_set
#define MPI_Errhandler_get unimpi.errhandler_get
#define MPI_Comm_get_errhandler unimpi.comm_get_errhandler
#define MPI_Comm_set_errhandler unimpi.comm_set_errhandler
#define MPI_Win_get_errhandler unimpi.win_get_errhandler
#define MPI_Win_set_errhandler unimpi.win_set_errhandler
#define MPI_File_get_errhandler unimpi.file_get_errhandler
#define MPI_File_set_errhandler unimpi.file_set_errhandler
#define MPI_Comm_create_errhandler unimpi.comm_create_errhandler
#define MPI_Comm_call_errhandler unimpi.comm_call_errhandler
#define MPI_Win_create_errhandler unimpi.win_create_errhandler
#define MPI_Win_create_keyval unimpi.win_create_keyval
#define MPI_Win_free_keyval unimpi.win_free_keyval
#define MPI_Win_set_attr unimpi.win_set_attr
#define MPI_Win_get_attr unimpi.win_get_attr
#define MPI_Win_delete_attr unimpi.win_delete_attr
#define MPI_Win_get_group unimpi.win_get_group
#define MPI_Win_call_errhandler unimpi.win_call_errhandler
#define MPI_File_create_errhandler unimpi.file_create_errhandler
#define MPI_File_set_atomicity unimpi.file_set_atomicity
#define MPI_File_get_atomicity unimpi.file_get_atomicity
#define MPI_File_sync unimpi.file_sync
#define MPI_File_call_errhandler unimpi.file_call_errhandler
#define MPI_File_set_errhandler unimpi.file_set_errhandler
#define MPI_File_get_errhandler unimpi.file_get_errhandler
#define MPI_Add_error_class unimpi.add_error_class
#define MPI_Add_error_code unimpi.add_error_code
#define MPI_Add_error_string unimpi.add_error_string
#define MPI_Error_class unimpi.error_class
#define MPI_Error_string unimpi.error_string
#define MPI_Pcontrol unimpi.pcontrol

/* MPI-3 Attributes */
#define MPI_Attr_put unimpi.attr_put
#define MPI_Attr_get unimpi.attr_get
#define MPI_Attr_delete unimpi.attr_delete

#endif /* UNIMPI_STD_MACROS_H */
