# Windows and MS-MPI Guide

Complete guide for building and using `unimpi` on Windows with MS-MPI.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Building with Visual Studio](#building-with-visual-studio)
- [Building with MinGW-w64](#building-with-mingw-w64)
- [Configuration](#configuration)
- [Running Applications](#running-applications)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Software

1. **MS-MPI SDK**
   - Download from: https://www.microsoft.com/download/details.aspx?id=57467
   - Install both:
     - `msmpisdk.msi` (SDK for development)
     - `MSMpiSetup.exe` (Runtime for execution)

2. **Visual Studio** (2017 or later) or **MinGW-w64**

3. **CMake** >= 3.10

### Environment Variables

After installing MS-MPI, ensure these are set:

```cmd
set MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include
set MSMPI_LIB64=C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64
```

Verify installation:

```cmd
dir "%MSMPI_INC%\mpi.h"
dir "%MSMPI_LIB64%\msmpi.lib"
```

---

## Installation

### Option 1: Using Visual Studio Installer

1. Download MS-MPI from Microsoft
2. Run `msmpisdk.msi` and `MSMpiSetup.exe`
3. Restart your computer

### Option 2: Using Chocolatey

```powershell
# Install MS-MPI
choco install msmpi

# Install MS-MPI SDK
choco install msmpi-sdk
```

### Option 3: Using vcpkg

```cmd
vcpkg install msmpi:x64-windows
```

---

## Building with Visual Studio

### Using CMake GUI

1. Open CMake GUI
2. Set source: `C:/path/to/unimpi`
3. Set build: `C:/path/to/unimpi/build`
4. Click "Configure", select "Visual Studio 17 2022" and x64
5. Set options:
   - `UNIMPI_BACKEND=msmpi` (optional, auto-detected)
6. Click "Generate"
7. Open generated `.sln` file in Visual Studio
8. Build → Build Solution

### Using Command Line

```cmd
cd C:\path\to\unimpi

# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Run tests
cd build
ctest -C Release --output-on-failure
```

### Visual Studio Project Settings

If integrating into existing Visual Studio project:

1. **Include Directories:**
   ```
   C:\Program Files (x86)\Microsoft SDKs\MPI\Include
   ```

2. **Library Directories:**
   ```
   C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64
   ```

3. **Additional Dependencies:**
   ```
   msmpi.lib
   unimpi.lib (or unimpi.dll if shared)
   ```

---

## Building with MinGW-w64

### Prerequisites

1. Install MinGW-w64 from https://www.mingw-w64.org/
   or use MSYS2: `pacman -S mingw-w64-x86_64-gcc`

2. Ensure `gcc` and `g++` are in PATH

### Build Steps

```bash
# MSYS2 terminal
cd /c/path/to/unimpi

# Configure with MinGW
cmake -B build -G "MinGW Makefiles" \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_PREFIX_PATH="/c/Program Files/Microsoft SDKs/MPI"

# Build
cmake --build build

# Test
export PATH="/c/Program Files/Microsoft MPI/Bin:$PATH"
cd build && ctest
```

### Cross-compiling from Linux

```bash
# Install MinGW cross-compiler
sudo apt-get install mingw-w64

# Create toolchain file
cat > cmake/toolchain-mingw.cmake << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
EOF

# Configure
cmake -B build-windows \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw.cmake \
    -DUNIMPI_BACKEND=msmpi

# Build
cmake --build build-windows
```

---

## Configuration

### Environment Variables

```cmd
:: Select MS-MPI backend
set UNIMPI_BACKEND=msmpi

:: Or specify library path
set UNIMPI_LIBRARY=C:\Program Files\Microsoft MPI\Bin\msmpi.dll

:: Debug level (0-5)
set UNIMPI_DEBUG=3
```

### CMake Options

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DUNIMPI_BACKEND=msmpi ^
    -DUNIMPI_BUILD_EXAMPLES=ON ^
    -DUNIMPI_BUILD_TESTS=ON ^
    -DCMAKE_INSTALL_PREFIX=C:\Program Files\unimpi
```

---

## Running Applications

### Using mpiexec

```cmd
:: Run on 4 processes
mpiexec -n 4 myapp.exe

:: Run with specific hosts
mpiexec -hosts 2 host1 4 host2 4 myapp.exe

:: Run with machine file
mpiexec -machinefile hosts.txt myapp.exe
```

### Using unimpi with MS-MPI

```c
#include "unimpi.h"

int main(int argc, char **argv) {
    // Will auto-detect MS-MPI on Windows
    unimpi_init(&argc, &argv);
    
    int rank, size;
    unimpi.comm_rank(UNIMPI_COMM_WORLD, &rank);
    unimpi.comm_size(UNIMPI_COMM_WORLD, &size);
    
    printf("Hello from rank %d of %d\n", rank, size);
    
    unimpi.finalize();
    return 0;
}
```

**Compile:**

```cmd
cl /EHsc /I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" ^
   /I"C:\path\to\unimpi\include" ^
   hello.c "C:\path\to\unimpi\build\Release\unimpi.lib"
```

**Run:**

```cmd
set UNIMPI_BACKEND=msmpi
mpiexec -n 4 hello.exe
```

---

## Integration with Visual Studio

### Step 1: Add unimpi to Project

1. Right-click project → Properties
2. C/C++ → General → Additional Include Directories:
   ```
   C:\path\to\unimpi\include
   C:\Program Files (x86)\Microsoft SDKs\MPI\Include
   ```

3. Linker → General → Additional Library Directories:
   ```
   C:\path\to\unimpi\build\Release
   C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64
   ```

4. Linker → Input → Additional Dependencies:
   ```
   unimpi.lib
   msmpi.lib
   ```

### Step 2: Set Environment for Debugging

1. Project → Properties → Debugging → Environment:
   ```
   PATH=C:\Program Files\Microsoft MPI\Bin;%PATH%
   UNIMPI_BACKEND=msmpi
   ```

---

## MS-MPI Specific Features

### Windows-specific Capabilities

1. **.NET Integration**
   ```csharp
   // Use unimpi from C# via P/Invoke
   [DllImport("unimpi.dll")]
   static extern int unimpi_init(IntPtr argc, IntPtr argv);
   ```

2. **Azure Batch Support**
   ```cmd
   :: Run on Azure Batch
   mpiexec -np 4 -env UNIMPI_BACKEND msmpi myapp.exe
   ```

3. **WSL Integration**
   ```bash
   # From WSL, use Windows MS-MPI
   export UNIMPI_LIBRARY="/mnt/c/Program Files/Microsoft MPI/Bin/msmpi.dll"
   ./myapp
   ```

---

## Troubleshooting

### "Cannot find msmpi.dll"

```cmd
:: Add to PATH
set PATH=C:\Program Files\Microsoft MPI\Bin;%PATH%

:: Or copy to executable directory
copy "C:\Program Files\Microsoft MPI\Bin\msmpi.dll" .
```

### "Cannot open include file: 'mpi.h'"

```cmd
:: Verify MSMPI_INC
set MSMPI_INC=C:\Program Files (x86)\Microsoft SDKs\MPI\Include

:: Check file exists
dir "%MSMPI_INC%\mpi.h"
```

### "Unresolved external symbol"

```cmd
:: Ensure linking against msmpi.lib
:: Check library architecture matches (x64 vs x86)
dumpbin /headers "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\msmpi.lib"
```

### CMake cannot find MPI

```cmd
:: Set explicitly
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
    -DMPI_C_INCLUDE_DIRS="C:/Program Files (x86)/Microsoft SDKs/MPI/Include" ^
    -DMPI_C_LIBRARIES="C:/Program Files (x86)/Microsoft SDKs/MPI/Lib/x64/msmpi.lib"
```

### Tests fail with "No backend found"

```cmd
:: Ensure MS-MPI runtime is installed
:: Download from: https://www.microsoft.com/download/details.aspx?id=57467

:: Set backend explicitly
set UNIMPI_BACKEND=msmpi
set UNIMPI_LIBRARY=C:\Program Files\Microsoft MPI\Bin\msmpi.dll

:: Run tests
cd build
ctest -C Release
```

---

## MS-MPI vs Linux MPI

### Compatibility

| Feature | MS-MPI | Linux MPI |
|---------|--------|-----------|
| API | MPI-3 | MPI-3 |
| Handles | Integer | Integer (MPICH-based) |
| Library | `msmpi.dll` | `libmpi.so` |
| Spawn | ✅ | ✅ |
| RMA | ✅ | ✅ |
| Threads | ✅ | ✅ |

### Portability Tips

1. **Path separators:**
   ```c
   // Use forward slashes or escape backslashes
   #ifdef _WIN32
       const char *file = "C:/data/file.dat";
   #else
       const char *file = "/home/user/file.dat";
   #endif
   ```

2. **Compiler directives:**
   ```c
   #ifdef _WIN32
       // Windows-specific code
   #else
       // Linux/macOS code
   #endif
   ```

3. **Process spawning:**
   ```c
   // MS-MPI requires executable with full path on Windows
   char *command = "C:\\path\\to\\slave.exe";
   unimpi.comm_spawn(command, ...);
   ```

---

## See Also

- [BUILDING.md](BUILDING.md) - General build instructions
- [BACKENDS.md](BACKENDS.md) - Backend comparison
- [MS-MPI Documentation](https://docs.microsoft.com/en-us/message-passing-interface/)
