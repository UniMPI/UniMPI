cmake_minimum_required(VERSION 3.10)

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
    set(backend_source_file "${UNIMPI_SOURCE_DIR}/src/backends/${backend}.c")
    if(NOT EXISTS "${backend_source_file}")
        continue()
    endif()

    file(READ "${backend_source_file}" backend_source)
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

    # Integer-handle backends bind request/message adapters through
    # unimpi_bind_integer_request_apis(), which expands field names via macros
    # rather than writing unimpi.<field> = in the backend file.
    if(backend_source MATCHES "unimpi_bind_integer_request_apis")
        set(request_bind_source_file
            "${UNIMPI_SOURCE_DIR}/src/backends/request_handle_wrappers.c")
        if(EXISTS "${request_bind_source_file}")
            file(READ "${request_bind_source_file}" request_bind_source)
            # Match only call sites: BIND_OPTIONAL(name, or BIND_ARRAY(name,
            # after stripping macro definitions so a parameter named "field"
            # or "slot" is never treated as an assigned vtable field.
            string(REGEX REPLACE
                "#[ \t]*define[ \t]+BIND_(OPTIONAL|ARRAY)\\([^\n]*\n"
                ""
                request_bind_calls
                "${request_bind_source}"
            )
            string(REGEX MATCHALL
                "BIND_(OPTIONAL|ARRAY)\\([A-Za-z0-9_]+,"
                bind_calls
                "${request_bind_calls}"
            )
            foreach(bind_call IN LISTS bind_calls)
                string(REGEX REPLACE
                    "BIND_(OPTIONAL|ARRAY)\\(([A-Za-z0-9_]+),.*"
                    "\\2"
                    field
                    "${bind_call}"
                )
                # Defensive filter for any residual macro-parameter tokens.
                if(NOT field STREQUAL "field" AND NOT field STREQUAL "slot")
                    list(APPEND assigned_fields "${field}")
                endif()
            endforeach()
            # Ialltoallw is intentionally forced to NULL inside the binder.
            if(request_bind_source MATCHES "unimpi\\.ialltoallw[ \t]*=")
                list(APPEND assigned_fields "ialltoallw")
            endif()
        endif()
    endif()
    list(REMOVE_DUPLICATES assigned_fields)

    set(missing_fields)
    foreach(field IN LISTS public_fields)
        list(FIND assigned_fields "${field}" assigned_index)
        if(assigned_index EQUAL -1)
            list(APPEND missing_fields "${field}")
        endif()
    endforeach()

    if(missing_fields)
        string(REPLACE ";" ", " missing_text "${missing_fields}")
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
