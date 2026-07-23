# Dynamic Processes Implementation Design

> **Date:** 2026-07-22
> **Feature:** MPI 2.2 Dynamic Processes Support
> **Scope:** Complete implementation with full test coverage

---

## Goal

Complete MPI 2.2 Dynamic Processes support by adding `MPI_Comm_get_parent` and creating a comprehensive test suite that validates all dynamic process operations across OpenMPI, MPICH, Intel-MPI, and MS-MPI backends.

---

## Architecture

### Core Components

```
include/unimpi_vtable.h          # Add comm_get_parent function pointer
include/unimpi_std_macros.h      # Add MPI_Comm_get_parent macro
src/backends/openmpi.c           # Load MPI_Comm_get_parent symbol
src/backends/mpich.c             # Load MPI_Comm_get_parent symbol
src/backends/intelmpi.c          # Load MPI_Comm_get_parent symbol
src/backends/msmpi.c             # Load MPI_Comm_get_parent symbol
tests/mpi/test_dynamic.c         # Main test program (spawn/connect tests)
tests/mpi/test_spawn_child.c     # Child executable for spawn tests
tests/CMakeLists.txt             # Build rules and test registration
```

### Function Signatures

```c
/* MPI_Comm_get_parent - Get parent intercommunicator in spawned process */
int MPI_Comm_get_parent(MPI_Comm *parent);

/* Existing functions (already in vtable) */
int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs,
                   MPI_Info info, int root, MPI_Comm comm,
                   MPI_Comm *intercomm, int array_of_errcodes[]);
int MPI_Comm_spawn_multiple(int count, char *array_of_commands[],
                            char **array_of_argv[],
                            const int array_of_maxprocs[],
                            const MPI_Info array_of_info[], int root,
                            MPI_Comm comm, MPI_Comm *intercomm,
                            int array_of_errcodes[]);
int MPI_Comm_connect(const char *port_name, MPI_Info info, int root,
                     MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_accept(const char *port_name, MPI_Info info, int root,
                    MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_disconnect(MPI_Comm *comm);
int MPI_Comm_join(int fd, MPI_Comm *intercomm);
int MPI_Open_port(MPI_Info info, char *port_name);
int MPI_Close_port(const char *port_name);
int MPI_Publish_name(const char *service_name, MPI_Info info,
                     const char *port_name);
int MPI_Unpublish_name(const char *service_name, MPI_Info info,
                       const char *port_name);
int MPI_Lookup_name(const char *service_name, MPI_Info info,
                    char *port_name);
```

---

## Global Constraints

- **Zero-overhead design:** Function pointer dispatch only, no additional wrapping
- **Cross-platform:** Linux, macOS, Windows (MS-MPI)
- **Backend compatibility:** All 4 backends must load symbols; tests handle backend-specific limitations
- **Test isolation:** Each test case is independent, failures don't cascade
- **Timeout handling:** Spawn tests must include timeout to prevent hangs

---

## Test Suite Design

### Test Programs

#### 1. test_spawn_child.c

**Purpose:** Child executable spawned by test_dynamic.c

**Behavior:**
- Calls `MPI_Comm_get_parent` to get parent intercommunicator
- Exchanges simple message with parent (ping/pong)
- Returns success/failure code via exit status

**Key Operations:**
```c
MPI_Comm parent;
MPI_Comm_get_parent(&parent);  /* Get parent intercommunicator */
/* Exchange data with parent */
MPI_Comm_disconnect(&parent);
```

#### 2. test_dynamic.c

**Purpose:** Main test program validating all dynamic process operations

**Test Cases:**

| Test | Function(s) | Description | Expected Result |
|------|-------------|-------------|-----------------|
| test_spawn_single | Comm_spawn | Spawn single child process | Child launches, exchanges message |
| test_spawn_multiple | Comm_spawn_multiple | Spawn multiple children with different args | All children launch |
| test_port_management | Open_port, Close_port | Create and close port | Port name generated, no error |
| test_publish_lookup | Publish_name, Lookup_name | Register and find service | Service name resolved to port |
| test_connect_accept | Connect, Accept | Client-server connection | Communication established |
| test_join | Comm_join | Socket-based connection | Communication established |
| test_disconnect | Comm_disconnect | Clean disconnection | No resource leaks |

**Backend-specific handling:**
- MS-MPI: Skip spawn tests (limited support)
- OpenMPI: Full test suite
- MPICH/IntelMPI: Full test suite (with environment checks)

### CMake Integration

```cmake
# Build child executable
add_executable(test_spawn_child mpi/test_spawn_child.c)
target_link_libraries(test_spawn_child unimpi)

# Build main test (runs with -np 1, spawns children)
add_executable(test_dynamic mpi/test_dynamic.c)
target_link_libraries(test_dynamic unimpi)

# Custom test command for spawn tests
add_mpi_spawn_test(test_dynamic test_dynamic test_spawn_child)
```

---

## Backend Implementation

### OpenMPI

```c
/* Symbol: MPI_Comm_get_parent */
unimpi.comm_get_parent = (int (*)(MPI_Comm*))
    unimpi_platform_dlsym(handle, "MPI_Comm_get_parent");
```

### MPICH/IntelMPI

```c
/* Symbol: MPI_Comm_get_parent (same as OpenMPI) */
unimpi.comm_get_parent = (int (*)(MPI_Comm*))
    unimpi_platform_dlsym(handle, "MPI_Comm_get_parent");
```

### MS-MPI

```c
/* Symbol: MPI_Comm_get_parent (if available) */
/* Note: MS-MPI has limited spawn support */
unimpi.comm_get_parent = (int (*)(MPI_Comm*))
    unimpi_platform_dlsym(handle, "MPI_Comm_get_parent");
/* Function pointer may be NULL on older MS-MPI */
```

---

## Error Handling

### Backend Not Supporting Feature

If a backend doesn't support a dynamic process function (returns NULL from dlsym):
- Set function pointer to NULL
- Application calling the function will get undefined behavior (same as native MPI)
- Tests should check for NULL and skip if unsupported

### Test Failures

- **Spawn failure:** Report error, don't fail entire suite
- **Timeout:** Use alarm() or MPI timeout mechanisms
- **Backend mismatch:** Skip tests with clear message

---

## Documentation Updates

### MPI_SUPPORT_ANALYSIS.md

Update Dynamic Processes section:
- Mark all 8 functions as ✅ supported
- Add test coverage note

---

## Implementation Order

1. Add `comm_get_parent` to vtable header
2. Add macro to std_macros header
3. Load symbol in all 4 backend files
4. Create test_spawn_child.c
5. Create test_dynamic.c
6. Update CMakeLists.txt
7. Run tests on all backends
8. Update documentation

---

## Testing Checklist

- [ ] OpenMPI: All tests pass
- [ ] MPICH: All tests pass
- [ ] IntelMPI: All tests pass
- [ ] MS-MPI: Graceful degradation (skip unsupported)

---

## References

- MPI 2.2 Standard, Chapter 5: Process Management
- OpenMPI: `mpi.h` spawn functions
- MPICH: `mpi.h` process management
- MS-MPI: Windows HPC documentation
