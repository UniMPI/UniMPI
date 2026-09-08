# Performance

UniMPI is designed so that dispatching through the library is close to free in
steady state. This page explains *why* the architecture has that property,
what a measurement does and does not prove, and why UniMPI's cost model differs
from a generic "abstraction layer". For how to run the shipped benchmarks
yourself, see [BENCHMARKS.md](BENCHMARKS.md).

## How dispatch is structured

Three design choices keep the hot path minimal.

1. **The public API is a macro layer, not a wrapper library.**

   `include/unimpi_std_macros.h` defines the standard names as plain text
   substitution:

   ```c
   #define MPI_Send unimpi.send
   ```

   (367 direct standard-name aliases — see `tools/count_surface.py`.)
   `MPI_Send(buf, ...)` is never a call into a UniMPI
   wrapper function — the macro expands the call site directly into a member
   access on the global dispatch table `unimpi`, followed by an indirect call.
   There is no extra stack frame and no argument shuffling.

2. **Symbol resolution happens once, at initialization.**

   During `MPI_Init` the loader opens the selected backend
   (`unimpi_loader_load`) and each backend fills the vtable with one `dlsym`
   per routine (`unimpi_vtable_init` in `src/vtable.c`; per-backend fill in
   `src/backends/*.c`). None of that runs on the hot path: steady-state calls
   do no string comparison, no hash lookup, and no dynamic-linker
   intervention.

3. **The dispatch table is a global, fixed-after-init struct.**

   In steady state a call compiles to roughly:

   ```asm
   mov rax, [rip + unimpi + <offset>]   ; load function pointer
   call rax                             ; indirect call
   ```

   The table is global and populated once, so the branch-target buffer learns
   the target on the first call and predicts subsequent calls accurately.

## Why this is not a "slow abstraction layer"

- **No wrapper function.** Macro expansion produces no intermediate function,
  so there is no extra frame and no extra jump beyond the raw call.
- **No repeated symbol lookup.** `dlsym` cost, where it exists at all, is paid
  once at init.
- **No extra indirection tier versus native MPI.** A dynamically linked native
  MPI library is itself reached through the PLT (a GOT-indirect jump). UniMPI
  adds a single `mov` that loads the address from a global instead of from the
  GOT — it does not add a jump level. Realistic dispatch overhead is on the
  order of a few cycles (sub-nanosecond), well within the noise of any real
  MPI operation.

## The honest cost model

- Dispatch cost is a fixed per-call addition; it does not grow with message
  size, datatype complexity, or communicator size.
- For every operation whose backend work is non-trivial — buffering, matching,
  copying, or any network or shared-memory transfer — UniMPI's dispatch cost is
  a rounding error.
- Where the dispatch could matter is a hot loop of tiny *local* operations on a
  single rank, e.g. `MPI_Comm_rank` or `MPI_Wtime`, with no competing branch
  pressure. `bench_overhead` measures exactly this: a paired direct-symbol
  vs. vtable delta on your hardware, so the number comes from your machine, not
  from a claim here.
- The benchmark tools are deliberately honest about their limits: they measure
  end-to-end operations and "must not be described as a proven nanosecond-level
  wrapper overhead" (see [BENCHMARKS.md](BENCHMARKS.md) — run a native baseline
  under identical conditions before claiming overhead).

## When UniMPI is worth it

UniMPI trades per-MPI recompilation for the freedom to relink to any supported
backend without recompiling ("compile once, run anywhere"). Because the
dispatch cost is negligible relative to real MPI work, that freedom is
effectively free for application code. The public API *is* the standard MPI
API, so switching back to native MPI at any time remains possible — UniMPI
imposes no lock-in.

## Limits and future work

- The call is still an indirect call. In scenarios with extreme I-cache or BTB
  pressure happening at the same time as sub-nanosecond tolerance, profile
  before concluding anything.
- Techniques that remove even the indirect call — freestanding `ifunc`
  resolvers, forced inlining, link-time rewriting — are deliberately not used,
  because UniMPI's purpose is a single library that dispatches to one of
  several backends chosen at runtime, not a per-backend specialized build.
