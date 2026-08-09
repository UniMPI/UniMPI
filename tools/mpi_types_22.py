#!/usr/bin/env python3
"""Authoritative MPI predefined type lists per version.

Handle types and predefined datatypes sourced from the standard's
C-binding datatype tables (Table 3.x). Some types appear only from a
specific MPI version (e.g. MPI_Count / MPI_Message are MPI-3.0+).
"""
import sys

# (name, min_version)  -- 22, 30, 31, 40
TYPES = [
    # Handle / opaque types
    ("MPI_Comm", 22), ("MPI_Datatype", 22), ("MPI_Op", 22),
    ("MPI_Group", 22), ("MPI_Request", 22), ("MPI_Info", 22),
    ("MPI_Win", 22), ("MPI_File", 22), ("MPI_Errhandler", 22),
    ("MPI_Status", 22), ("MPI_Aint", 22), ("MPI_Offset", 22),
    ("MPI_Message", 30), ("MPI_Count", 30),
    # Predefined C datatypes (Table 3.2)
    ("MPI_CHAR", 22), ("MPI_SHORT", 22), ("MPI_INT", 22),
    ("MPI_LONG", 22), ("MPI_LONG_LONG_INT", 22), ("MPI_LONG_LONG", 22),
    ("MPI_SIGNED_CHAR", 22), ("MPI_UNSIGNED_CHAR", 22),
    ("MPI_UNSIGNED_SHORT", 22), ("MPI_UNSIGNED", 22),
    ("MPI_UNSIGNED_LONG", 22), ("MPI_UNSIGNED_LONG_LONG", 22),
    ("MPI_FLOAT", 22), ("MPI_DOUBLE", 22), ("MPI_LONG_DOUBLE", 22),
    ("MPI_WCHAR", 22), ("MPI_C_BOOL", 22),
    ("MPI_INT8_T", 22), ("MPI_INT16_T", 22), ("MPI_INT32_T", 22),
    ("MPI_INT64_T", 22), ("MPI_UINT8_T", 22), ("MPI_UINT16_T", 22),
    ("MPI_UINT32_T", 22), ("MPI_UINT64_T", 22),
    ("MPI_C_COMPLEX", 22), ("MPI_C_FLOAT_COMPLEX", 22),
    ("MPI_C_DOUBLE_COMPLEX", 22), ("MPI_C_LONG_DOUBLE_COMPLEX", 22),
    ("MPI_BYTE", 22), ("MPI_PACKED", 22),
    # Compound / reduction datatypes
    ("MPI_FLOAT_INT", 22), ("MPI_DOUBLE_INT", 22), ("MPI_LONG_INT", 22),
    ("MPI_SHORT_INT", 22), ("MPI_2INT", 22), ("MPI_LONG_DOUBLE_INT", 22),
    ("MPI_LONG_LONG_INT", 22), ("MPI_COMPLEX", 22),
    ("MPI_DOUBLE_COMPLEX", 22), ("MPI_LONG_DOUBLE_COMPLEX", 22),
    ("MPI_2DOUBLE_PRECISION", 22), ("MPI_2REAL", 22),
    ("MPI_2COMPLEX", 22), ("MPI_2DOUBLE_COMPLEX", 22), ("MPI_2FLOAT", 22),
    # Marker/special datatypes
    ("MPI_UB", 22), ("MPI_LB", 22), ("MPI_AINT", 22), ("MPI_OFFSET", 22),
]

def main():
    ver = int(sys.argv[1])
    names = sorted({n for n, v in TYPES if v <= ver})
    for n in names:
        print(n)

if __name__ == "__main__":
    main()
