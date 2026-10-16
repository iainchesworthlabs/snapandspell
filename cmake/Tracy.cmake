# ---------------------------------------------------------------------------
# Tracy.cmake
#
# Defines an INTERFACE target `wjs::tracy` that, when WJS_ENABLE_TRACY is on,
# links Tracy's client library. The profiling calls the sources make
# (wjs/profiling.hpp) are one of two headers with the same path under
# src/lib/variants/; src/lib/CMakeLists.txt puts exactly one of them on
# wjs::lib's include path, chosen by this option and not by an #if, so every
# instrumented call site compiles unchanged either way. The disabled variant
# holds empty functions: a normal build pays nothing for the instrumentation
# and does not need Tracy present.
#
# Off by default. Tracy itself is an OPT-IN vcpkg manifest feature
# ("profiling" in vcpkg.json, not a base dependency); the top-level
# CMakeLists.txt adds it to VCPKG_MANIFEST_FEATURES when this option is on,
# so one switch is the whole story:
#
#   cmake --preset windows-msvc-profiling   # or linux-gcc- / macos-clang-
#
# The feature also builds Tracy's command-line tools for the HOST
# (tracy-capture and tracy-csvexport), so a trace can be recorded and dumped
# without the GUI profiler. docs/profiling.md says how.
#
# Nothing here is for shipping: a Tracy client opens a TCP listener and answers
# UDP discovery broadcasts, which a released binary must not do. Only the
# *-profiling presets turn this on, and the profiling.shipped_presets test
# (tests/profiling/check_presets.cmake) holds that.
# ---------------------------------------------------------------------------

option(WJS_ENABLE_TRACY "Enable Tracy profiler instrumentation (never for a shipped build)" OFF)

add_library(wjs_tracy INTERFACE)
add_library(wjs::tracy ALIAS wjs_tracy)

# Which wjs/profiling.hpp wjs::lib's users see.
if(WJS_ENABLE_TRACY)
    set(WJS_PROFILING_VARIANT_DIR "${PROJECT_SOURCE_DIR}/src/lib/variants/profiling-enabled")
else()
    set(WJS_PROFILING_VARIANT_DIR "${PROJECT_SOURCE_DIR}/src/lib/variants/profiling-disabled")
endif()

if(WJS_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(wjs_tracy INTERFACE Tracy::TracyClient)

    # Not an error: someone chasing a bug in the profiling build itself wants
    # exactly this. But a number taken from a sanitizer or coverage build is
    # not a number about the program, and nothing else will say so.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR WJS_ENABLE_COVERAGE)
        message(WARNING
            "WJS_ENABLE_TRACY is on for a Debug or coverage build. Sanitizer "
            "and coverage instrumentation dominate the timings Tracy reports; "
            "use a RelWithDebInfo build (the *-profiling presets) for numbers "
            "worth reading.")
    endif()
endif()
