#!/usr/bin/env python3
"""MPI-version gating checker for UniMPI.

The *version truth* lives in two plain-text CSVs:
  1. api_versions.csv        -- every versioned entity and its introduction
                                major.minor version (the "version truth").
  2. versioned_clusters.csv  -- which clusters appear in which files, and the
                                version each cluster is gated to in that file.

This tool proves the two tables do not drift and (optionally) that the `#if`
guards later tasks add to the C sources still match.

The canonical cluster -> members mapping is embedded in REGISTRY below. Member
names are the bare field/macro names WITHOUT the `MPI_` prefix, exactly as they
appear in unimpi_vtable.h.

Subcommands / flags:
  check                  Data consistency only (default): every cluster's
                         declared version must equal max(member version) from
                         api_versions.csv, and every entity in api_versions.csv
                         must belong to exactly one cluster.
  check --require-guards Additionally verify that in each listed file a
                         contiguous span wraps the cluster's members in
                         `#if UNIMPI_MPI_AT_LEAST(<maj>,<min>)` .. `#endif`
                         carrying a matching `/* MPI-<maj>.<min> <cluster> */`
                         comment. Fails cleanly (non-zero) when a guard is
                         missing.

This tool operates purely on aligned plain text -- it does NOT parse a C AST.
Members are located by their bare name in the file text. If a member cannot be
located, it errors out loudly; it never guesses.

The `generate` subcommand from the plan is deliberately deferred (out of scope
for this task) -- `check` is the required deliverable.

Dependency-free (Python 3 stdlib only).
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(HERE)

# ---------------------------------------------------------------------------
# Canonical cluster -> members registry (V3_FUNCS grouping, all MPI-3.0).
# Member names are bare field/macro names WITHOUT the MPI_ prefix.
# ---------------------------------------------------------------------------
REGISTRY = {
    "matched_probe": [
        "mprobe", "improbe", "mrecv", "imrecv",
    ],
    "nonblocking_collectives": [
        "ibarrier", "ibcast", "igather", "igatherv", "iscatter", "iscatterv",
        "iallgather", "iallgatherv", "ialltoall", "ialltoallv", "ialltoallw",
        "ireduce", "iallreduce", "ireduce_scatter", "ireduce_scatter_block",
        "iscan", "iexscan",
    ],
    "comm_3x": [
        "comm_dup_with_info", "comm_split_type", "comm_create_group",
        "comm_get_info", "comm_set_info",
    ],
    "win_alloc_shared": ["win_allocate_shared", "win_create_dynamic"],
    "rma_atomics": [
        "get_accumulate", "fetch_and_op", "compare_and_swap", "rput", "rget",
        "raccumulate", "rget_accumulate",
    ],
    "rma_sync_3x": [
        "win_lock_all", "win_unlock_all", "win_flush", "win_flush_all",
        "win_flush_local", "win_sync",
    ],
}

FAILURES = []


def fail(msg):
    FAILURES.append(msg)
    print("ERROR: %s" % msg)


def parse_csv(path, ncols):
    """Read a CSV with a header row, returning a list of row tuples (strings)."""
    rows = []
    with open(path, "r", encoding="utf-8-sig") as fh:
        for lineno, line in enumerate(fh, start=1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = [p.strip() for p in line.split(",")]
            if len(parts) != ncols:
                fail("%s:%d: expected %d columns, got %d" % (path, lineno, ncols, len(parts)))
                continue
            rows.append(tuple(parts))
    return rows


def load_api_versions(api_path):
    """Return {entity_name: (maj, min)} from api_versions.csv (skips header)."""
    entities = {}
    rows = parse_csv(api_path, 3)
    for name, maj, mn in rows:
        if name == "实体":
            continue  # header
        try:
            entities[name] = (int(maj), int(mn))
        except ValueError:
            fail("%s: non-numeric version for %s (%s,%s)" % (api_path, name, maj, mn))
    return entities


def load_clusters(clusters_path):
    """Return the list of (file, cluster, maj, min) tuples (skips header)."""
    out = []
    rows = parse_csv(clusters_path, 4)
    for fname, cluster, maj, mn in rows:
        if fname == "文件":
            continue  # header
        try:
            out.append((fname, cluster, int(maj), int(mn)))
        except ValueError:
            fail("%s: non-numeric version for %s/%s (%s,%s)"
                 % (clusters_path, fname, cluster, maj, mn))
    return out


def check_cluster_versions(entities, cluster_rows):
    """Rule (a): each cluster's declared version == max(member version)."""
    for fname, cluster, maj, mn in cluster_rows:
        if cluster not in REGISTRY:
            fail("cluster '%s' (in %s) does not exist in the registry" % (cluster, fname))
            continue
        emaj, emn = 0, 0
        for member in REGISTRY[cluster]:
            if member not in entities:
                fail("registry member '%s' of cluster '%s' has no api_versions entry"
                     % (member, cluster))
                continue
            mmaj, mmn = entities[member]
            if (mmaj, mmn) > (emaj, emn):
                emaj, emn = mmaj, mmn
        if (maj, mn) != (emaj, emn):
            fail("%s: cluster '%s' declares %d.%d but members imply %d.%d "
                 "(declared version must equal the max member version)"
                 % (fname, cluster, maj, mn, emaj, emn))


def check_coverage(entities):
    """Rule (b): every entity in api_versions.csv belongs to exactly one cluster."""
    for name in entities:
        owning = [c for c, members in REGISTRY.items() if name in members]
        if len(owning) == 0:
            fail("versioned entity '%s' is not a member of any cluster in the registry" % name)
        elif len(owning) > 1:
            fail("versioned entity '%s' is a member of multiple clusters: %s"
                 % (name, ", ".join(owning)))


def resolve_file_path(fname):
    if os.path.isabs(fname):
        return fname
    return os.path.join(REPO_ROOT, fname)


def _member_line_indices(lines, member):
    """Return 0-based lines where `member` appears as a whole word (bare name)."""
    rx = re.compile(r"\b" + re.escape(member) + r"\b")
    idx = []
    for i, line in enumerate(lines):
        if rx.search(line):
            idx.append(i)
    return idx


def check_guards(cluster_rows):
    """Rule (c): each listed file wraps its cluster's members in a matching guard."""
    for fname, cluster, maj, mn in cluster_rows:
        path = resolve_file_path(fname)
        if not os.path.isfile(path):
            fail("--require-guards: file not found: %s" % path)
            continue
        with open(path, "r", encoding="utf-8-sig") as fh:
            lines = fh.readlines()

        comment_rx = re.compile(
            r"/\*\s*MPI-%d\.%d\s+%s\s*\*/" % (maj, mn, re.escape(cluster)))
        guard_rx = re.compile(r"#if\s+UNIMPI_MPI_AT_LEAST\(%d,%d\)" % (maj, mn))

        comment_lines = [i for i, ln in enumerate(lines) if comment_rx.search(ln)]
        if not comment_lines:
            fail("%s:%s: could not locate guard comment '/* MPI-%d.%d %s */'"
                 % (fname, cluster, maj, mn, cluster))
            continue
        comment_line = comment_lines[0]

        # Nearest `#if UNIMPI_MPI_AT_LEAST(maj,min)` above the comment.
        start = None
        for i in range(comment_line - 1, -1, -1):
            if guard_rx.search(lines[i]):
                start = i
                break
        # Nearest `#endif` below the comment.
        end = None
        for i in range(comment_line + 1, len(lines)):
            if re.search(r"^\s*#endif\b", lines[i]):
                end = i
                break

        if start is None:
            fail("%s:%s: '#if UNIMPI_MPI_AT_LEAST(%d,%d)' not found before guard comment"
                 % (fname, cluster, maj, mn))
            continue
        if end is None:
            fail("%s:%s: '#endif' not found after guard comment" % (fname, cluster))
            continue
        if end <= start:
            fail("%s:%s: malformed guard span (#if after #endif)" % (fname, cluster))
            continue

        for member in REGISTRY[cluster]:
            hits = _member_line_indices(lines, member)
            if not hits:
                fail("%s: member '%s' of cluster '%s' not found in file at all"
                     % (fname, member, cluster))
                continue
            inside = [ln for ln in hits if start < ln < end]
            if not inside:
                hit_line = hits[0] + 1  # 1-based, the first occurrence found
                fail("%s:%d: member '%s' of cluster '%s' is outside the guard "
                     "span (not wrapped by #if..#endif)" % (fname, hit_line, member, cluster))
    # start/end are 0-based and exclusive; members must sit strictly between them.


# ---------------------------------------------------------------------------
# 2.2-base anti-drift verification.
#
# The gated (MPI-3.0) clusters must be DISJOINT from the MPI-2.2 canonical
# function list -- otherwise a 2.2 baseline silently loses a function it is
# required to expose. This is the mechanical guard for the alltoallw /
# comm_join / op_commutative misclassification that was fixed once in Task 47.
#
# The MPI-2.2 canonical list lives in docs/MPI_VERSION_EVOLUTION.md under
# "## MPI 2.2 完整函数清单（基准）" as "- MPI_Xxx" bullets. It is the project's
# declared version truth for the 2.2 base, kept in sync manually; this check
# mechanically enforces that the gated surface never overlaps it.
# ---------------------------------------------------------------------------
DEF_EVO = os.path.join(REPO_ROOT, "docs", "MPI_VERSION_EVOLUTION.md")


def load_base_22(evo_path=DEF_EVO):
    """Return ({field_name}, count) for the MPI-2.2 canonical function list.

    field_name is the vtable member-equivalent: the doc bullet 'MPI_Comm_dup'
    becomes 'comm_dup' (MPI_ prefix stripped, lowercased).
    """
    with open(evo_path, "r", encoding="utf-8-sig") as fh:
        lines = fh.readlines()

    heading = "## MPI 2.2 完整函数清单（基准）"
    start = next((i for i, l in enumerate(lines) if l.strip() == heading), None)
    if start is None:
        fail("base: heading '%s' not found in %s" % (heading, evo_path))
        return set(), 0

    fields = set()
    for l in lines[start + 1:]:
        if re.match(r"^##\s", l.strip()):
            break  # next heading ends the 2.2 list
        m = re.match(r"^\s*-\s+MPI_(\w+)", l)
        if m:
            fields.add(m.group(1).lower())
    return fields, len(fields)


def extract_always_present_fields(vtable_path):
    """Return fields NOT wrapped in an `#if UNIMPI_MPI_AT_LEAST` guard.

    A field at AT_LEAST-depth 0 is part of the always-present base surface,
    regardless of any other (platform / feature) preprocessor guard around it.
    """
    with open(vtable_path, "r", encoding="utf-8-sig") as fh:
        lines = fh.readlines()

    field_rx = re.compile(r"\(\*\s*(\w+)\s*\)")
    # Predefined-value globals are exposed as `extern <type> *NAME;` (e.g. the
    # predefined attribute callbacks), not as vtable fields. They are part of
    # the always-present base surface too.
    extern_rx = re.compile(r"^\s*extern\s+[^;]+?\*\s*(\w+)\s*;")
    guard_rx = re.compile(r"#\s*if\s+UNIMPI_MPI_AT_LEAST")
    present = set()
    depth = 0
    for l in lines:
        if guard_rx.search(l):
            depth += 1
            continue
        if re.match(r"^\s*#endif\b", l.strip()):
            depth = max(0, depth - 1)
            continue
        if depth > 0:
            continue
        m = field_rx.search(l)
        if m:
            present.add(m.group(1))
            continue
        m2 = extern_rx.match(l)
        if m2:
            # Predefined-value globals carry the STANDARD name (MPI_COMM_DUP_FN);
            # normalize to the vtable-field convention used by the base list
            # (strip MPI_ prefix, lowercase) so comm_dup_fn matches.
            present.add(re.sub(r"^MPI_", "", m2.group(1), flags=re.I).lower())
    return present


def run_check_base(args):
    evo = os.path.abspath(args.evolution)
    if not os.path.isfile(evo):
        fail("base: evolution doc not found: %s" % evo)
        return
    base, base_count = load_base_22(evo)
    vtable_path = resolve_file_path(args.vtable)
    if not os.path.isfile(vtable_path):
        fail("base: vtable header not found: %s" % vtable_path)
        return

    present = extract_always_present_fields(vtable_path)

    # Gated = every vtable field that is at_least-present. Derive it from the
    # registry clusters (the gated set is exactly the cluster members).
    gated = set()
    for members in REGISTRY.values():
        gated.update(members)

    # (a) HARD: no gated field may be a 2.2 canonical function.
    overlap = gated & base
    if overlap:
        for f in sorted(overlap):
            fail("base: '%s' is gated as >=MPI-3.0 but is in the MPI-2.2 "
                 "canonical list -- a 2.2 baseline would silently lose it "
                 "(misclassification regression)" % f)
    else:
        print("base: gated(3.0) surface and 2.2 canonical list are disjoint OK")

    # (b) INFO: how much of the 2.2 base is exposed (always-present fields).
    covered = base & present
    missing = base - present
    print("base: 2.2 canonical functions: %d" % base_count)
    print("base: exposed in always-present vtable: %d (%.1f%%)"
          % (len(covered), 100.0 * len(covered) / base_count))
    # Fields the 2.2 list names but the vtable nowhere declares (not just
    # gated) are pre-existing base gaps -- reported, not fatal (see Task 49).
    absent = missing - gated
    if absent:
        print("base: %d 2.2 functions absent from the whole vtable (pre-existing "
              "coverage gap, not a gating regression):" % len(absent))
        for f in sorted(absent):
            print("   - %s" % f)

    if FAILURES:
        print("base check FAILED: %d problem(s)" % len(FAILURES))
        sys.exit(1)
    print("base check passed (%d/%d 2.2 functions exposed; %d absent)"
          % (len(covered), base_count, len(absent)))


def run_check(args):
    entities = load_api_versions(args.api)
    cluster_rows = load_clusters(args.clusters)

    check_cluster_versions(entities, cluster_rows)
    check_coverage(entities)

    if args.require_guards:
        check_guards(cluster_rows)

    if FAILURES:
        print("gate check FAILED: %d problem(s)" % len(FAILURES))
        sys.exit(1)
    print("gate check passed (%d clusters, %d entities)"
          % (len(set(r[1] for r in cluster_rows)), len(entities)))


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="mpi_version_gate.py",
        description="Verify MPI-version-gating truth tables and guards.")
    sub = parser.add_subparsers(dest="command")
    check = sub.add_parser("check", help="verify version-truth consistency (and guards)")
    check.add_argument("--clusters", default=os.path.join(HERE, "versioned_clusters.csv"),
                       help="path to versioned_clusters.csv")
    check.add_argument("--api", default=os.path.join(HERE, "api_versions.csv"),
                       help="path to api_versions.csv")
    check.add_argument("--require-guards", action="store_true",
                       help="also verify #if guards in the listed source files")
    check.set_defaults(func=run_check)

    base = sub.add_parser("base", help="verify the MPI-2.2 base never overlaps "
                                       "the gated surface (and report coverage)")
    base.add_argument("--evolution", default=DEF_EVO,
                      help="path to docs/MPI_VERSION_EVOLUTION.md (2.2 canonical list)")
    base.add_argument("--vtable", default=os.path.join("include", "unimpi_vtable.h"),
                      help="path to include/unimpi_vtable.h")
    base.set_defaults(func=run_check_base)

    args = parser.parse_args(argv)
    if not hasattr(args, "func"):
        parser.print_help()
        sys.exit(2)
    # argparse's --require-guards becomes args.require_guards
    args.func(args)


if __name__ == "__main__":
    main()
