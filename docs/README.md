# UNIMPI Documentation

Welcome to the UNIMPI documentation. This directory contains comprehensive documentation for the Universal MPI wrapper library.

## Quick Start

New to UNIMPI? Start here:

- [BUILDING.md](BUILDING.md) - How to build and install UNIMPI
- [API.md](API.md) - UNIMPI API documentation
- [WINDOWS.md](WINDOWS.md) - Windows-specific instructions

## MPI Reference Documentation

We provide two levels of MPI documentation:

### 1. Quick Reference (Offline)
- [mpi-api-summary.md](mpi-api-summary.md) - Concise reference for 50+ most common MPI functions
  - Environment management
  - Point-to-point communication
  - Collective operations
  - Datatypes and constants

### 2. Official Links (Online)
- [mpi-references.md](mpi-references.md) - Comprehensive links to:
  - MPI standard documents (PDF/HTML)
  - Implementation documentation (OpenMPI, MPICH, Intel MPI, MS-MPI)
  - Tutorials and learning resources
  - Quick reference cards

## Developer Documentation

- [design.md](design.md) - Architecture and design decisions
- [BACKENDS.md](BACKENDS.md) - Backend implementation details
- [CI_CD.md](CI_CD.md) - Continuous integration setup
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines

## Documentation Structure

```
docs/
├── README.md              # This file
├── BUILDING.md            # Build instructions
├── API.md                 # UNIMPI API docs
├── BACKENDS.md            # Backend docs
├── WINDOWS.md             # Windows guide
├── design.md              # Architecture
├── CI_CD.md               # CI/CD docs
├── CONTRIBUTING.md        # Contributing guide
├── mpi-references.md      # MPI official links
└── mpi-api-summary.md     # MPI API quick reference
```
