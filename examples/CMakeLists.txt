# Function pointer style example
add_executable(minimal minimal.c)
target_link_libraries(minimal unimpi)

# Standard MPI style example
add_executable(std_style std_style.c)
target_link_libraries(std_style unimpi)
target_compile_definitions(std_style PRIVATE UNIMPI_USE_STD_NAMES)

# Non-blocking communication example
add_executable(nonblocking nonblocking.c)
target_link_libraries(nonblocking unimpi)
target_compile_definitions(nonblocking PRIVATE UNIMPI_USE_STD_NAMES)

# Collective operations example
add_executable(collective collective.c)
target_link_libraries(collective unimpi)
target_compile_definitions(collective PRIVATE UNIMPI_USE_STD_NAMES)

# RMA (One-sided) example
add_executable(rma rma.c)
target_link_libraries(rma unimpi)
target_compile_definitions(rma PRIVATE UNIMPI_USE_STD_NAMES)
