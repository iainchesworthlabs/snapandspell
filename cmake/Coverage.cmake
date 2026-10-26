# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# Coverage.cmake
#
# Defines an INTERFACE target `sns::coverage` that, when SNS_ENABLE_COVERAGE is
# on, turns on source-based coverage instrumentation. Off by default: only the
# dedicated *-coverage presets turn it on, so normal dev/CI builds pay no
# instrumentation cost.
#
# Two instrumentations, because the compilers' runtimes differ:
#
#   * GCC and Clang (GNU command line): --coverage, read back with gcovr.
#   * clang-cl (Clang with the MSVC frontend, the Windows preset): LLVM's
#     source-based coverage (-fprofile-instr-generate -fcoverage-mapping),
#     read back with llvm-profdata/llvm-cov. MSVC's cl has no equivalent.
#
# SNS_COVERAGE_STYLE is `gcov`, `llvm` or empty (not instrumented); the report
# targets and tools/coverage/run_coverage.cmake read it. Link sns::coverage
# PRIVATE into every first-party target whose coverage should be measured (the
# libraries, the programs and the test executables).
#
# The thresholds are the project's minimum standard: 90% of lines and 80% of
# branches. They are enforced by `coverage-unit` (and the Linux CI coverage
# job), not by an ordinary ctest run.
# ---------------------------------------------------------------------------

option(SNS_ENABLE_COVERAGE "Enable source coverage instrumentation (gcov or llvm-cov)" OFF)

set(SNS_COVERAGE_MIN_LINE 90 CACHE STRING
    "Coverage gate: the least percentage of lines a measured tier may execute")
set(SNS_COVERAGE_MIN_BRANCH 80 CACHE STRING
    "Coverage gate: the least percentage of branches a measured tier may take")
set(SNS_COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage" CACHE PATH
    "Where the coverage reports and profile data are written")

add_library(sns_coverage INTERFACE)
add_library(sns::coverage ALIAS sns_coverage)

set(SNS_COVERAGE_STYLE "")

if(SNS_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # clang-cl. The profile is written by the compiler-rt runtime at exit;
        # LLVM_PROFILE_FILE (set for each test run by run_coverage.cmake) names
        # the file and may contain %p (process id) and %m (binary signature).
        # /Ob0 is clang-cl's spelling of -fno-inline (it rejects the GNU one).
        # No link option: the instrumentation puts clang_rt.profile on the
        # object's default-library list, and CMake's Visual Studio generator
        # hands the link to lld-link, which would warn about the driver flag.
        set(SNS_COVERAGE_STYLE "llvm")
        target_compile_options(sns_coverage INTERFACE
            -fprofile-instr-generate
            -fcoverage-mapping
            /Ob0)
        # MSVC's STL vectorises std::find for 1/2/4/8-byte types and puts a
        # static_assert(false) in the discarded branch for every other size.
        # clang-cl diagnoses that assertion even when the branch is discarded,
        # so a 16-byte sns::Position refuses to compile. Turning the vectorised
        # algorithms off is the supported switch, and this is an instrumented
        # build, not a shipped one.
        target_compile_definitions(sns_coverage INTERFACE _USE_STD_VECTOR_ALGORITHMS=0)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(SNS_COVERAGE_STYLE "gcov")
        # -fprofile-update=atomic: the dictionary loader and the solver are
        # exercised from several threads in the tests, and gcov's default
        # non-atomic counter updates then race. gcov derives the counts of arcs
        # it does not instrument by subtraction, so a lost update shows up as a
        # negative count and gcovr aborts the report (gcc bug 68080).
        target_compile_options(sns_coverage INTERFACE
            --coverage -fprofile-update=atomic -fno-inline)
        target_link_options(sns_coverage INTERFACE --coverage)
    else()
        message(WARNING
            "SNS_ENABLE_COVERAGE is on but ${CMAKE_CXX_COMPILER_ID} has no "
            "source-based coverage runtime this project knows how to read; "
            "coverage instrumentation is skipped. Use GCC, Clang or clang-cl.")
    endif()
endif()

# ---------------------------------------------------------------------------
# The report targets. They are only defined in an instrumented build: a report
# made from a build without counters would show zero everywhere.
#
#   coverage-unit   runs the `unit` label, then reports and gates libs/core on
#                   90% lines / 80% branches (SNS_COVERAGE_MIN_*).
#   coverage-ui     runs the `ui` label (the Qt Quick Test suites and the two
#                   stub shells), then reports apps/app/src - the C++ the QML drives.
#   coverage        both, one after the other.
#
# Each target runs the tests itself, with a profile directory of its own, so the
# unit report is not polluted by the UI's run (or the other way round). The
# script needs ctest and the toolchain's report tool; it stops with a sentence
# naming what is missing rather than pretending the numbers are real.
# ---------------------------------------------------------------------------
function(sns_add_coverage_targets)
    if(SNS_COVERAGE_STYLE)
    # The report tools belong to the compiler that made the counters. clang-cl's
    # llvm-profdata/llvm-cov sit beside it; gcov is found beside the GNU compiler
    # first and then on PATH (the CI job pins the matching gcov by name).
    set(_sns_coverage_tools "")
    if(SNS_COVERAGE_STYLE STREQUAL "llvm")
        get_filename_component(_sns_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(SNS_LLVM_PROFDATA NAMES llvm-profdata HINTS "${_sns_compiler_dir}" NO_DEFAULT_PATH)
        find_program(SNS_LLVM_COV NAMES llvm-cov HINTS "${_sns_compiler_dir}" NO_DEFAULT_PATH)
        if(NOT SNS_LLVM_PROFDATA)
            find_program(SNS_LLVM_PROFDATA NAMES llvm-profdata)
        endif()
        if(NOT SNS_LLVM_COV)
            find_program(SNS_LLVM_COV NAMES llvm-cov)
        endif()
        if(NOT SNS_LLVM_PROFDATA OR NOT SNS_LLVM_COV)
            message(FATAL_ERROR
                "SNS_ENABLE_COVERAGE is on with clang-cl, and llvm-profdata/"
                "llvm-cov were not found beside ${CMAKE_CXX_COMPILER} or on "
                "PATH. Point PATH at the LLVM tools that came with the compiler.")
        endif()
        list(APPEND _sns_coverage_tools
            "-DLLVM_PROFDATA=${SNS_LLVM_PROFDATA}"
            "-DLLVM_COV=${SNS_LLVM_COV}")
    else()
        get_filename_component(_sns_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(SNS_GCOV NAMES gcov HINTS "${_sns_compiler_dir}" NO_DEFAULT_PATH)
        if(NOT SNS_GCOV)
            find_program(SNS_GCOV NAMES gcov)
        endif()
        # The CI job pins gcov to the compiler's major version (gcov-14 and the
        # like); honour that choice when it is in the environment.
        if(DEFINED ENV{GCOV} AND NOT "$ENV{GCOV}" STREQUAL "")
            set(SNS_GCOV "$ENV{GCOV}")
        endif()
        find_program(SNS_GCOVR NAMES gcovr)
        if(NOT SNS_GCOVR)
            message(FATAL_ERROR
                "SNS_ENABLE_COVERAGE is on and gcovr was not found. Install the "
                "pinned report tool: pip install -r .github/requirements/coverage.txt")
        endif()
        list(APPEND _sns_coverage_tools "-DGCOVR=${SNS_GCOVR}")
        if(SNS_GCOV)
            list(APPEND _sns_coverage_tools "-DGCOV=${SNS_GCOV}")
        endif()
    endif()

    set(_sns_coverage_common
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DOUTPUT_DIR=${SNS_COVERAGE_OUTPUT_DIR}"
        "-DMIN_LINE=${SNS_COVERAGE_MIN_LINE}"
        "-DMIN_BRANCH=${SNS_COVERAGE_MIN_BRANCH}"
        "-DSTYLE=${SNS_COVERAGE_STYLE}"
        "-DCTEST_CONFIG=$<CONFIG>"
        "-DCTEST_EXECUTABLE=${CMAKE_CTEST_COMMAND}"
        "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}"
        ${_sns_coverage_tools})

    # The first-party executable that is instrumented for the tier. A unit
    # report is read from sns_unit_tests, a UI report from sns_quick_tests:
    # llvm-cov wants the binary as well as the profile.
    set(_sns_unit_executable "")
    if(TARGET sns_unit_tests)
        set(_sns_unit_executable "$<TARGET_FILE:sns_unit_tests>")
    endif()
    set(_sns_ui_executable "")
    if(TARGET sns_quick_tests)
        set(_sns_ui_executable "$<TARGET_FILE:sns_quick_tests>")
    endif()

    if(_sns_unit_executable)
        add_custom_target(coverage-unit
            COMMAND "${CMAKE_COMMAND}"
                ${_sns_coverage_common}
                "-DTIER=unit"
                "-DLABEL=unit"
                "-DSCOPE=libs/core"
                "-DEXECUTABLE=${_sns_unit_executable}"
                -P "${PROJECT_SOURCE_DIR}/tools/coverage/run_coverage.cmake"
            USES_TERMINAL
            COMMENT "Coverage: unit tier over libs/core (gate ${SNS_COVERAGE_MIN_LINE} percent lines / ${SNS_COVERAGE_MIN_BRANCH} percent branches)")
        add_dependencies(coverage-unit sns_unit_tests)
    endif()

    if(_sns_ui_executable)
        add_custom_target(coverage-ui
            COMMAND "${CMAKE_COMMAND}"
                ${_sns_coverage_common}
                "-DTIER=ui"
                "-DLABEL=ui"
                "-DSCOPE=apps/app/src"
                "-DEXECUTABLE=${_sns_ui_executable}"
                "-DMIN_LINE=0"
                "-DMIN_BRANCH=0"
                -P "${PROJECT_SOURCE_DIR}/tools/coverage/run_coverage.cmake"
            USES_TERMINAL
            COMMENT "Coverage: the Qt Quick Test suites over apps/app/src")
        add_dependencies(coverage-ui sns_quick_tests)
        # The `ui` label also runs the two stub shells, and the Qt Quick Test
        # suites need the QML modules the application's own windeployqt run
        # puts beside the programs (QtQuick.Layouts and QtQuick.Shapes are not
        # imports of the test runner's own QML). Depend on all three so a clean
        # `coverage-ui` is a single command, not a build order the developer
        # has to know.
        if(TARGET sns_parent_preview)
            add_dependencies(coverage-ui sns_parent_preview)
        endif()
        if(TARGET sns_scan_preview)
            add_dependencies(coverage-ui sns_scan_preview)
        endif()
        if(TARGET SnapAndSpell)
            add_dependencies(coverage-ui SnapAndSpell)
        endif()
    endif()

    if(TARGET coverage-unit AND TARGET coverage-ui)
        add_custom_target(coverage)
        add_dependencies(coverage coverage-unit coverage-ui)
        # One after the other: both run ctest, and two suites building and
        # writing the same build tree at once is noise in a report about a
        # stopwatch-free tier.
        add_dependencies(coverage-ui coverage-unit)
    elseif(TARGET coverage-unit)
        add_custom_target(coverage)
        add_dependencies(coverage coverage-unit)
    elseif(TARGET coverage-ui)
        add_custom_target(coverage)
        add_dependencies(coverage coverage-ui)
    endif()

    # The top-level CMakeLists' folder pass has already run by the time this
    # deferred function does, so the report targets are put in it here.
    foreach(_target IN ITEMS coverage coverage-unit coverage-ui)
        if(TARGET ${_target})
            set_target_properties(${_target} PROPERTIES FOLDER "Tests")
        endif()
    endforeach()
    endif()
endfunction()

# The test targets do not exist when this module is included (Coverage.cmake is
# read before add_subdirectory(tests)); the report targets need to know whether
# sns_unit_tests and sns_quick_tests were built, so they are made at the end of
# the directory's processing instead.
cmake_language(DEFER CALL sns_add_coverage_targets)
