#!/usr/bin/env python3
"""Self-test for mpi_version_gate.py.

Feeds deliberately-wrong versioned_clusters.csv inputs to `check` and asserts it
exits non-zero; also asserts a clean fixture passes. Runs in a temp dir, stdlib only.

TDD note: before tools/mpi_version_gate.py exists, invoking it returns non-zero
for every case, so the clean-fails assertions are the ones that catch the RED
state (a truly passing test requires the clean fixture to exit 0, which only
happens after the checker is implemented).
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, "mpi_version_gate.py")
REAL_CLUSTERS = os.path.join(HERE, "versioned_clusters.csv")

failures = []


def expect(cond, msg):
    if not cond:
        failures.append(msg)
        print("FAIL:", msg)


def run_check(clusters_csv, *extra):
    """Run `check` against the given clusters csv, return returncode."""
    return subprocess.run(
        [sys.executable, SCRIPT, "check", "--clusters", clusters_csv] + list(extra),
        capture_output=True,
        text=True,
    ).returncode


def main():
    d = tempfile.mkdtemp(prefix="unimpi-gate-test-")

    # --- Case 1: clean fixture (the real csv) must pass --------------------
    rc = run_check(REAL_CLUSTERS)
    expect(rc == 0, "clean fixture should exit 0 (got %d): run on real csv" % rc)

    # --- Case 2: cluster name not in the registry must fail ---------------
    bad1 = os.path.join(d, "bad-unknown.csv")
    with open(bad1, "w") as fh:
        fh.write("文件,簇名,生效主版,生效次版\n"
                 "include/unimpi_vtable.h,nonblock,3,0\n"
                 "include/unimpi_vtable.h,fake,0,0\n")
    rc = run_check(bad1)
    expect(rc != 0, "unknown cluster name must make check fail (got %d)" % rc)

    # --- Case 3: valid cluster but wrong declared version must fail --------
    bad2 = os.path.join(d, "bad-version.csv")
    with open(bad2, "w") as fh:
        fh.write("文件,簇名,生效主版,生效次版\n"
                 "include/unimpi_vtable.h,alltoallw,2,1\n")
    rc = run_check(bad2)
    expect(rc != 0, "wrong cluster version must make check fail (got %d)" % rc)

    # --- Case 4: a member missing from the protected span must fail --------
    guard_partial = os.path.join(d, "guard_partial.c")
    with open(guard_partial, "w") as fh:
        fh.write("#if UNIMPI_MPI_AT_LEAST(3,0)\n"
                 "/* MPI-3.0 matched_probe */\n"
                 "int (*mprobe)(void);\n"     # guarded
                 "int (*improbe)(void);\n"    # guarded
                 "int (*mrecv)(void);\n"      # guarded
                 "#endif\n"
                 "int (*imrecv)(void);\n")    # member left OUTSIDE the guard
    gc_partial = os.path.join(d, "guard_partial.csv")
    with open(gc_partial, "w") as fh:
        fh.write("文件,簇名,生效主版,生效次版\n"
                 "%s,matched_probe,3,0\n" % guard_partial)
    rc = run_check(gc_partial, "--require-guards")
    expect(rc != 0, "a member outside the protected span must fail (got %d)" % rc)

    # --- Case 5: guard present must pass (with --require-guards) -----------
    guard_ok = os.path.join(d, "guard_ok.c")
    with open(guard_ok, "w") as fh:
        fh.write("int unrelated;\n"
                 "#if UNIMPI_MPI_AT_LEAST(3,0)\n"
                 "/* MPI-3.0 alltoallw */\n"
                 "int (*alltoallw)(const void *a);\n"
                 "#endif\n")
    gc_ok = os.path.join(d, "guard_ok.csv")
    with open(gc_ok, "w") as fh:
        fh.write("文件,簇名,生效主版,生效次版\n"
                 "%s,alltoallw,3,0\n" % guard_ok)
    rc = run_check(gc_ok, "--require-guards")
    expect(rc == 0, "a present, correctly-versioned guard must pass (got %d)" % rc)

    # --- Case 6: guard missing / wrong version must fail -------------------
    guard_bad = os.path.join(d, "guard_bad.c")
    with open(guard_bad, "w") as fh:
        fh.write("int unrelated;\n"
                 "int (*alltoallw)(const void *a);\n")  # no guard at all
    gc_bad = os.path.join(d, "guard_bad.csv")
    with open(gc_bad, "w") as fh:
        fh.write("文件,簇名,生效主版,生效次版\n"
                 "%s,alltoallw,3,0\n" % guard_bad)
    rc = run_check(gc_bad, "--require-guards")
    expect(rc != 0, "a missing guard must make check fail (got %d)" % rc)

    # --- Case 7: guard with wrong version must fail ------------------------
    guard_wrong = os.path.join(d, "guard_wrong.c")
    with open(guard_wrong, "w") as fh:
        fh.write("#if UNIMPI_MPI_AT_LEAST(2,1)\n"
                 "/* MPI-2.1 alltoallw */\n"
                 "int (*alltoallw)(const void *a);\n"
                 "#endif\n")
    gc_wrong = os.path.join(d, "guard_wrong.csv")
    with open(gc_wrong, "w") as fh:
        fh.write("文件,簇名,生效主版,生效次版\n"
                 "%s,alltoallw,3,0\n" % guard_wrong)
    rc = run_check(gc_wrong, "--require-guards")
    expect(rc != 0, "a guard with the wrong version must fail (got %d)" % rc)

    if failures:
        print("gate-check-FAIL (%d problems)" % len(failures))
        sys.exit(1)
    print("gate-check-ok")


if __name__ == "__main__":
    main()
