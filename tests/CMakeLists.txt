# Simple test runner - uses assertions
enable_testing()

# Error handling test
add_executable(test_error unit/test_error.c)
target_link_libraries(test_error tftk_mpi)
add_test(NAME test_error COMMAND test_error)

# Loader test
add_executable(test_loader unit/test_loader.c)
target_link_libraries(test_loader tftk_mpi)
add_test(NAME test_loader COMMAND test_loader)
