# UniMPI Documentation

UniMPI exposes a runtime-populated MPI dispatch table across four backend
families. Documentation distinguishes API inventory from behavior verified by
tests; a symbol or vtable field is not automatically a support guarantee.

## Start here

- [BUILDING.md](BUILDING.md) — configure, build, install, and consume UniMPI.
- [BACKENDS.md](BACKENDS.md) — select a matching launcher and runtime library.
- [TESTING.md](TESTING.md) — unit/fake and real-MPI test layers.
- [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md) — current backend/API verification
  boundary.
- [../examples/README.md](../examples/README.md) — runnable examples and rank
  requirements.

## API and architecture

- [API.md](API.md) — public lifecycle, diagnostics, and dispatch APIs.
- [mpi-api-summary.md](mpi-api-summary.md) — concise standard-name reference
  with a support disclaimer.
- [MPI_SUPPORT_ANALYSIS.md](MPI_SUPPORT_ANALYSIS.md) — implementation inventory
  and known gaps.
- [design.md](design.md) — loader, lifecycle, vtable, and platform design.
- [mpi-references.md](mpi-references.md) — official MPI and vendor references.

## Operations and development

- [BENCHMARKS.md](BENCHMARKS.md) — benchmark CLI and measurement methodology.
- [CI_CD.md](CI_CD.md) — GitHub Actions backend matrix.
- [WINDOWS.md](WINDOWS.md) — Windows/MS-MPI setup.
- [CONTRIBUTING.md](CONTRIBUTING.md) — contribution and test requirements.

The documents under `docs/superpowers/plans/` are historical implementation
plans. They explain earlier design intent and must not be treated as the
current support contract.
