#!/bin/bash
# scripts/coverage_report.sh - Generate test coverage report

set -e

echo "=== UniMPI Test Coverage Report ==="

# Create build directory with coverage flags
rm -rf build-coverage
mkdir -p build-coverage
cd build-coverage

# Configure with coverage flags
cmake .. -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="--coverage" \
    -DCMAKE_CXX_FLAGS="--coverage" \
    -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
    -DUNIMPI_BUILD_TESTS=ON \
    -DUNIMPI_BUILD_MPI_TESTS=OFF  # Coverage tests don't need real MPI

# Build
make -j$(nproc)

# Run tests
echo ""
echo "=== Running tests for coverage ==="
ctest --output-on-failure

# Generate coverage report
echo ""
echo "=== Generating coverage report ==="
gcov -o . ../src/*.c ../src/backends/*.c 2>/dev/null || true

# Generate HTML report with lcov if available
if command -v lcov &> /dev/null; then
    echo "Using lcov for detailed report..."
    lcov --capture --directory . --output-file coverage.info 2>/dev/null || true
    lcov --remove coverage.info '/usr/*' --output-file coverage.info 2>/dev/null || true

    if command -v genhtml &> /dev/null; then
        genhtml coverage.info --output-directory coverage_html 2>/dev/null || true
        echo ""
        echo "✅ HTML coverage report generated in build-coverage/coverage_html/"
        echo "   Open build-coverage/coverage_html/index.html to view"
    fi

    # Show summary
    echo ""
    echo "=== Coverage Summary ==="
    lcov --summary coverage.info 2>/dev/null || echo "Summary not available"
else
    echo "lcov not installed. Installing..."
    echo "  Ubuntu/Debian: sudo apt-get install lcov"
    echo "  macOS: brew install lcov"
    echo ""
    echo "Showing raw gcov output (limited):"
    find . -name "*.gcov" -exec head -20 {} \; 2>/dev/null | head -100 || true
fi

cd ..
echo ""
echo "Coverage report complete!"
