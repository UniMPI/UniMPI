add_executable(bench_latency
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/benchmark/bench_latency.c)
target_link_libraries(bench_latency PRIVATE unimpi)

add_executable(bench_collective
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/benchmark/bench_collective.c)
target_link_libraries(bench_collective PRIVATE unimpi)

add_executable(bench_overhead
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/benchmark/bench_overhead.c)
target_link_libraries(bench_overhead PRIVATE unimpi)
