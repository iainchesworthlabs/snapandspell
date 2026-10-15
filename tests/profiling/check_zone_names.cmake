# ---------------------------------------------------------------------------
# Every zone has a name of the right shape, and no name is used twice.
#
#   cmake -DREPO_ROOT=<repository root> -P tests/profiling/check_zone_names.cmake
#
# A zone's source location is a function-local static of zone<"name">() (see
# src/lib/variants/profiling-enabled/wjs/profiling.hpp), so there is one per
# NAME: a name used at two places shows the first one's file and line, and the
# second's call is indistinguishable from it in a trace. The names are also
# what the CSV export and the profiler show and what any baseline reads, so
# they are `<layer>.<thing>`, lower case, dotted (docs/profiling.md).
#
# Scans src/ for `zone<"...">`. The variant headers and profiling_types.hpp are
# skipped (they are where the call is defined, and their comments quote it), and
# so is every line that is a // comment.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

if(NOT REPO_ROOT)
    message(FATAL_ERROR "Pass -DREPO_ROOT=<repository root>.")
endif()

file(GLOB_RECURSE _sources
    "${REPO_ROOT}/src/*.cpp" "${REPO_ROOT}/src/*.hpp" "${REPO_ROOT}/src/*.mm")

set(_seen "")
set(_failures "")
set(_count 0)
foreach(_source IN LISTS _sources)
    file(RELATIVE_PATH _relative "${REPO_ROOT}" "${_source}")
    if(_relative MATCHES "^src/lib/variants/" OR _relative STREQUAL "src/lib/include/wjs/profiling_types.hpp")
        continue()
    endif()

    file(STRINGS "${_source}" _lines)
    set(_number 0)
    foreach(_line IN LISTS _lines)
        math(EXPR _number "${_number} + 1")
        if(_line MATCHES "^[ \t]*//")
            continue()
        endif()
        if(NOT _line MATCHES "zone<\"([^\"]*)\">")
            continue()
        endif()
        set(_name "${CMAKE_MATCH_1}")
        math(EXPR _count "${_count} + 1")

        if(NOT _name MATCHES "^[a-z][a-z0-9_]*(\\.[a-z][a-z0-9_]*)+$")
            set(_failures "${_failures}\n  ${_relative}:${_number}: '${_name}' is not <layer>.<thing>, lower case and dotted")
        endif()
        if("${_name}" IN_LIST _seen)
            set(_failures "${_failures}\n  ${_relative}:${_number}: '${_name}' is used again; one name is one place")
        endif()
        list(APPEND _seen "${_name}")
    endforeach()
endforeach()

if(_failures)
    message(FATAL_ERROR "Zone names:${_failures}")
endif()
message(STATUS "OK: ${_count} zones, each named once")
