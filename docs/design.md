# UniMPI Design

## Scope

UniMPI is a runtime dispatch layer for native MPI implementations. It provides
one C99-facing library and selects a backend when the process initializes.

The current dispatch structure has 276 MPI function-pointer fields, and the
standard-name header has 247 direct `MPI_*` aliases. These inventory counts do
not imply complete MPI-3/MPI-4 conformance; see
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).

## Components

```text
application
    |
    | unimpi_init / MPI_Init
    v
lifecycle state machine
    |
    +--> loader selection
    |      UNIMPI_LIBRARY > UNIMPI_BACKEND > platform fallback
    |
    +--> platform loader
    |      dlopen/dlsym or LoadLibrary/GetProcAddress
    |
    +--> backend identification
    |      Open MPI / MPICH / Intel MPI / MS-MPI
    |
    +--> backend adapter
           vtable functions + predefined handles/constants
```

Primary source responsibilities:

| Component | Files | Responsibility |
|---|---|---|
| Lifecycle | `src/core.c` | Initialization/finalization state and backend identity |
| Selection | `src/loader.c` | Environment priority, loading, identification, diagnostics |
| Platform | `src/platform_posix.c`, `src/platform_windows.c` | Dynamic-loader abstraction |
| Dispatch | `src/vtable.c`, `include/unimpi_vtable.h` | Global runtime function table |
| Adapters | `src/backends/*.c` | Backend-specific symbols, handles, constants, and ABI details |
| Standard names | `include/unimpi_std_macros.h` | Optional `MPI_*` aliases |

## Backend selection

Selection order is:

1. `UNIMPI_LIBRARY`;
2. `UNIMPI_BACKEND`;
3. `msmpi.dll` on Windows or `libmpi.so.40` on POSIX.

The fallback is only a loader basename. It does not inspect every installed MPI
and is unsuitable for deterministic selection on macOS or multi-MPI Linux
hosts. Exact paths are documented in [BACKENDS.md](BACKENDS.md).

## Lifecycle

The process-wide state machine distinguishes:

- never initialized;
- initializing;
- active;
- initialization failed;
- finalizing;
- finalized;
- finalization failed.

An initialization failure can be retried after cleanup. MPI re-initialization
after successful finalization is rejected. Backend handle, identity strings,
predefined values, and vtable state must be cleaned consistently on every
failure path.

Lifecycle state and the global vtable are process-wide. The implementation does
not claim that concurrent initialization/finalization from multiple application
threads is safe. `unimpi_init_thread` negotiates backend MPI thread support; it
does not by itself make UniMPI lifecycle mutation thread-safe.

## Vtable initialization

Initialization performs:

1. load the chosen shared library;
2. verify core symbols;
3. identify the backend;
4. reject unsupported platform/backend combinations;
5. populate backend-specific predefined handles and constants;
6. resolve vtable entries;
7. invoke `MPI_Init` or `MPI_Init_thread`;
8. publish active backend identity.

Some MPI operations can be absent in a vendor/version even when a corresponding
vtable field exists at compile time. Tests therefore validate a required
profile and exercise focused categories rather than assuming every `dlsym`
succeeded.

## ABI representation

MPI handles are represented with `intptr_t` so the facade can hold both
pointer-style Open MPI objects and integer-style MPICH-derived handles.

`MPI_Status` uses a union with backend-oriented layouts and a 128-byte raw
buffer. This is an internal compatibility strategy, not a portable serialization
format. Applications must not persist or exchange its raw bytes.

Predefined communicator, datatype, operation, request, and info values are
initialized by backend adapters. Code must use the exported values instead of
hard-coded vendor constants.

## API styles

Direct dispatch:

```c
unimpi.send(buffer, count, UNIMPI_INT, destination, tag, UNIMPI_COMM_WORLD);
```

Standard-name aliases:

```c
#define UNIMPI_USE_STD_NAMES
#include "unimpi.h"

MPI_Send(buffer, count, MPI_INT, destination, tag, MPI_COMM_WORLD);
```

Both forms use the same runtime-populated table. Neither may be called before
successful initialization unless the specific control API documents pre-init
behavior.

## Error model

UniMPI control functions return `UNIMPI_*` errors for loader, argument, state,
and initialization failures. Runtime MPI calls return the backend's MPI result
through the vtable.

Backend initialization failure must not leave a partially active identity or
call an MPI error-class function before MPI is ready. Fake libraries provide
deterministic failure injection for these paths.

## Verification strategy

- Fake shared libraries test loader priority, identity, incomplete symbols,
  lifecycle transitions, cleanup, and required vtable fields.
- Real MPI tests verify communication and ABI-sensitive behavior on each
  supported backend/platform combination.
- Sanitizers execute fake/unit tests under strict C99.
- Examples and benchmarks use CTest smoke workloads; benchmark timing is not a
  conformance or hard-threshold check.

Details are in [TESTING.md](TESTING.md) and
[SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).

## Non-goals

UniMPI currently does not promise:

- the full API of any MPI standard revision;
- cross-vendor communication within one MPI job;
- conversion of one vendor's ABI into another;
- thread-safe concurrent lifecycle mutation;
- zero or fixed dispatch overhead;
- fault tolerance, GPU awareness, or vendor extensions;
- semantic coverage for every vtable field.
