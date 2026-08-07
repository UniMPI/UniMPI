# MPI-2.2 持续改进循环任务 (Loop)

> **For agentic workers:** This is a recurring improvement loop. Each iteration
> picks the next uncovered category, drives it through the project's full
> verification close-the-loop (vtable → backends → tests → docs), self-checks,
> then advances the queue. Use `superpowers:subagent-driven-development` or
> `superpowers:executing-plans` to implement each iteration task-by-task.

**Goal:** Continuously expand the verified MPI-2.2 surface, one category per
iteration, keeping every change behind a passing cross-backend test before the
support matrix is updated.

**Authority / scope:** Only the category named at the top of the current
iteration may be changed. No opportunistic refactors outside it. Commit each
iteration separately with a conventional-commit message.

---

## Close-the-Loop Checklist (run every iteration)

Follow this exact order. `SUPPORT_MATRIX.md` is updated **last**, and only after
the new tests actually pass.

- [ ] 1. **Pick category** — take the first not-yet-done, non-blocked item from
      the Priority Queue below. Record it as the iteration's scope.
- [ ] 2. **Vtable + alias** — add any missing typed field to
      `include/unimpi_vtable.h` and the matching `MPI_*` alias in
      `include/unimpi_std_macros.h` (4-space indent, project naming rules).
- [ ] 3. **Backend resolution** — resolve the symbol in every applicable backend
      adapter (`openmpi.c`, `mpich.c`, `intelmpi.c`, `msmpi.c`), respecting
      platform limits (e.g. MS-MPI capability subset).
- [ ] 4. **required vs optional** — classify each new entry required or optional
      for the supported profile; required entries must be present or the
      backend misses validation.
- [ ] 5. **Fake/unit coverage** — add fake-backend or unit coverage where loading
      or state changes (loader, lifecycle, vtable validation, memory safety).
- [ ] 6. **Real-backend test** — add a focused `tests/mpi/*` integration test
      with the needed process count, wired through `tests/CMakeLists.txt`.
- [ ] 7. **Self-check** — build unit + one real backend locally; `ctest -L unit`
      and `ctest -L integration` both green. Fix failures before proceeding.
- [ ] 8. **Docs** — update `SUPPORT_MATRIX.md` (and `MPI_SUPPORT_ANALYSIS.md` if
      the claim changes) only now, deleting the category from the uncovered list.
- [ ] 9. **Commit** — conventional message, e.g. `feat: add MPI-2.2 <category>`
      or `fix:`/`test:` as appropriate. Push only when asked.
- [ ] 10. **Advance** — mark the category done; the next loop iteration opens.

---

## Priority Queue (MPI-2.2 focused, ordered by value & dependency)

Order matters: earlier items unblock later ones (e.g. the status-stride adapter
enables per-element status results). Do not skip ahead past a blocker.

1. **Portable `MPI_Status` field access** — native/facade stride adapter for
   source/tag/error across status layouts. *Unlocks* per-element status results.
2. **Per-element status results from request arrays** — `Waitall`/`Waitany`/
   `Testall` status-array correctness once item 1 lands.
3. **`Alltoallw` typed datatype-array adapter** — integer-handle backends
   (MPICH/Intel/MS-MPI) complete the nonblocking-collective story.
4. **Persistent collective requests** — `.begin/.end` lifecycle + broader
   persistent P2P lifecycle and error cases.
5. **Matched probe / ready-buffered sends / cancellation** — `MPI_Mprobe`,
   `MPI_Mrecv`, `MPI_Cancel`, cancelled-status semantics.
6. **Advanced datatype constructors** — `MPI_Type_hindexed`,
   `MPI_Type_subarray`, `MPI_Type_darray`, external32 packing.
7. **RMA other epoch models** — lock/unlock, flush, PSCW, shared & dynamic
   windows (beyond the fence-based subset).
8. **MPI I/O views & pointers** — file views, shared/ordered pointers, seek/
   position, split collectives, preallocation.
9. **Distributed-graph topology** — `MPI_Dist_graph_create*`.
10. **Custom ops / error handlers / attribute corners** — user reductions,
    custom error handlers, attribute edge cases.
11. **Threading & resilience (stretch)** — `MPI_THREAD_MULTIPLE` stress and
    concurrent lifecycle (documented, highest-risk; do last).

Items already claimed in `SUPPORT_MATRIX.md` are skipped. The queue is a living
list: split or reorder a category if it proves too large for one clean iteration.

---

## Iteration Card

```md
# Iteration N: <category name>
- [ ] Close-the-loop steps 1–10
- [ ] New vtable fields: <list>
- [ ] New standard aliases: <list>
- [ ] New tests: <tests/mpi/*.c>
- [ ] SUPPORT_MATRIX row updated: <category>
```

Copy this card to the top of the working checkpoint (e.g. a git branch name
`loop/mpi22-<category>`) and fill it in as you go.

---

## Health / definition of "done"

An iteration is done only when all of:

- unit + integration tests pass (self-checked), not just "committed";
- the support matrix row is updated and the category removed from the uncovered
  list;
- the change is isolated to the named category and committed conventionally.

If a category is blocked (e.g. a backend cannot export the symbol), record the
block in the iteration card, keep the entry in the uncovered list with a note,
mark the category "blocked" in the queue, and move to the next unblocked item.
