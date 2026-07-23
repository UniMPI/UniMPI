# Windows and MS-MPI

MS-MPI is UniMPI's supported Windows backend. Use a native Windows executable;
a Linux or WSL process cannot load `msmpi.dll`.

## Install

Install both Microsoft packages:

- MS-MPI Runtime — provides `mpiexec.exe` and `msmpi.dll`;
- MS-MPI SDK — provides development metadata used by CMake MPI discovery.

Typical paths:

```text
C:\Program Files\Microsoft MPI\Bin\mpiexec.exe
C:\Windows\System32\msmpi.dll
C:\Program Files (x86)\Microsoft SDKs\MPI\Include
C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64
```

Verify the runtime in PowerShell:

```powershell
$launcher = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"
$library = "$env:WINDIR\System32\msmpi.dll"

if (!(Test-Path $launcher)) { throw "MS-MPI launcher not found" }
if (!(Test-Path $library)) { throw "MS-MPI runtime DLL not found" }
& $launcher -help
```

## Build

```powershell
$launcher = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DBUILD_SHARED_LIBS=OFF `
  -DUNIMPI_BUILD_EXAMPLES=ON `
  -DUNIMPI_BUILD_TESTS=ON `
  -DUNIMPI_BUILD_MPI_TESTS=ON `
  -DUNIMPI_BUILD_BENCHMARKS=ON `
  "-DMPIEXEC_EXECUTABLE=$launcher"

cmake --build build --config Release --parallel
```

`UNIMPI_BACKEND` is a runtime environment variable, not a CMake option.

## Run tests

```powershell
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"

ctest --test-dir build -C Release -L unit `
  --output-on-failure --timeout 180

ctest --test-dir build -C Release -L integration `
  --output-on-failure --timeout 180
```

CTest receives the launcher from `MPIEXEC_EXECUTABLE`. `UNIMPI_LIBRARY`
selects the DLL loaded inside each launched process.

## Run examples

```powershell
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"
$MPIEXEC = "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe"

& $MPIEXEC -np 1 .\build\examples\Release\minimal.exe
& $MPIEXEC -np 1 .\build\examples\Release\backend_info.exe
& $MPIEXEC -np 1 .\build\examples\Release\thread_init.exe
& $MPIEXEC -np 2 .\build\examples\Release\nonblocking.exe
& $MPIEXEC -np 4 .\build\examples\Release\collective.exe
& $MPIEXEC -np 2 .\build\examples\Release\rma.exe
```

See [`examples/README.md`](../examples/README.md) for feature and rank
requirements.

## Selection

Recommended:

```powershell
$env:UNIMPI_LIBRARY = "$env:WINDIR\System32\msmpi.dll"
```

Name-based fallback:

```powershell
Remove-Item Env:UNIMPI_LIBRARY -ErrorAction SilentlyContinue
$env:UNIMPI_BACKEND = "msmpi"
```

If both are set, `UNIMPI_LIBRARY` wins. The Program Files `Bin` directory
contains the launcher; the runtime DLL used by UniMPI is normally in
`System32`.

## Architecture

Build the application, UniMPI, and MS-MPI for the same architecture. The
maintained CI configuration uses MSVC x64:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

Do not mix a 32-bit executable with the 64-bit runtime.

## Troubleshooting

### `msmpi.dll` cannot be loaded

```powershell
Test-Path "$env:WINDIR\System32\msmpi.dll"
Get-Item "$env:WINDIR\System32\msmpi.dll" |
  Select-Object FullName, Length, VersionInfo
```

Check application dependencies with a Visual Studio developer shell:

```cmd
dumpbin /DEPENDENTS my_program.exe
```

### `mpiexec.exe` is not found

Use the full launcher path:

```powershell
& "$env:ProgramFiles\Microsoft MPI\Bin\mpiexec.exe" -np 2 .\my_program.exe
```

### CMake cannot find MPI for integration tests

Confirm the SDK paths exist and configure from an x64 Visual Studio developer
environment. You can still build the UniMPI library and fake/unit tests with:

```powershell
cmake -S . -B build-unit -G "Visual Studio 17 2022" -A x64 `
  -DUNIMPI_BUILD_TESTS=ON `
  -DUNIMPI_BUILD_MPI_TESTS=OFF
```

### Tests hang

- verify every process uses the same `UNIMPI_LIBRARY`;
- verify the launcher is MS-MPI, not an executable from WSL or another MPI;
- rerun one test with `ctest -V -R <name>`;
- keep a finite CTest timeout.

## Verification boundary

Windows CI exercises the MS-MPI loader, lifecycle, and focused MPI integration
suite. It does not establish complete MPI conformance or every vtable entry.
See [SUPPORT_MATRIX.md](SUPPORT_MATRIX.md).
