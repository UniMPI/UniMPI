# Function pointer style example
add_executable(minimal minimal.c)
target_link_libraries(minimal unimpi)

# Standard MPI style example
add_executable(std_style std_style.c)
target_link_libraries(std_style unimpi)
target_compile_definitions(std_style PRIVATE UNIMPI_USE_STD_NAMES)
