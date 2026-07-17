# GitLab CI/CD Configuration

This document describes the GitLab CI/CD setup for `unimpi`.

---

## Overview

The CI/CD pipeline automatically:
- Builds the project with multiple MPI backends (OpenMPI, MPICH)
- Runs the full test suite on each backend
- Executes performance benchmarks
- Performs code quality checks

---

## Pipeline Stages

```
build → test → quality
```

### Build Stage
- **build:openmpi** - Build with OpenMPI
- **build:mpich** - Build with MPICH

### Test Stage
- **test:openmpi** - Run tests with OpenMPI backend
- **test:mpich** - Run tests with MPICH backend
- **benchmark:openmpi** - Run performance benchmarks (optional)

### Quality Stage
- **code_style** - Check code formatting with clang-format
- **static_analysis** - Run cppcheck static analysis
- **release** - Create release artifacts (tags only)

---

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `BUILD_DIR` | `build` | CMake build directory |
| `MPI_PROCS` | `4` | Number of MPI processes for tests |
| `UNIMPI_BACKEND` | auto | Backend to use for testing |

### Runners

Jobs require GitLab runners with:
- Linux OS
- Docker executor
- Access to Ubuntu 22.04 images

### Caching

Build artifacts are cached between jobs on the same branch to speed up builds.

---

## Usage

### View Pipeline Status

1. Go to **Project** → **CI/CD** → **Pipelines**
2. View current and past pipeline runs
3. Click on a pipeline to see job details

### Manual Jobs

Some jobs are manual:
- **benchmark:openmpi** - Run after successful tests
- **release** - Only runs on git tags

### Trigger Pipeline

Pipelines run automatically on:
- Every push to any branch
- Merge requests
- Git tags

Manual trigger:
1. Go to **CI/CD** → **Pipelines**
2. Click **Run pipeline**
3. Select branch and variables
4. Click **Run pipeline**

---

## Local Testing with GitLab Runner

Install GitLab runner locally to test CI configuration:

```bash
# Install GitLab runner
sudo curl -L --output /usr/local/bin/gitlab-runner \
    https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-linux-amd64
sudo chmod +x /usr/local/bin/gitlab-runner

# Register runner (optional for local testing)
gitlab-runner register

# Test locally
gitlab-runner exec docker build:openmpi
```

---

## Extending CI/CD

### Adding a New Backend

1. Add build job:
```yaml
build:intelmpi:
  <<: *linux_mpi
  stage: build
  image: intel/oneapi-basekit:latest
  script:
    - source /opt/intel/oneapi/setvars.sh
    - cmake -B ${BUILD_DIR} .
    - cmake --build ${BUILD_DIR}
```

2. Add test job:
```yaml
test:intelmpi:
  <<: *linux_mpi
  stage: test
  image: intel/oneapi-basekit:latest
  needs:
    - build:intelmpi
  variables:
    UNIMPI_BACKEND: intelmpi
  script:
    - source /opt/intel/oneapi/setvars.sh
    - cd ${BUILD_DIR}
    - ctest --output-on-failure
```

### Adding Tests

New tests are automatically picked up by `ctest` if added to `tests/CMakeLists.txt`.

### Custom Variables

Create `.gitlab-ci-local.yml` for local overrides:

```yaml
variables:
  MPI_PROCS: 2  # Use fewer processes locally
```

---

## Troubleshooting

### Job Fails with "mpirun not found"

Ensure MPI runtime is installed:
```yaml
before_script:
  - apt-get update && apt-get install -y libopenmpi-dev
```

### Permission Denied for mpirun

OpenMPI requires special flags in Docker:
```bash
mpirun --allow-run-as-root -np 4 ./test
```

### Cache Not Working

Clear cache from **Project** → **CI/CD** → **Pipelines** → **Clear runner caches**.

---

## Badges

Add these badges to your README:

```markdown
[![pipeline status](https://gitlab.com/YOUR_USERNAME/unimpi/badges/main/pipeline.svg)](https://gitlab.com/YOUR_USERNAME/unimpi/-/commits/main)
[![coverage report](https://gitlab.com/YOUR_USERNAME/unimpi/badges/main/coverage.svg)](https://gitlab.com/YOUR_USERNAME/unimpi/-/commits/main)
```

---

## See Also

- [.gitlab-ci.yml](../.gitlab-ci.yml) - Pipeline configuration
- [BUILDING.md](BUILDING.md) - Manual build instructions
- [BACKENDS.md](BACKENDS.md) - Backend information
