# ---------------------------------------------------------------------------
# Nothing that ships has Tracy in it.
#
#   cmake -DREPO_ROOT=<repository root> -P tests/profiling/check_presets.cmake
#
# A Tracy client opens a TCP listener (port 8086, on every interface) and
# answers UDP discovery broadcasts, independently of anything the application
# does (docs/profiling.md, "The profiler listens on the network"). So a build
# with it in is a development build and is never packaged. This is the check
# that a shipped preset did not quietly turn it on, and it runs in every
# ctest, which is every CI leg:
#
#   1. Resolving `inherits` the way CMake does, WJS_ENABLE_TRACY is ON for a
#      configure preset only if the preset is named *-profiling (or is the
#      hidden `profiling` preset those inherit). The *-release presets, which
#      are what a package is built from, are named in the failure if they are.
#   2. The option's default, in cmake/Tracy.cmake, is OFF.
#   3. Tracy is a vcpkg manifest FEATURE (`profiling`) and not a base
#      dependency, and the feature is not a default feature.
#   4. The top-level CMakeLists.txt adds that feature only when the option is on.
#   5. No CI workflow turns the option on by hand.
#
# It runs on whatever host runs the tests: only the presets' JSON is read.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

if(NOT REPO_ROOT)
    message(FATAL_ERROR "Pass -DREPO_ROOT=<repository root>.")
endif()

set(_failures "")
function(_fail text)
    set(_failures "${_failures}\n  ${text}" PARENT_SCOPE)
endfunction()

# ---- 1. the presets ----------------------------------------------------------

file(READ "${REPO_ROOT}/CMakePresets.json" _presets)
string(JSON _count LENGTH "${_presets}" configurePresets)
if(_count EQUAL 0)
    message(FATAL_ERROR "CMakePresets.json has no configure presets.")
endif()

set(_names "")
math(EXPR _last "${_count} - 1")
foreach(_i RANGE 0 ${_last})
    string(JSON _name GET "${_presets}" configurePresets ${_i} name)
    list(APPEND _names "${_name}")

    # What the preset itself sets (empty: it says nothing about Tracy).
    set(_value "")
    string(JSON _type ERROR_VARIABLE _error TYPE "${_presets}" configurePresets ${_i}
        cacheVariables WJS_ENABLE_TRACY)
    if(_error STREQUAL "NOTFOUND")
        if(_type STREQUAL "OBJECT")
            string(JSON _value GET "${_presets}" configurePresets ${_i}
                cacheVariables WJS_ENABLE_TRACY value)
        else()
            string(JSON _value GET "${_presets}" configurePresets ${_i}
                cacheVariables WJS_ENABLE_TRACY)
        endif()
    endif()
    set("_VALUE_${_name}" "${_value}")

    # What it inherits from, in order (a string or an array of strings).
    set(_parents "")
    string(JSON _type ERROR_VARIABLE _error TYPE "${_presets}" configurePresets ${_i} inherits)
    if(_error STREQUAL "NOTFOUND")
        if(_type STREQUAL "ARRAY")
            string(JSON _n LENGTH "${_presets}" configurePresets ${_i} inherits)
            if(_n GREATER 0)
                math(EXPR _m "${_n} - 1")
                foreach(_j RANGE 0 ${_m})
                    string(JSON _parent GET "${_presets}" configurePresets ${_i} inherits ${_j})
                    list(APPEND _parents "${_parent}")
                endforeach()
            endif()
        else()
            string(JSON _parent GET "${_presets}" configurePresets ${_i} inherits)
            list(APPEND _parents "${_parent}")
        endif()
    endif()
    set("_INHERITS_${_name}" "${_parents}")
endforeach()

# The value a preset ends up with: its own, else its first parent that has one
# (CMake prefers the earlier preset in `inherits`).
function(_effective name result)
    if(NOT "${_VALUE_${name}}" STREQUAL "")
        set(${result} "${_VALUE_${name}}" PARENT_SCOPE)
        return()
    endif()
    foreach(_parent IN LISTS _INHERITS_${name})
        _effective("${_parent}" _inherited)
        if(NOT "${_inherited}" STREQUAL "")
            set(${result} "${_inherited}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${result} "" PARENT_SCOPE)
endfunction()

set(_profiling_presets "")
foreach(_name IN LISTS _names)
    _effective("${_name}" _value)
    if(_value MATCHES "^(ON|TRUE|YES|Y|1)$" OR _value MATCHES "^[1-9][0-9]*$")
        set(_on TRUE)
    else()
        set(_on FALSE)
    endif()
    set(_is_profiling_name FALSE)
    if(_name MATCHES "-profiling$" OR _name STREQUAL "profiling")
        set(_is_profiling_name TRUE)
    endif()

    if(_on AND NOT _is_profiling_name)
        _fail("configure preset '${_name}' turns WJS_ENABLE_TRACY on; only *-profiling presets may")
    endif()
    if(_name MATCHES "release" AND _on)
        _fail("the release preset '${_name}' has Tracy in it; a package is built from it")
    endif()
    if(_on)
        list(APPEND _profiling_presets "${_name}")
    endif()
endforeach()

if(NOT _profiling_presets)
    _fail("no preset turns WJS_ENABLE_TRACY on, so the profiling build cannot be made from a preset")
endif()

# ---- 2. the option's default -------------------------------------------------

file(READ "${REPO_ROOT}/cmake/Tracy.cmake" _tracy_cmake)
if(NOT _tracy_cmake MATCHES "option\\(WJS_ENABLE_TRACY \"[^\"]*\" OFF\\)")
    _fail("cmake/Tracy.cmake: option(WJS_ENABLE_TRACY ...) is not declared with default OFF")
endif()

# ---- 3. the manifest ---------------------------------------------------------

file(READ "${REPO_ROOT}/vcpkg.json" _manifest)
string(JSON _deps LENGTH "${_manifest}" dependencies)
if(_deps GREATER 0)
    math(EXPR _last "${_deps} - 1")
    foreach(_i RANGE 0 ${_last})
        string(JSON _type TYPE "${_manifest}" dependencies ${_i})
        if(_type STREQUAL "OBJECT")
            string(JSON _dep GET "${_manifest}" dependencies ${_i} name)
        else()
            string(JSON _dep GET "${_manifest}" dependencies ${_i})
        endif()
        if(_dep STREQUAL "tracy")
            _fail("vcpkg.json: tracy is a base dependency; it belongs only to the 'profiling' feature")
        endif()
    endforeach()
endif()

string(JSON _defaults ERROR_VARIABLE _error GET "${_manifest}" default-features)
if(_error STREQUAL "NOTFOUND")
    string(JSON _n LENGTH "${_manifest}" default-features)
    if(_n GREATER 0)
        math(EXPR _last "${_n} - 1")
        foreach(_i RANGE 0 ${_last})
            string(JSON _feature GET "${_manifest}" default-features ${_i})
            if(_feature STREQUAL "profiling")
                _fail("vcpkg.json: 'profiling' is a default feature, so every build installs Tracy")
            endif()
        endforeach()
    endif()
endif()

string(JSON _type ERROR_VARIABLE _error TYPE "${_manifest}" features profiling)
if(NOT _error STREQUAL "NOTFOUND")
    _fail("vcpkg.json has no 'profiling' feature for the option to select")
endif()

# ---- 4. the one place the feature is requested --------------------------------

file(READ "${REPO_ROOT}/CMakeLists.txt" _root)
string(REGEX MATCHALL "[^\n]*\"profiling\"[^\n]*" _mentions "${_root}")
if(NOT _root MATCHES "if\\(WJS_ENABLE_TRACY\\)[ \t\r\n]*list\\(APPEND VCPKG_MANIFEST_FEATURES \"profiling\"\\)")
    _fail("CMakeLists.txt does not add the 'profiling' feature under if(WJS_ENABLE_TRACY) alone")
endif()
list(LENGTH _mentions _mention_count)
if(_mention_count GREATER 1)
    _fail("CMakeLists.txt names the 'profiling' feature more than once: ${_mentions}")
endif()

# ---- 5. CI -------------------------------------------------------------------

file(GLOB _workflows "${REPO_ROOT}/.github/workflows/*.yml" "${REPO_ROOT}/.github/workflows/*.yaml")
foreach(_workflow IN LISTS _workflows)
    file(READ "${_workflow}" _text)
    if(_text MATCHES "WJS_ENABLE_TRACY=(ON|1|TRUE)")
        get_filename_component(_file "${_workflow}" NAME)
        _fail("${_file} turns WJS_ENABLE_TRACY on by hand; use a *-profiling preset")
    endif()
endforeach()

# ---- result ------------------------------------------------------------------

if(_failures)
    message(FATAL_ERROR "A shipped configuration can have Tracy in it:${_failures}")
endif()

list(LENGTH _names _preset_count)
list(JOIN _profiling_presets ", " _profiling_list)
message(STATUS "OK: ${_preset_count} configure presets; Tracy is on only in: ${_profiling_list}")
