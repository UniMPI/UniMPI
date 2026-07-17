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
