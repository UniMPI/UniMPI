# Function pointer style example
add_executable(minimal minimal.c)
target_link_libraries(minimal unimpi)

# Standard MPI style example
add_executable(std_style std_style.c)
target_link_libraries(std_style unimpi)
target_compile_definitions(std_style PRIVATE UNIMPI_USE_STD_NAMES)

# Non-blocking communication example
# nonblocking example uses MPI-3.0 non-blocking collectives (Ibarrier, etc.);
# only register it when the target version is at least MPI-3.0.
if(NOT (UNIMPI_MPI_TARGET_VERSION LESS 3))
    add_executable(nonblocking nonblocking.c)
    target_link_libraries(nonblocking unimpi)
    target_compile_definitions(nonblocking PRIVATE UNIMPI_USE_STD_NAMES)
endif()

# Collective operations example
add_executable(collective collective.c)
target_link_libraries(collective unimpi)
target_compile_definitions(collective PRIVATE UNIMPI_USE_STD_NAMES)

# RMA (One-sided) example
add_executable(rma rma.c)
target_link_libraries(rma unimpi)
target_compile_definitions(rma PRIVATE UNIMPI_USE_STD_NAMES)

# Runtime backend information example
add_executable(backend_info backend_info.c)
target_link_libraries(backend_info unimpi)

# Thread support negotiation example
add_executable(thread_init thread_init.c)
target_link_libraries(thread_init unimpi)
