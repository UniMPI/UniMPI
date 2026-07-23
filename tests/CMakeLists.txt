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
        add_test(NAME ${name} COMMAND ${test_command})
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

# Loader test
add_executable(test_loader internal/test_loader.c)
target_link_libraries(test_loader unimpi)
add_test(NAME test_loader COMMAND test_loader)

# Fake MPI library for lifecycle testing
add_library(fake_mpi SHARED fixtures/fake_mpi.c)
set_target_properties(fake_mpi PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

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
if(UNIMPI_REAL_MPI_TESTS_ENABLED)
    add_test(NAME test_lifecycle COMMAND test_lifecycle)
endif()

# Vtable validation test
add_executable(test_vtable_validation internal/test_vtable_validation.c)
target_link_libraries(test_vtable_validation unimpi)
add_test(NAME test_vtable_validation COMMAND test_vtable_validation)

# Backend identification test
add_executable(test_backend_identification internal/test_backend_identification.c)
target_link_libraries(test_backend_identification unimpi)
add_test(NAME test_backend_identification
    COMMAND test_backend_identification
            $<TARGET_FILE:fake_openmpi_identity>
            $<TARGET_FILE:fake_mpich_identity>
            $<TARGET_FILE:fake_intelmpi_identity>
            $<TARGET_FILE:fake_unknown_identity>)

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
add_test(NAME test_memory_safety COMMAND test_memory_safety)

# Platform tests
add_executable(test_platform internal/test_platform.c)
target_link_libraries(test_platform unimpi)
add_test(NAME test_platform COMMAND test_platform)

# ============================================
# Benchmarks (not run by ctest by default)
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

add_executable(bench_latency benchmark/bench_latency.c)
target_link_libraries(bench_latency unimpi)

add_executable(bench_collective benchmark/bench_collective.c)
target_link_libraries(bench_collective unimpi)

add_executable(bench_overhead benchmark/bench_overhead.c)
target_link_libraries(bench_overhead unimpi)
