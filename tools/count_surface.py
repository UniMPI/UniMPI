#!/usr/bin/env python3
"""Count the UniMPI public API surface without building.

This is the reproducible source of truth for the inventory figures quoted in
the docs (README, SUPPORT_MATRIX, API, design, ...). It is deliberately a
cheap static pass -- no preprocessing, no build -- so any target/commit the
numbers drift from can be caught by re-running it.

Two notices apply to every number it prints:

  1. VTABLE fields are compile-time conditioned. The main vtable
     (unimpi_vtable.h) drops MPI-3.0 fields behind `#if
     UNIMPI_MPI_AT_LEAST(3,0)`, so a target-2.2 build exports fewer fields
     than the language-level total reported here. The value a given build
     actually carries is `UNIMPI_VTABLE_COUNT` (== sizeof(unimpi_vtable_t) /
     sizeof(void*)), e.g. 364 at target 3.0 / 297-299 at target 2.2, and is
     also printed by tests/internal/test_vtable_layout.
  2. Direct standard-name aliases are `#define MPI_* -> unimpi.*` (or
     `unimpi_mt.t_*`) macros in unimpi_std_macros.h. Two aliases
     (MPI_File_get/set_errhandler) are conditionally defined twice, so the
     unique-name count is 2 fewer than the raw #define count.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VTABLE_H = os.path.join(ROOT, "include", "unimpi_vtable.h")
STD_MACROS_H = os.path.join(ROOT, "include", "unimpi_std_macros.h")


def count_vtable_fields(path):
    """Function-pointer members of unimpi_vtable_t (excludes typedef decls)."""
    names = []
    for line in open(path, encoding="utf-8"):
        s = line.strip()
        if s.startswith("typedef"):
            continue
        m = re.match(r"(?:const\s+)?[\w\s\*]+\(\*(\w+)\)\s*\(", s)
        if m:
            names.append(m.group(1))
    return names


def count_direct_aliases(path):
    """#define MPI_* -> unimpi.* / unimpi_mt.t_* macros, by unique name."""
    lines = []
    for line in open(path, encoding="utf-8"):
        m = re.match(r"#define\s+(MPI_\w+)\s+unimpi(_mt)?\.(\w+)", line.strip())
        if m:
            lines.append(m.group(1))
    return lines


def main():
    vfields = count_vtable_fields(VTABLE_H)
    aliases = count_direct_aliases(STD_MACROS_H)
    fields = len(vfields)
    raw = len(aliases)
    unique = len(set(aliases))
    ptr = 8 if sys.maxsize > 2**32 else 4

    print("main vtable (language-level field count): %d" % fields)
    print("  vtable size on %d-bit: %d" % (ptr * 8, fields * ptr))
    print("direct standard-name aliases (unique): %d" % unique)
    print("direct standard-name aliases (raw #define count): %d (incl. 2 "
          "conditional dup: MPI_File_get/set_errhandler)" % raw)
    print("note: %d fields is the target-3.0 (MPI_AT_LEAST(3,0)=1) total; a "
          "target-2.2 build carries fewer (run test_vtable_layout for the "
          "exact value)." % fields)


if __name__ == "__main__":
    main()
