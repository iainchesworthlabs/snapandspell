# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# Fuzzing.cmake
#
# SNS_ENABLE_FUZZING builds the libFuzzer harnesses under libs/<lib>/fuzz and instruments every
# target for them (AddressSanitizer, UndefinedBehaviorSanitizer and libFuzzer's coverage hooks).
# libFuzzer is Clang's, so the option needs Clang. It cannot be combined with the other
# instrumentation switches, Tracy or the hardening flags (a fortified libc call bypasses
# AddressSanitizer's interceptors); the linux-fuzz preset sets it on its own.
#
# sns_add_fuzz_target(<name> <library>) is called from a fuzz directory that holds <name>.cpp,
# corpus/<name>/ (seed inputs) and regressions/<name>/ (inputs that once crashed it). It adds the
# executable sns_fuzz_<name> and the CTest test fuzz.regress.<name>, which replays both directories
# once (-runs=0) and takes seconds. Fuzzing itself is `sns_fuzz_<name> corpus/<name>` for as long as
# you like (docs/fuzzing.md).
# ---------------------------------------------------------------------------

option(SNS_ENABLE_FUZZING "Build the libFuzzer harnesses (Clang only) and instrument the libraries for them" OFF)
# The sanitizers the harnesses are built with. ClusterFuzzLite-style callers that bring their own
# set it empty.
set(SNS_FUZZING_SANITIZERS "address,undefined" CACHE STRING
    "Comma-separated -fsanitize= list for the libFuzzer harnesses; empty adds none")

if(SNS_ENABLE_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR MSVC)
        message(FATAL_ERROR "SNS_ENABLE_FUZZING needs Clang (libFuzzer): the compiler is ${CMAKE_CXX_COMPILER_ID}.")
    endif()
    foreach(_sns_other IN ITEMS SNS_ENABLE_SANITIZERS SNS_ENABLE_TSAN SNS_ENABLE_TRACY SNS_ENABLE_COVERAGE
                                SNS_ENABLE_HARDENING)
        if(${_sns_other})
            message(FATAL_ERROR "SNS_ENABLE_FUZZING and ${_sns_other} cannot both be on. Use the fuzz preset.")
        endif()
    endforeach()
    unset(_sns_other)
    set(_sns_fuzz_compile fuzzer-no-link)
    if(SNS_FUZZING_SANITIZERS)
        string(APPEND _sns_fuzz_compile ",${SNS_FUZZING_SANITIZERS}")
        add_link_options(-fsanitize=${SNS_FUZZING_SANITIZERS})
    endif()
    add_compile_options(
        -fsanitize=${_sns_fuzz_compile}
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
        -O1)
    unset(_sns_fuzz_compile)
    message(STATUS "Fuzzing enabled: libFuzzer harnesses with sanitizers '${SNS_FUZZING_SANITIZERS}'")
endif()

function(sns_add_fuzz_target name library)
    set(target sns_fuzz_${name})
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp")
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/tests/support")
    target_compile_features(${target} PRIVATE cxx_std_23)
    target_link_libraries(${target} PRIVATE ${library} sns::warnings)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer)
    add_test(NAME fuzz.regress.${name}
        COMMAND ${target} -runs=0
            "${CMAKE_CURRENT_SOURCE_DIR}/corpus/${name}"
            "${CMAKE_CURRENT_SOURCE_DIR}/regressions/${name}")
    set_tests_properties(fuzz.regress.${name} PROPERTIES TIMEOUT 120 LABELS "fuzz")
endfunction()
