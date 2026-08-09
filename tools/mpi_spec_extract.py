#!/usr/bin/env python3
"""Extract public MPI function names from an MPI standard report text.

A public C-binding function is a line like `int MPI_Name(...)`. We drop
per-process internal '_c' variants and known function-pointer *types* (e.g.
MPI_User_function), which are callback signatures, not callable functions.
"""
import re, sys
RET_LINE = re.compile(
    r'(?:^|[\s,;])(?:int|void|double|float|unsigned char|unsigned long|char|MPI_Count|MPI_Aint|size_t)\s+'
    r'(MPI_[A-Za-z0-9_]+)\s*\(')
# Callback / function-pointer types are NOT functions. They end in _function,
# _fn, _fn_null, _c_function, or are specific known typedef names.
TYPE_ONLY = re.compile(
    r'MPI_(.*_function|.*_cb_function|.*_c_function|User_function|Copy_function|'
    r'Delete_function|Handler_function|Datarep_conversion_function|'
    r'Datarep_extent_function|Grequest_cancel_function|Grequest_free_function|'
    r'Grequest_query_function|Comm_copy_attr_function|Comm_delete_attr_function|'
    r'Type_copy_attr_function|Type_delete_attr_function|Win_copy_attr_function|'
    r'Win_delete_attr_function|Errhandler_function|Session_errhandler_function'
    r'|T_event_cb_function|T_event_dropped_cb_function|T_event_free_cb_function'
    r'|CONVERSION_FN_NULL|CONVERSION_FN_NULL_C|op_free|Handler_function)\b')
def main():
    out = set()
    path = sys.argv[1]
    for ln in open(path, errors='ignore'):
        stripped = re.sub(r'^\s{0,4}\d{1,5}\s{1,3}', '', ln)
        m = RET_LINE.search(stripped)
        if not m:
            continue
        name = m.group(1)
        if name.endswith('_c'):
            name = name[:-2]
        if TYPE_ONLY.search(name):
            continue
        out.add(name)
    for n in sorted(out):
        print(n)
if __name__ == '__main__':
    main()
