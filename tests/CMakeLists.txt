# Tests CMakeLists.txt
enable_testing()

# Find MPI for testing
find_package(MPI COMPONENTS C)

# Configure MPI executor (for CI)
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
else()
    message(WARNING "MPI executor not found - MPI tests will be disabled")
endif()

# Parse preflags if provided
if(MPIEXEC_PREFLAGS)
    separate_arguments(MPIEXEC_PREFLAGS_LIST UNIX_COMMAND "${MPIEXEC_PREFLAGS}")
else()
    set(MPIEXEC_PREFLAGS_LIST "")
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

# Error handling test
add_executable(test_error unit/test_error.c)
target_link_libraries(test_error unimpi)
add_test(NAME test_error COMMAND test_error)

# Loader test
add_executable(test_loader unit/test_loader.c)
target_link_libraries(test_loader unimpi)
add_test(NAME test_loader COMMAND test_loader)

# Fake MPI library for lifecycle testing
add_library(fake_mpi SHARED fixtures/fake_mpi.c)
set_target_properties(fake_mpi PROPERTIES
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Lifecycle tests
add_executable(test_lifecycle unit/test_lifecycle.c)
target_link_libraries(test_lifecycle unimpi)
add_test(NAME test_lifecycle COMMAND test_lifecycle)

add_executable(test_lifecycle_macros unit/test_lifecycle_macros.c)
target_link_libraries(test_lifecycle_macros unimpi)
add_test(NAME test_lifecycle_macros_init
    COMMAND test_lifecycle_macros init)
add_test(NAME test_lifecycle_macros_thread
    COMMAND test_lifecycle_macros thread)

# Vtable validation test
add_executable(test_vtable_validation unit/test_vtable_validation.c)
target_link_libraries(test_vtable_validation unimpi)
add_test(NAME test_vtable_validation COMMAND test_vtable_validation)

# Point-to-point communication tests
add_executable(test_p2p unit/test_p2p.c)
target_link_libraries(test_p2p unimpi)
add_mpi_test(test_p2p test_p2p 2)

# Collective communication tests
add_executable(test_collective unit/test_collective.c)
target_link_libraries(test_collective unimpi)
add_mpi_test(test_collective test_collective 4)

# Datatype tests
add_executable(test_datatype unit/test_datatype.c)
target_link_libraries(test_datatype unimpi)
add_mpi_test(test_datatype test_datatype 2)

# Benchmarks (not run by ctest by default)
add_executable(bench_latency benchmark/bench_latency.c)
target_link_libraries(bench_latency unimpi)

add_executable(bench_collective benchmark/bench_collective.c)
target_link_libraries(bench_collective unimpi)

add_executable(bench_overhead benchmark/bench_overhead.c)
target_link_libraries(bench_overhead unimpi)
