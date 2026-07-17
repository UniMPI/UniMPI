# Backend Selection Guide

`unimpi` supports four MPI implementations. This guide helps you choose and configure the right backend for your needs.

---

## Quick Comparison

| Feature | OpenMPI | MPICH | Intel-MPI | MS-MPI |
|---------|---------|-------|-----------|--------|
| **Platform** | Linux, macOS | Linux, macOS | Linux | Windows |
| **Performance** | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Ease of Use** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Feature Set** | Full | Full | Full + Intel optim | Full |
| **Typical Use** | Academic | Research | HPC/Enterprise | Windows |

---

## Automatic Detection

`unimpi` automatically detects available backends in this order:

1. **User-specified** (via `UNIMPI_BACKEND` or `UNIMPI_LIBRARY`)
2. **OpenMPI** (`libmpi.so.40`)
3. **Intel-MPI** (`libmpi.so`)
4. **MPICH** (`libmpich.so`)
5. **MS-MPI** (`msmpi.dll`) - Windows only

---

## Manual Backend Selection

### Using Environment Variables

```bash
# Select by name
export UNIMPI_BACKEND=openmpi
export UNIMPI_BACKEND=mpich
export UNIMPI_BACKEND=intelmpi
export UNIMPI_BACKEND=msmpi    # Windows

# Or specify exact library path
export UNIMPI_LIBRARY=/opt/openmpi/lib/libmpi.so.40
export UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so
export UNIMPI_LIBRARY=/usr/lib/x86_64-linux-gnu/libmpich.so
```

### Runtime Switching

```bash
# Run with OpenMPI
UNIMPI_BACKEND=openmpi ./my_app

# Run with Intel MPI
UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so ./my_app

# Run with MPICH
UNIMPI_BACKEND=mpich mpirun -np 4 ./my_app
```

---

## Backend Details

### OpenMPI

**Best for:** General purpose, academic use, Linux clusters

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install libopenmpi-dev

# CentOS/RHEL
sudo yum install openmpi-devel

# macOS (Homebrew)
brew install openmpi
```

**Configuration:**
```bash
export UNIMPI_BACKEND=openmpi

# Or specify version
export UNIMPI_LIBRARY=/usr/lib/x86_64-linux-gnu/libmpi.so.40
```

**Special Notes:**
- Uses pointers for MPI handles
- Library name varies by version (`.so.40` for v4.x, `.so.20` for v3.x)
- Good default choice on Linux

**Performance Characteristics:**
- Excellent for InfiniBand networks
- Good startup times
- Scalable to thousands of processes

---

### MPICH

**Best for:** Research, portability, embedded systems

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install libmpich-dev

# CentOS/RHEL
sudo yum install mpich-devel

# macOS (Homebrew)
brew install mpich
```

**Configuration:**
```bash
export UNIMPI_BACKEND=mpich
export UNIMPI_LIBRARY=/usr/lib/x86_64-linux-gnu/libmpich.so
```

**Special Notes:**
- Uses integers for MPI handles
- More strict MPI compliance
- Often used as base for other implementations

**Performance Characteristics:**
- Excellent for TCP networks
- Very portable
- Good for development/debugging

---

### Intel-MPI

**Best for:** High-performance computing, Intel hardware

**Installation:**
```bash
# Download from Intel oneAPI
# https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library.html

# Or use package manager
# Ubuntu/Debian (via Intel repository)
sudo apt-get install intel-oneapi-mpi-devel
```

**Configuration:**
```bash
# Source Intel environment
source /opt/intel/oneapi/setvars.sh

# unimpi will auto-detect
export UNIMPI_BACKEND=intelmpi

# Or explicitly
export UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so
```

**Special Notes:**
- Based on MPICH (integer handles)
- Includes Intel-specific optimizations
- Best performance on Intel CPUs and fabrics
- Requires `setvars.sh` or manual path configuration

**Performance Characteristics:**
- ⭐⭐⭐⭐⭐ Best overall performance
- Optimized for Intel Omni-Path
- Excellent for large-scale HPC

---

### MS-MPI

**Best for:** Windows development, .NET integration

**Installation:**
```powershell
# Download from Microsoft
# https://www.microsoft.com/download/details.aspx?id=57467

# Install both:
# 1. MS-MPI SDK (for development)
# 2. MS-MPI Runtime (for execution)
```

**Configuration:**
```cmd
set UNIMPI_BACKEND=msmpi

:: Or specify path
set UNIMPI_LIBRARY=C:\Program Files\Microsoft MPI\Bin\msmpi.dll
```

**Special Notes:**
- Windows only
- Based on MPICH
- Integrates with Visual Studio
- Handles use MS-MPI specific values (0x44000000)

**Build Requirements:**
```cmd
:: Set environment for CMake
set MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include
set MSMPI_LIB64=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64
```

**Performance Characteristics:**
- Good for Windows HPC
- Compatible with Linux MPI codes
- Scales to Azure cloud

---

## Backend-Specific Features

| Feature | OpenMPI | MPICH | Intel-MPI | MS-MPI |
|---------|---------|-------|-----------|--------|
| CUDA-aware | ✅ | ✅ | ✅ | ✅ |
| InfiniBand | Native | ✅ (via libfabric) | Native | Via WARP |
| Process affinities | Good | Good | Excellent | Good |
| Debugging | Good | Excellent | Good | Good |
| Fault tolerance | ✅ | ❌ | ✅ | ✅ |

---

## Choosing a Backend

### For Development

**Recommendation:** MPICH or OpenMPI

- Strict MPI compliance for portability
- Good debugging support
- Easy to install

```bash
export UNIMPI_BACKEND=mpich
```

### For Production HPC

**Recommendation:** Intel-MPI (Intel CPUs) or OpenMPI (other)

- Maximum performance
- Optimized for specific hardware
- Production-grade reliability

```bash
source /opt/intel/oneapi/setvars.sh
export UNIMPI_BACKEND=intelmpi
```

### For Windows

**Recommendation:** MS-MPI

- Native Windows support
- Visual Studio integration
- Azure cloud ready

```cmd
set UNIMPI_BACKEND=msmpi
```

### For Teaching

**Recommendation:** OpenMPI

- Widely documented
- Simple installation
- Large community

```bash
export UNIMPI_BACKEND=openmpi
```

---

## Testing Different Backends

```bash
#!/bin/bash
# test_backends.sh

BACKENDS=("openmpi" "mpich" "intelmpi")

for backend in "${BACKENDS[@]}"; do
    echo "Testing with $backend..."
    UNIMPI_BACKEND=$backend mpirun -np 4 ./my_app
done
```

---

## Troubleshooting

### Backend Not Found

```bash
# Check if library exists
ldconfig -p | grep libmpi

# Set explicit path
export UNIMPI_LIBRARY=/usr/lib/x86_64-linux-gnu/libmpi.so.40
```

### Wrong Backend Detected

```bash
# Force specific backend
export UNIMPI_BACKEND=intelmpi

# Or disable auto-detection with explicit path
unset UNIMPI_BACKEND
export UNIMPI_LIBRARY=/opt/intel/oneapi/mpi/latest/lib/libmpi.so
```

### Intel MPI Not Detected

```bash
# Ensure Intel environment is loaded
source /opt/intel/oneapi/setvars.sh

# Verify library exists
ls $I_MPI_ROOT/lib/libmpi.so

# Set manually
export UNIMPI_LIBRARY=$I_MPI_ROOT/lib/libmpi.so
```

### MS-MPI on Windows

```cmd
:: Verify installation
dir "C:\Program Files\Microsoft MPI\Bin\msmpi.dll"

:: Set in CMake
set MSMPI_DIR=C:\Program Files\Microsoft MPI
cmake -B build -DUNIMPI_BACKEND=msmpi
```

---

## Advanced Configuration

### Multiple Versions

```bash
# Create scripts for each version
# openmpi.sh
export PATH=/opt/openmpi/bin:$PATH
export LD_LIBRARY_PATH=/opt/openmpi/lib:$LD_LIBRARY_PATH
export UNIMPI_BACKEND=openmpi

# intelmpi.sh
source /opt/intel/oneapi/setvars.sh
export UNIMPI_BACKEND=intelmpi
```

### Container Deployment

```dockerfile
# Dockerfile example
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y openmpi-bin libopenmpi-dev
COPY . /unimpi
RUN cd /unimpi && cmake -B build . && cmake --build build
ENV UNIMPI_BACKEND=openmpi
```

---

## See Also

- [BUILDING.md](BUILDING.md) - Build instructions
- [WINDOWS.md](WINDOWS.md) - Windows-specific guide
- [API.md](API.md) - API reference
