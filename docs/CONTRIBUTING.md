# Contributing to unimpi

Thank you for your interest in contributing to `unimpi`! This document provides guidelines for contributing.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Contribute](#how-to-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)

---

## Code of Conduct

This project adheres to a code of conduct that we expect all contributors to follow:

- Be respectful and constructive
- Welcome newcomers
- Focus on what is best for the community
- Show empathy towards others

---

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](../../issues)
2. If not, create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - Environment details (OS, MPI version, compiler)

### Suggesting Features

1. Check if the feature has already been suggested
2. Create a new issue with the "feature request" label
3. Describe the feature and its use case

### Contributing Code

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes
4. Test your changes
5. Submit a pull request

---

## Development Setup

### Prerequisites

- CMake >= 3.10
- C99 compatible compiler
- At least one MPI implementation (OpenMPI, MPICH, Intel-MPI)

### Building from Source

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/unimpi.git
cd unimpi

# Configure
cmake -B build .

# Build
cmake --build build

# Run tests
cd build && ctest --output-on-failure
```

### Running with Different Backends

```bash
# Test with OpenMPI
UNIMPI_BACKEND=openmpi ctest

# Test with MPICH
UNIMPI_BACKEND=mpich ctest

# Test with Intel MPI
UNIMPI_BACKEND=intelmpi ctest
```

---

## Coding Standards

### C Code Style

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style
  ```c
  int function(void) {
      if (condition) {
          do_something();
      }
      return 0;
  }
  ```
- **Line length**: Max 100 characters
- **Naming**:
  - Functions: `snake_case`
  - Macros: `UPPER_CASE`
  - Types: `snake_case_t`
  - Global vtable: `unimpi`

### Example

```c
/* Function to initialize backend */
int unimpi_backend_init(const char *name) {
    if (!name) {
        return UNIMPI_ERR_INVALID_ARGUMENT;
    }
    
    for (int i = 0; i < UNIMPI_MAX_BACKENDS; i++) {
        if (strcmp(unimpi_backends[i].name, name) == 0) {
            return unimpi_loader_load(unimpi_backends[i].lib_name);
        }
    }
    
    return UNIMPI_ERR_NO_BACKEND;
}
```

### Documentation

- Add comments for non-obvious code
- Document function parameters
- Update relevant `.md` files for new features

---

## Testing

### Test Requirements

All new features must include tests:

- Unit tests in `tests/unit/`
- Integration tests if applicable
- Test with all supported backends

### Writing Tests

Example test structure:

```c
/* tests/unit/test_my_feature.c */
#include <stdio.h>
#include <assert.h>
#include "unimpi.h"

void test_my_feature(void) {
    // Test code here
    assert(condition);
    printf("My feature test passed\n");
}

int main(int argc, char **argv) {
    int ret = unimpi_init(&argc, &argv);
    if (ret != UNIMPI_OK) {
        fprintf(stderr, "Init failed: %s\n", unimpi_error_string(ret));
        return 1;
    }
    
    test_my_feature();
    
    unimpi_finalize();
    return 0;
}
```

### Adding Tests to CMake

Edit `tests/CMakeLists.txt`:

```cmake
# My feature test
add_executable(test_my_feature unit/test_my_feature.c)
target_link_libraries(test_my_feature unimpi)
add_test(NAME test_my_feature COMMAND test_my_feature)
```

---

## Backend Development

### Adding a New Backend

1. Create `src/backends/mybackend.c`
2. Implement `unimpi_vtable_init_mybackend()`
3. Update `src/loader.c` with detection logic
4. Update `include/unimpi_loader.h`
5. Add backend info to documentation
6. Write tests

### Backend Template

```c
/* src/backends/mybackend.c */
#include "unimpi_vtable.h"
#include "unimpi_platform.h"
#include "unimpi.h"

int unimpi_vtable_init_mybackend(unimpi_lib_handle_t handle) {
    /* Environment Management */
    unimpi.init = (int (*)(int*, char***))
        unimpi_platform_dlsym(handle, "MPI_Init");
    // ... more functions
    
    /* Set predefined communicator values */
    UNIMPI_COMM_WORLD = get_mybackend_comm_world();
    UNIMPI_COMM_SELF = get_mybackend_comm_self();
    
    return UNIMPI_OK;
}
```

---

## Submitting Changes

### Pull Request Process

1. **Before submitting:**
   - Run all tests
   - Update documentation
   - Add tests for new features
   - Follow coding standards

2. **PR description should include:**
   - What changed
   - Why it changed
   - How to test it
   - Related issues

3. **Review process:**
   - Maintainers will review
   - Address feedback
   - Once approved, maintainers will merge

### Commit Messages

Follow conventional commits format:

```
type: short description

Longer explanation if needed.

- Bullet points for details
- Another point

Fixes #123
```

Types:
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation
- `test:` - Tests
- `refactor:` - Code refactoring
- `perf:` - Performance
- `chore:` - Maintenance

Examples:
```
feat: add CUDA-aware backend detection

fix: handle NULL pointer in unimpi_init

docs: update Windows build instructions
```

---

## Release Process

1. Update version in `include/unimpi.h`
2. Update `CHANGELOG.md`
3. Tag release: `git tag v0.2.0`
4. Push tags: `git push origin --tags`
5. Create GitHub release

---

## Questions?

- Open an [issue](../../issues)
- Check existing [discussions](../../discussions)
- Read the [documentation](../README.md)

Thank you for contributing to unimpi!
