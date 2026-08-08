cmake_minimum_required(VERSION 3.10)

# Backend API coverage parity check for UniMPI
#
# Verifies that all four backends (OpenMPI, MPICH, Intel-MPI, MS-MPI)
# provide consistent vtable field coverage for the MPI functions
# we claim to support.
#
# Usage:
#   cmake -DUNIMPI_SOURCE_DIR=/path/to/source -P check_backend_parity.cmake
#

if(NOT DEFINED UNIMPI_SOURCE_DIR)
    message(FATAL_ERROR "UNIMPI_SOURCE_DIR is required")
endif()

set(PARITY_CHECK_PASSED TRUE)
set(PARITY_WARNINGS)

message(STATUS "Checking backend parity in: ${UNIMPI_SOURCE_DIR}")

# Function to extract vtable assignments from a backend file
function(extract_vtable_assignments BACKEND_FILE OUT_VAR)
    if(NOT EXISTS "${BACKEND_FILE}")
        set(${OUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${BACKEND_FILE}" backend_source)
    string(REGEX MATCHALL
        "unimpi\\.[A-Za-z0-9_]+[ \\t]*="
        assignments
        "${backend_source}"
    )

    set(ASSIGNED_FIELDS)
    foreach(assignment IN LISTS assignments)
        string(REGEX REPLACE
            "unimpi\\.([A-Za-z0-9_]+).*"
            "\\1"
            field
            "${assignment}"
        )
        list(APPEND ASSIGNED_FIELDS "${field}")
    endforeach()
    list(REMOVE_DUPLICATES ASSIGNED_FIELDS)
    set(${OUT_VAR} "${ASSIGNED_FIELDS}" PARENT_SCOPE)
endfunction()

# Extract assignments from all backends
set(BACKEND_OPENMPI "${UNIMPI_SOURCE_DIR}/src/backends/openmpi.c")
set(BACKEND_MPICH "${UNIMPI_SOURCE_DIR}/src/backends/mpich.c")
set(BACKEND_INTEL "${UNIMPI_SOURCE_DIR}/src/backends/intelmpi.c")
set(BACKEND_MSMPI "${UNIMPI_SOURCE_DIR}/src/backends/msmpi.c")

extract_vtable_assignments("${BACKEND_OPENMPI}" OPENMPI_FIELDS)
extract_vtable_assignments("${BACKEND_MPICH}" MPICH_FIELDS)
extract_vtable_assignments("${BACKEND_INTEL}" INTEL_FIELDS)
extract_vtable_assignments("${BACKEND_MSMPI}" MSMPI_FIELDS)

# Helper function to check if list contains item
function(list_contains LIST_VAR ITEM RESULT_VAR)
    set(RESULT FALSE)
    foreach(ITEM_IN_LIST ${${LIST_VAR}})
        if(ITEM_IN_LIST STREQUAL ITEM)
            set(RESULT TRUE)
            break()
        endif()
    endforeach()
    set(${RESULT_VAR} ${RESULT} PARENT_SCOPE)
endfunction()

# Compare backends
message(STATUS "")
message(STATUS "=== Backend Coverage Analysis ===")
message(STATUS "")

# Core functions that should be present in all backends
set(CORE_FUNCTIONS
    init
    finalize
    comm_size
    comm_rank
    send
    recv
    wait
    waitall
    barrier
    bcast
    reduce
    allreduce
)

# Check core function coverage
message(STATUS "Core function coverage:")
foreach(FUNC ${CORE_FUNCTIONS})
    set(OPENMPI_HAS FALSE)
    set(MPICH_HAS FALSE)
    set(INTEL_HAS FALSE)
    set(MSMPI_HAS FALSE)

    list_contains(OPENMPI_FIELDS "${FUNC}" OPENMPI_HAS)
    list_contains(MPICH_FIELDS "${FUNC}" MPICH_HAS)
    list_contains(INTEL_FIELDS "${FUNC}" INTEL_HAS)
    list_contains(MSMPI_FIELDS "${FUNC}" MSMPI_HAS)

    set(MISSING)
    if(NOT OPENMPI_HAS)
        list(APPEND MISSING "OpenMPI")
    endif()
    if(NOT MPICH_HAS)
        list(APPEND MISSING "MPICH")
    endif()
    if(NOT INTEL_HAS)
        list(APPEND MISSING "Intel")
    endif()
    if(NOT MSMPI_HAS)
        list(APPEND MISSING "MS-MPI")
    endif()

    if(MISSING)
        message(STATUS "  [WARN] ${FUNC}: missing in ${MISSING}")
        list(APPEND PARITY_WARNINGS "${FUNC} missing in ${MISSING}")
    else()
        message(STATUS "  [OK]   ${FUNC}")
    endif()
endforeach()

# Check request array operations
message(STATUS "")
message(STATUS "Request array operations:")
set(REQUEST_ARRAY_FUNCS
    testany
    testsome
    testall
    waitany
    waitsome
    startall
)

foreach(FUNC ${REQUEST_ARRAY_FUNCS})
    set(OPENMPI_HAS FALSE)
    set(MPICH_HAS FALSE)
    set(INTEL_HAS FALSE)
    set(MSMPI_HAS FALSE)

    list_contains(OPENMPI_FIELDS "${FUNC}" OPENMPI_HAS)
    list_contains(MPICH_FIELDS "${FUNC}" MPICH_HAS)
    list_contains(INTEL_FIELDS "${FUNC}" INTEL_HAS)
    list_contains(MSMPI_FIELDS "${FUNC}" MSMPI_HAS)

    if(OPENMPI_HAS AND MPICH_HAS AND INTEL_HAS AND MSMPI_HAS)
        message(STATUS "  [OK]   ${FUNC}")
    else()
        message(STATUS "  [INFO] ${FUNC}: coverage varies by backend")
    endif()
endforeach()

# Check non-blocking collectives
message(STATUS "")
message(STATUS "Non-blocking collective support:")
set(NBC_FUNCS
    ibarrier
    ibcast
    igather
    iscatter
    iallgather
    ialltoall
    ialltoallv
    ialltoallw
    ireduce
    iallreduce
    iscan
    iexscan
)

foreach(FUNC ${NBC_FUNCS})
    set(OPENMPI_HAS FALSE)
    set(MPICH_HAS FALSE)
    set(INTEL_HAS FALSE)
    set(MSMPI_HAS FALSE)

    list_contains(OPENMPI_FIELDS "${FUNC}" OPENMPI_HAS)
    list_contains(MPICH_FIELDS "${FUNC}" MPICH_HAS)
    list_contains(INTEL_FIELDS "${FUNC}" INTEL_HAS)
    list_contains(MSMPI_FIELDS "${FUNC}" MSMPI_HAS)

    set(MISSING)
    if(NOT OPENMPI_HAS)
        list(APPEND MISSING "OpenMPI")
    endif()
    if(NOT MPICH_HAS)
        list(APPEND MISSING "MPICH")
    endif()
    if(NOT INTEL_HAS)
        list(APPEND MISSING "Intel")
    endif()
    if(NOT MSMPI_HAS)
        list(APPEND MISSING "MS-MPI")
    endif()

    if(MISSING)
        # Non-blocking collectives may vary - just INFO level
        message(STATUS "  [INFO] ${FUNC}: not available in ${MISSING}")
    else()
        message(STATUS "  [OK]   ${FUNC}")
    endif()
endforeach()

# Check extended APIs
message(STATUS "")
message(STATUS "Extended API support:")
set(EXTENDED_FUNCS
    # RMA
    win_create
    win_free
    put
    get
    accumulate
    # I/O
    file_open
    file_close
    # Dynamic
    comm_spawn
    comm_accept
    comm_connect
)

foreach(FUNC ${EXTENDED_FUNCS})
    set(OPENMPI_HAS FALSE)
    set(MPICH_HAS FALSE)
    set(INTEL_HAS FALSE)
    set(MSMPI_HAS FALSE)

    list_contains(OPENMPI_FIELDS "${FUNC}" OPENMPI_HAS)
    list_contains(MPICH_FIELDS "${FUNC}" MPICH_HAS)
    list_contains(INTEL_FIELDS "${FUNC}" INTEL_HAS)
    list_contains(MSMPI_FIELDS "${FUNC}" MSMPI_HAS)

    set(COUNT 0)
    if(OPENMPI_HAS)
        math(EXPR COUNT "${COUNT} + 1")
    endif()
    if(MPICH_HAS)
        math(EXPR COUNT "${COUNT} + 1")
    endif()
    if(INTEL_HAS)
        math(EXPR COUNT "${COUNT} + 1")
    endif()
    if(MSMPI_HAS)
        math(EXPR COUNT "${COUNT} + 1")
    endif()

    if(COUNT EQUAL 4)
        message(STATUS "  [OK]   ${FUNC}")
    elseif(COUNT GREATER 0)
        message(STATUS "  [INFO] ${FUNC}: partial coverage (${COUNT}/4)")
    else()
        message(STATUS "  [WARN] ${FUNC}: no coverage")
    endif()
endforeach()

# Summary
message(STATUS "")
message(STATUS "=== Backend Parity Summary ===")

list(LENGTH OPENMPI_FIELDS OPENMPI_COUNT)
list(LENGTH MPICH_FIELDS MPICH_COUNT)
list(LENGTH INTEL_FIELDS INTEL_COUNT)
list(LENGTH MSMPI_FIELDS MSMPI_COUNT)

message(STATUS "OpenMPI:  ${OPENMPI_COUNT} vtable assignments")
message(STATUS "MPICH:    ${MPICH_COUNT} vtable assignments")
message(STATUS "Intel:    ${INTEL_COUNT} vtable assignments")
message(STATUS "MS-MPI:   ${MSMPI_COUNT} vtable assignments")

list(LENGTH PARITY_WARNINGS WARN_COUNT)
if(WARN_COUNT GREATER 0)
    message(STATUS "")
    message(STATUS "Warnings: ${WARN_COUNT}")
    foreach(WARN ${PARITY_WARNINGS})
        message(STATUS "  - ${WARN}")
    endforeach()
    message(STATUS "")
    message(FATAL_ERROR "Backend parity check failed with ${WARN_COUNT} warnings")
else()
    message(STATUS "No coverage warnings")
    message(STATUS "All backends provide consistent core API coverage")
endif()
