# Function pointer style example
add_executable(minimal minimal.c)
target_link_libraries(minimal tftk_mpi)

# Standard MPI style example
add_executable(std_style std_style.c)
target_link_libraries(std_style tftk_mpi)
target_compile_definitions(std_style PRIVATE TFTK_MPI_USE_STD_NAMES)
