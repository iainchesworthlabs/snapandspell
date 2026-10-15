# CMake 4 is required; the version range keeps policy behaviour current up to 4.4.
cmake_minimum_required(VERSION 3.28...4.4)

# ---------------------------------------------------------------------------
# Vendored vcpkg (git submodule at deps/vcpkg).
#
# The dependency provider is pinned by the submodule so builds are fully
# reproducible without a machine-wide vcpkg or a VCPKG_ROOT env var. The
# CMakePresets point CMAKE_TOOLCHAIN_FILE at deps/vcpkg/scripts/buildsystems.
# If the submodule was not checked out, initialise it here so a fresh clone
# configures without extra steps.
# ---------------------------------------------------------------------------
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/vcpkg/scripts/buildsystems/vcpkg.cmake")
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        message(STATUS "Initialising vendored vcpkg submodule (deps/vcpkg)")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" submodule update --init --depth 1 -- deps/vcpkg
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE WJS_SUBMODULE_RESULT)
        if(NOT WJS_SUBMODULE_RESULT EQUAL 0)
            message(FATAL_ERROR
                "Failed to initialise the vcpkg submodule. Run:\n"
                "  git submodule update --init --depth 1 deps/vcpkg")
        endif()
    endif()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/vcpkg/scripts/buildsystems/vcpkg.cmake")
    message(FATAL_ERROR
        "Vendored vcpkg not found at deps/vcpkg.\n"
        "Clone with submodules (git clone --recurse-submodules ...) or run:\n"
        "  git submodule update --init --depth 1 deps/vcpkg")
endif()

# ---------------------------------------------------------------------------
# Select vcpkg manifest features BEFORE project() (the vcpkg toolchain reads
# VCPKG_MANIFEST_FEATURES when it is loaded at project() time). Qt is a
# prebuilt dependency found via CMAKE_PREFIX_PATH (see cmake/FindQt6.cmake),
# not built by vcpkg.
# ---------------------------------------------------------------------------
if(NOT DEFINED WJS_BUILD_TESTS OR WJS_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(WordJewelsSolver
    VERSION 0.1.0
    DESCRIPTION "Finds every word on a Boggle board"
    HOMEPAGE_URL "https://github.com/iainchesworthlabs/wordjewelssolver"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global language settings: C++23 only, no compiler extensions.
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clang-tidy / editors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put all runtime artefacts in one place.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# The one place the target OS is decided (cmake/Platform.cmake); every other
# CMake file reads what the platform's fragment declared.
include(Platform)
include(CompilerWarnings)
include(Sanitizers)
include(Coverage)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(WJS_BUILD_CLI "Build the wjs-cli command-line solver" ON)
option(WJS_BUILD_TESTS "Build the unit test suite" ON)

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
# Everything that differs between operating systems, behind the interfaces in
# src/platforms/interfaces. Compiled from the sources the selected platform's
# platform.cmake names.
add_subdirectory(src/platforms)

# The library holds ALL of the game logic and is completely GUI-free and
# platform-free.
add_subdirectory(src/lib)

if(WJS_BUILD_CLI)
    add_subdirectory(src/cli)
endif()

if(WJS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "WordJewelsSolver ${PROJECT_VERSION} for ${WJS_PLATFORM_DISPLAY_NAME}")
message(STATUS "  Build CLI   : ${WJS_BUILD_CLI}")
message(STATUS "  Build tests : ${WJS_BUILD_TESTS}")
