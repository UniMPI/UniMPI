# Tests CMakeLists.txt
enable_testing()

# The test suite uses assert() for both checks and some checked calls. Keep
# assertions enabled even when the library itself is built in Release mode.
if(MSVC)
    add_compile_options(/UNDEBUG)
else()
    add_compile_options(-UNDEBUG)
endif()

# Include directories for tests
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpi)

# Configure the executor only when real MPI tests were requested.
set(UNIMPI_REAL_MPI_TESTS_ENABLED OFF)
if(UNIMPI_BUILD_MPI_TESTS)
    find_package(MPI COMPONENTS C)

    if(NOT MPIEXEC_EXECUTABLE)
        if(MPI_C_EXECUTABLE)
            set(MPIEXEC_EXECUTABLE "${MPI_C_EXECUTABLE}")
        elseif(WIN32)
            find_program(MPIEXEC_EXECUTABLE NAMES mpiexec.exe msmpi.exe PATHS "C:/Program Files/Microsoft MPI/Bin" DOC "MPI executor")
        else()
            find_program(MPIEXEC_EXECUTABLE NAMES mpirun mpiexec DOC "MPI executor")
        endif()
    endif()

    if(MPIEXEC_EXECUTABLE)
        message(STATUS "MPI executor: ${MPIEXEC_EXECUTABLE}")
        set(UNIMPI_REAL_MPI_TESTS_ENABLED ON)
    else()
        message(WARNING "MPI executor not found - real MPI tests will be disabled")
    endif()

    # Parse preflags if provided
    if(MPIEXEC_PREFLAGS)
        separate_arguments(MPIEXEC_PREFLAGS_LIST UNIX_COMMAND "${MPIEXEC_PREFLAGS}")
    else()
        set(MPIEXEC_PREFLAGS_LIST "")
    endif()
endif()

# Helper function to add MPI tests
function(add_mpi_test name target np)
    if(MPIEXEC_EXECUTABLE)
        set(test_command ${MPIEXEC_EXECUTABLE})
        if(MPIEXEC_PREFLAGS_LIST)
            list(APPEND test_command ${MPIEXEC_PREFLAGS_LIST})
        endif()
        list(APPEND test_command "-np" "${np}")
        # Use generator expression to get the actual built executable path
        list(APPEND test_command "$<TARGET_FILE:${target}>")
        if(ARGN)
            list(APPEND test_command ${ARGN})
        endif()
        add_test(NAME ${name} COMMAND ${test_command})
        set_tests_properties(${name} PROPERTIES
            LABELS "integration;mpi"
            PROCESSORS "${np}"
            TIMEOUT 180
        )
    else()
        # Skip MPI tests if executor not available
        add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -E echo "SKIP: MPI executor not available")
        set_tests_properties(${name} PROPERTIES DISABLED TRUE)
    endif()
endfunction()

# ============================================
# Internal tests (unimpi internal functions, no MPI dependency)
# ============================================

# Error handling test
add_executable(test_error internal/test_error.c)
target_link_libraries(test_error unimpi)
add_test(NAME test_error COMMAND test_error)
set_tests_properties(test_error PROPERTIES
    LABELS "unit;internal;error"
    TIMEOUT 30
)

# Loader test
add_executable(test_loader internal/test_loader.c)
target_link_libraries(test_loader unimpi)
add_test(NAME test_loader COMMAND test_loader)
set_tests_properties(test_loader PROPERTIES
    LABELS "unit;internal;loader"
    TIMEOUT 30
)

# Compile-time MPI version gating macro test
add_executable(test_version_gate internal/test_version_gate.c)
target_link_libraries(test_version_gate unimpi)
add_test(NAME test_version_gate COMMAND test_version_gate)
set_tests_properties(test_version_gate PROPERTIES
    LABELS "unit;internal;version"
    TIMEOUT 30
)

# Fake MPI libraries for lifecycle and vtable validation.  Each missing-symbol
# variant is a separate DSO so symbol presence is tested through the real
# platform loader rather than by mocking dlsym/GetProcAddress.
function(add_fake_mpi_fixture target)
    add_library(${target} SHARED fixtures/fake_mpi.c)
    if(ARGN)
        target_compile_definitions(${target} PRIVATE ${ARGN})
    endif()
    set_target_properties(${target} PROPERTIES
        PREFIX ""
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endfunction()

add_fake_mpi_fixture(fake_mpi)
add_fake_mpi_fixture(fake_mpi_no_init_thread UNIMPI_FAKE_OMIT_INIT_THREAD)
add_fake_mpi_fixture(fake_mpi_missing_init UNIMPI_FAKE_OMIT_INIT)
add_fake_mpi_fixture(fake_mpi_missing_finalize UNIMPI_FAKE_OMIT_FINALIZE)
add_fake_mpi_fixture(fake_mpi_missing_comm_size UNIMPI_FAKE_OMIT_COMM_SIZE)
add_fake_mpi_fixture(fake_mpi_missing_comm_rank UNIMPI_FAKE_OMIT_COMM_RANK)

# Fake MPI libraries for backend identification testing
add_library(fake_openmpi_identity SHARED fixtures/fake_mpi_identity.c)
target_compile_definitions(fake_openmpi_identity PRIVATE UNIMPI_FAKE_OPENMPI)
set_target_properties(fake_openmpi_identity PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_library(fake_mpich_identity SHARED fixtures/fake_mpi_identity.c)
target_compile_definitions(fake_mpich_identity PRIVATE UNIMPI_FAKE_MPICH)
set_target_properties(fake_mpich_identity PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_library(fake_intelmpi_identity SHARED fixtures/fake_mpi_identity.c)
target_compile_definitions(fake_intelmpi_identity PRIVATE UNIMPI_FAKE_INTELMPI)
set_target_properties(fake_intelmpi_identity PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

add_library(fake_unknown_identity SHARED fixtures/fake_mpi_identity.c)
set_target_properties(fake_unknown_identity PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Lifecycle tests (internal API)
add_executable(test_lifecycle internal/test_lifecycle.c)
target_link_libraries(test_lifecycle unimpi)

function(add_fake_lifecycle_test name scenario fixture)
    add_test(NAME ${name}
        COMMAND test_lifecycle ${scenario} $<TARGET_FILE:${fixture}> ${ARGN})
    set_tests_properties(${name} PROPERTIES
        LABELS "unit;fake;internal;lifecycle"
        TIMEOUT 30
    )
endfunction()

add_fake_lifecycle_test(test_lifecycle_preinit preinit fake_mpi)
add_fake_lifecycle_test(test_lifecycle_success success fake_mpi)
add_fake_lifecycle_test(test_lifecycle_double_init double-init fake_mpi)
add_fake_lifecycle_test(test_lifecycle_init_failure_retry
    init-failure-retry fake_mpi)
add_fake_lifecycle_test(test_lifecycle_thread_single
    thread-level fake_mpi 0)
add_fake_lifecycle_test(test_lifecycle_thread_funneled
    thread-level fake_mpi 1)
add_fake_lifecycle_test(test_lifecycle_thread_serialized
    thread-level fake_mpi 2)
add_fake_lifecycle_test(test_lifecycle_thread_multiple
    thread-level fake_mpi 3)
add_fake_lifecycle_test(test_lifecycle_thread_downgrade
    thread-downgrade fake_mpi)
add_fake_lifecycle_test(test_lifecycle_thread_fallback
    thread-fallback fake_mpi_no_init_thread)
add_fake_lifecycle_test(test_lifecycle_finalize_failure
    finalize-failure fake_mpi)
add_fake_lifecycle_test(test_lifecycle_postfinal postfinal fake_mpi)
add_fake_lifecycle_test(test_lifecycle_identity_cleanup
    identity-cleanup fake_mpi)

# Vtable validation test
add_executable(test_vtable_validation internal/test_vtable_validation.c)
target_link_libraries(test_vtable_validation unimpi)
add_test(NAME test_vtable_validation
    COMMAND test_vtable_validation
            $<TARGET_FILE:fake_mpi>
            $<TARGET_FILE:fake_mpi_missing_init>
            $<TARGET_FILE:fake_mpi_missing_finalize>
            $<TARGET_FILE:fake_mpi_missing_comm_size>
            $<TARGET_FILE:fake_mpi_missing_comm_rank>)
set_tests_properties(test_vtable_validation PROPERTIES
    LABELS "unit;fake;internal;vtable"
    TIMEOUT 30
)

# Backend identification test
add_executable(test_backend_identification internal/test_backend_identification.c)
target_link_libraries(test_backend_identification unimpi)
add_test(NAME test_backend_identification
    COMMAND test_backend_identification
            $<TARGET_FILE:fake_openmpi_identity>
            $<TARGET_FILE:fake_mpich_identity>
            $<TARGET_FILE:fake_intelmpi_identity>
            $<TARGET_FILE:fake_unknown_identity>)
set_tests_properties(test_backend_identification PROPERTIES
    LABELS "unit;fake;internal;loader"
    TIMEOUT 30
)

# Static API/backend parity contract. This verifies that every direct MPI macro
# mapping is assigned by every backend, catching surface drift without needing
# each real runtime on the current host.
add_test(NAME test_backend_parity
    COMMAND ${CMAKE_COMMAND}
            -DUNIMPI_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_backend_parity.cmake)
set_tests_properties(test_backend_parity PROPERTIES
    LABELS "unit;internal;contract"
    TIMEOUT 30
)

# ============================================
# MPI layer tests (standard MPI API)
# ============================================

# Lifecycle tests (MPI standard API via macros)
add_executable(test_lifecycle_macros mpi/test_lifecycle_macros.c)
target_link_libraries(test_lifecycle_macros unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_test(NAME test_lifecycle_macros_init
        COMMAND test_lifecycle_macros init)
    add_test(NAME test_lifecycle_macros_thread
        COMMAND test_lifecycle_macros thread)
    set_tests_properties(
        test_lifecycle_macros_init
        test_lifecycle_macros_thread
        PROPERTIES
            LABELS "integration;mpi;lifecycle"
            PROCESSORS 1
            TIMEOUT 180
    )
endif()

# Point-to-point communication tests
add_executable(test_p2p mpi/test_p2p.c)
target_link_libraries(test_p2p unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_p2p test_p2p 2)
endif()

# Collective communication tests
add_executable(test_collective mpi/test_collective.c)
target_link_libraries(test_collective unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_collective test_collective 4)
endif()

# Datatype tests
add_executable(test_datatype mpi/test_datatype.c)
target_link_libraries(test_datatype unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_datatype test_datatype 2)
endif()

# Extended P2P tests
add_executable(test_p2p_extended mpi/test_p2p_extended.c)
target_link_libraries(test_p2p_extended unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_p2p_extended test_p2p_extended 2)
endif()

# Communicator tests
add_executable(test_communicator mpi/test_communicator.c)
target_link_libraries(test_communicator unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_communicator test_communicator 4)
endif()

# Group tests
add_executable(test_group mpi/test_group.c)
target_link_libraries(test_group unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_group test_group 4)
endif()

# Extended datatype tests
add_executable(test_datatype_extended mpi/test_datatype_extended.c)
target_link_libraries(test_datatype_extended unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_datatype_extended test_datatype_extended 2)
endif()

# Extended collective tests
add_executable(test_collective_extended mpi/test_collective_extended.c)
target_link_libraries(test_collective_extended unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_collective_extended test_collective_extended 4)
endif()

# MPI-3 nonblocking collective coverage
add_executable(test_collective_nonblocking
    mpi/test_collective_nonblocking.c)
target_link_libraries(test_collective_nonblocking unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(
        test_collective_nonblocking test_collective_nonblocking 4)
endif()

# Environment, Info, thread, memory, op and status coverage
add_executable(test_environment_info mpi/test_environment_info.c)
target_link_libraries(test_environment_info unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_environment_info test_environment_info 2)
endif()

# Request-array operations exercise the MPICH/Intel/MS-MPI 32-bit handle
# wrappers and the native OpenMPI path with the same semantics.
add_executable(test_request_arrays mpi/test_request_arrays.c)
target_link_libraries(test_request_arrays unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_request_arrays test_request_arrays 2)
endif()

# Topology tests
add_executable(test_topology test_topology.c)
target_link_libraries(test_topology unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_topology test_topology 4)
endif()

# Intercommunicator tests (MPI-2.2)
add_executable(test_intercomm mpi/test_intercomm.c)
target_link_libraries(test_intercomm unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_intercomm test_intercomm 4)
    add_mpi_test(test_intercomm_odd test_intercomm 5)
endif()

# Extended datatype tests (MPI-2.2)
add_executable(test_datatype_extended2 mpi/test_datatype_extended2.c)
target_link_libraries(test_datatype_extended2 unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_datatype_extended2 test_datatype_extended2 2)
endif()

# RMA extension tests (MPI-2.2)
add_executable(test_rma_extended mpi/test_rma_extended.c)
target_link_libraries(test_rma_extended unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_rma_extended test_rma_extended 2)
endif()

# I/O extension tests (MPI-2.2)
add_executable(test_io_extended mpi/test_io_extended.c)
target_link_libraries(test_io_extended unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_mpi_test(test_io_extended test_io_extended 2)
endif()

# Dynamic Processes tests (MPI 2.2)
add_executable(test_spawn_child mpi/test_spawn_child.c)
target_link_libraries(test_spawn_child unimpi)

add_executable(test_dynamic mpi/test_dynamic.c)
target_link_libraries(test_dynamic unimpi)
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    # Dynamic processes tests - run with 1 process (spawns its own children)
    # Pass child executable path via environment
    add_mpi_test(test_dynamic test_dynamic 1)
    set_tests_properties(test_dynamic PROPERTIES
        ENVIRONMENT "UNIMPI_SPAWN_CHILD_PATH=$<TARGET_FILE:test_spawn_child>"
    )
endif()

# Memory safety tests
add_executable(test_memory_safety internal/test_memory_safety.c)
target_link_libraries(test_memory_safety unimpi)
add_test(NAME test_memory_safety
    COMMAND test_memory_safety $<TARGET_FILE:fake_mpi>)
set_tests_properties(test_memory_safety PROPERTIES
    LABELS "unit;fake;internal;lifecycle"
    TIMEOUT 30
)

# Platform tests
add_executable(test_platform internal/test_platform.c)
target_link_libraries(test_platform unimpi)
add_test(NAME test_platform COMMAND test_platform)
set_tests_properties(test_platform PROPERTIES
    LABELS "unit;internal;platform"
    TIMEOUT 30
)

# ============================================
# Example and benchmark correctness smoke tests
# ============================================
# ============================================
# Example and benchmark correctness smoke tests
# ============================================

# MS-MPI capabilities test
add_executable(test_msmpi_capabilities mpi/test_msmpi_capabilities.c)
target_link_libraries(test_msmpi_capabilities unimpi)

# MS-MPI actual capability test (direct MS-MPI calls)
add_executable(test_msmpi_actual mpi/test_msmpi_actual.c)
target_link_libraries(test_msmpi_actual unimpi)

# MS-MPI spawn test (test if Comm_spawn works with valid path)
add_executable(test_spawn_test mpi/test_spawn_test.c)
target_link_libraries(test_spawn_test unimpi)

# Feature detection test
add_executable(test_feature_detection mpi/test_feature_detection.c)
target_link_libraries(test_feature_detection unimpi)

add_executable(test_benchmarks mpi/test_benchmarks.c)
target_link_libraries(test_benchmarks unimpi)

if(UNIMPI_REAL_MPI_TESTS_ENABLED AND UNIMPI_BUILD_EXAMPLES)
    add_mpi_test(example_minimal minimal 1)
    add_mpi_test(example_std_style std_style 1)
    add_mpi_test(example_backend_info backend_info 1)
    add_mpi_test(example_thread_init thread_init 1)
    add_mpi_test(example_nonblocking nonblocking 2)
    add_mpi_test(example_collective collective 4)
    add_mpi_test(example_rma rma 2)
    set_tests_properties(
        example_minimal
        example_std_style
        example_backend_info
        example_thread_init
        example_nonblocking
        example_collective
        example_rma
        PROPERTIES LABELS "integration;mpi;example"
    )
endif()

if(UNIMPI_REAL_MPI_TESTS_ENABLED AND UNIMPI_BUILD_BENCHMARKS)
    add_mpi_test(
        benchmark_latency_smoke bench_latency 2 --smoke --format csv)
    add_mpi_test(
        benchmark_collective_smoke bench_collective 4 --smoke --format csv)
    add_mpi_test(
        benchmark_overhead_smoke bench_overhead 1 --smoke --format csv)
    set_tests_properties(
        benchmark_latency_smoke
        benchmark_collective_smoke
        benchmark_overhead_smoke
        PROPERTIES
            LABELS "integration;mpi;benchmark"
            TIMEOUT 180
    )
endif()
