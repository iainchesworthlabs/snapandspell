find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(wjs_unit_tests
    unit/board_test.cpp
    unit/dictionary_test.cpp
    unit/paths_test.cpp
    unit/scoring_test.cpp
    unit/solver_test.cpp)

target_link_libraries(wjs_unit_tests
    PRIVATE
        wjs::lib
        wjs::platform
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)

wjs_deploy_sanitizer_runtime(wjs_unit_tests)

catch_discover_tests(wjs_unit_tests)

# Every platform fragment satisfies the platform contract, on whichever host
# this runs (a build only includes the one fragment for its own platform).
add_test(NAME platform.fragments
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/platform/check_fragments.cmake")

# No preprocessor conditionals or defines, no OS headers outside the platform
# section, no OS tests in CMake outside the selector. CI runs it as its own job
# too; this is the same check, so a developer sees it fail locally.
find_program(WJS_PWSH NAMES pwsh)
if(WJS_PWSH)
    add_test(NAME platform.isolation
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/checks/check_platform_isolation.ps1"
            -Root "${PROJECT_SOURCE_DIR}")
endif()
