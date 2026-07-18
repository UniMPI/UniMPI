# Simple test runner - uses assertions
enable_testing()

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

# Vtable validation test (placeholder)
add_executable(test_vtable_validation unit/test_vtable_validation.c)
target_link_libraries(test_vtable_validation unimpi)
add_test(NAME test_vtable_validation COMMAND test_vtable_validation)

# Point-to-point communication tests
add_executable(test_p2p unit/test_p2p.c)
target_link_libraries(test_p2p unimpi)
add_test(NAME test_p2p COMMAND mpirun -np 2 ./test_p2p)

# Collective communication tests
add_executable(test_collective unit/test_collective.c)
target_link_libraries(test_collective unimpi)
add_test(NAME test_collective COMMAND mpirun -np 4 ./test_collective)

# Datatype tests
add_executable(test_datatype unit/test_datatype.c)
target_link_libraries(test_datatype unimpi)
add_test(NAME test_datatype COMMAND mpirun -np 2 ./test_datatype)

# Benchmarks
add_executable(bench_latency benchmark/bench_latency.c)
target_link_libraries(bench_latency unimpi)

add_executable(bench_collective benchmark/bench_collective.c)
target_link_libraries(bench_collective unimpi)

add_executable(bench_overhead benchmark/bench_overhead.c)
target_link_libraries(bench_overhead unimpi)
