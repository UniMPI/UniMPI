if(NOT DEFINED UNIMPI_SOURCE_DIR)
    message(FATAL_ERROR "UNIMPI_SOURCE_DIR is required")
endif()

file(READ "${UNIMPI_SOURCE_DIR}/include/unimpi_std_macros.h" macro_header)
string(REGEX MATCHALL
    "#define[ \t]+MPI_[A-Za-z0-9_]+[ \t]+unimpi\\.[A-Za-z0-9_]+"
    macro_mappings
    "${macro_header}"
)

set(public_fields)
foreach(mapping IN LISTS macro_mappings)
    string(REGEX REPLACE
        ".*unimpi\\.([A-Za-z0-9_]+).*"
        "\\1"
        field
        "${mapping}"
    )
    list(APPEND public_fields "${field}")
endforeach()
list(REMOVE_DUPLICATES public_fields)
list(SORT public_fields)

list(LENGTH public_fields public_field_count)
if(public_field_count LESS 240)
    message(FATAL_ERROR
        "Expected at least 240 public MPI macro mappings, found ${public_field_count}"
    )
endif()

set(backends openmpi mpich intelmpi msmpi)
foreach(backend IN LISTS backends)
    file(READ
        "${UNIMPI_SOURCE_DIR}/src/backends/${backend}.c"
        backend_source
    )
    string(REGEX MATCHALL
        "unimpi\\.[A-Za-z0-9_]+[ \t]*="
        assignments
        "${backend_source}"
    )

    set(assigned_fields)
    foreach(assignment IN LISTS assignments)
        string(REGEX REPLACE
            "unimpi\\.([A-Za-z0-9_]+).*"
            "\\1"
            field
            "${assignment}"
        )
        list(APPEND assigned_fields "${field}")
    endforeach()
    list(REMOVE_DUPLICATES assigned_fields)

    set(missing_fields)
    foreach(field IN LISTS public_fields)
        if(NOT field IN_LIST assigned_fields)
            list(APPEND missing_fields "${field}")
        endif()
    endforeach()

    if(missing_fields)
        list(JOIN missing_fields ", " missing_text)
        message(FATAL_ERROR
            "${backend} does not assign public vtable fields: ${missing_text}"
        )
    endif()

    list(LENGTH assigned_fields assigned_count)
    message(STATUS
        "${backend}: ${public_field_count} public mappings covered by "
        "${assigned_count} vtable assignments"
    )
endforeach()

message(STATUS
    "All ${public_field_count} direct MPI macro mappings are assigned by every backend"
)
