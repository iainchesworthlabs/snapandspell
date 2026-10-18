# CMake 4 is required; the version range keeps policy behaviour current up to 4.4.
cmake_minimum_required(VERSION 3.28...4.4)

# ---------------------------------------------------------------------------
# Shared vcpkg checkout, found through the VCPKG_ROOT environment variable.
#
# One vcpkg clone serves every project on the machine. The CMakePresets point
# CMAKE_TOOLCHAIN_FILE at $env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake.
# cmake/vcpkg-commit.txt is the vcpkg commit this project was verified with:
# the checkout must contain it (be at it, or ahead of it), so a stale clone
# that predates the baseline fails here rather than deep inside a port build.
# ---------------------------------------------------------------------------
if(NOT CMAKE_TOOLCHAIN_FILE OR NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(FATAL_ERROR
        "The vcpkg toolchain file was not found.\n"
        "Set VCPKG_ROOT to a clone of https://github.com/iainchesworthlabs/vcpkg "
        "(or pass -DCMAKE_TOOLCHAIN_FILE=<root>/scripts/buildsystems/vcpkg.cmake) "
        "and reconfigure. See docs/building.md.")
endif()

get_filename_component(WJS_VCPKG_ROOT "${CMAKE_TOOLCHAIN_FILE}/../../.." ABSOLUTE)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vcpkg-commit.txt" WJS_VCPKG_PIN
    LIMIT_COUNT 1)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vcpkg-commit.txt")

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${WJS_VCPKG_ROOT}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${WJS_VCPKG_ROOT}"
                merge-base --is-ancestor "${WJS_VCPKG_PIN}" HEAD
        RESULT_VARIABLE WJS_VCPKG_PIN_RESULT
        ERROR_QUIET)
    if(WJS_VCPKG_PIN_RESULT EQUAL 1)
        message(FATAL_ERROR
            "The vcpkg checkout at ${WJS_VCPKG_ROOT} does not contain the pinned "
            "commit ${WJS_VCPKG_PIN} (cmake/vcpkg-commit.txt).\n"
            "Update it:\n"
            "  git -C \"${WJS_VCPKG_ROOT}\" pull")
    elseif(NOT WJS_VCPKG_PIN_RESULT EQUAL 0)
        message(FATAL_ERROR
            "The vcpkg checkout at ${WJS_VCPKG_ROOT} does not have the pinned "
            "commit ${WJS_VCPKG_PIN} (cmake/vcpkg-commit.txt) in its history; "
            "it may be a shallow clone or from a different remote.\n"
            "Fetch it:\n"
            "  git -C \"${WJS_VCPKG_ROOT}\" fetch origin")
    endif()
else()
    message(WARNING
        "${WJS_VCPKG_ROOT} is not a git checkout, so the pinned vcpkg commit "
        "${WJS_VCPKG_PIN} cannot be verified.")
endif()
message(STATUS "vcpkg: ${WJS_VCPKG_ROOT} (pin ${WJS_VCPKG_PIN})")

# ---------------------------------------------------------------------------
# Select vcpkg manifest features BEFORE project() (the vcpkg toolchain reads
# VCPKG_MANIFEST_FEATURES when it is loaded at project() time). Qt is a
# prebuilt dependency found via CMAKE_PREFIX_PATH (see cmake/FindQt6.cmake),
# not built by vcpkg.
# ---------------------------------------------------------------------------
if(NOT DEFINED WJS_BUILD_TESTS OR WJS_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

# Tracy is opt-in: the one switch (declared in cmake/Tracy.cmake) both makes
# the profiling calls real and asks vcpkg for the client and the tools.
if(WJS_ENABLE_TRACY)
    list(APPEND VCPKG_MANIFEST_FEATURES "profiling")
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
include(Tracy)
# The interface's languages (cmake/Translations.cmake): the lists lupdate scans
# and the catalogues lrelease compiles. The function itself is called by
# src/app once Qt's Linguist tools have been found.
include(Translations)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(WJS_BUILD_CLI "Build the wjs-cli command-line solver" ON)
# Off by default: the application needs a prebuilt Qt 6.12 found via
# QT_ROOT_DIR or CMAKE_PREFIX_PATH (cmake/FindQt6.cmake), which is not vcpkg's,
# so a machine that only builds the solver does not need one. The Windows CI
# leg installs it and turns it on. It links Qt Quick 3D for the board scene, so
# the binary it makes is GPL-3.0 only (the author's decision of 2026-10-16;
# docs/design/board.md).
option(WJS_BUILD_APP "Build the Qt Quick application (needs Qt 6.12; GPL-3.0 only)" OFF)
option(WJS_BUILD_TESTS "Build the unit test suite" ON)
# The harness around the 3D board scene (src/app/prototype/shake, and
# docs/design/shake-prototype.md): a window with a Shake button, a drag standing
# in for the accelerometer and the frame timing, over the scene module the
# application also links (WordJewelsSolver.Scene, GPL-3.0 only). A program of its
# own, not part of the application. Needs WJS_BUILD_APP=ON.
option(WJS_BUILD_SHAKE_PROTOTYPE "Build the 3D board-scene prototype harness (needs Qt Quick 3D; GPL-3.0 only)" OFF)

# The review gate. Phonics data (data/phonics) is written and reviewed by a
# person qualified to do it, and a file says so in its header (reviewed-by and
# reviewed-on). A file without them is test data. With this on, the build runs
# tools/data/check_phonics.ps1 -RequireReviewed before the library is built and
# stops if any file is test data, so a release cannot carry unreviewed content.
# On by default for the release build types, and for any multi-config generator
# (which has no build type at configure time, so it is the developer who says
# "not a release" by passing -DWJS_REQUIRE_REVIEWED_PHONICS=OFF).
if(CMAKE_CONFIGURATION_TYPES)
    set(WJS_REQUIRE_REVIEWED_PHONICS_DEFAULT ON)
elseif(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(WJS_REQUIRE_REVIEWED_PHONICS_DEFAULT ON)
else()
    set(WJS_REQUIRE_REVIEWED_PHONICS_DEFAULT OFF)
endif()
option(WJS_REQUIRE_REVIEWED_PHONICS
    "Refuse to build while data/phonics holds a file without a review header"
    ${WJS_REQUIRE_REVIEWED_PHONICS_DEFAULT})

# ---------------------------------------------------------------------------
# Qt, when the application is built. Found here, before any target and before
# the notices: the third-party notice names Qt's version (cmake/Notices.cmake),
# and imported targets are directory-scoped, so a find_package() at the top
# level is visible to tests/ and to every module under src/app.
# Quick3D is the board scene's, and it makes the application GPL-3.0 only: it is
# a required component now, not the prototype's alone. LinguistTools is the
# catalogues' (lupdate/lrelease).
# ---------------------------------------------------------------------------
if(WJS_BUILD_APP)
    set(WJS_QT_COMPONENTS Core Gui Qml Quick QuickControls2 Quick3D LinguistTools)
    find_package(Qt6 6.12 REQUIRED COMPONENTS ${WJS_QT_COMPONENTS})
    # The feedback engines' modules are optional: the application's sound,
    # speech and motion have silent variants selected by CMake when a module is
    # absent (src/app/feedback/CMakeLists.txt), so a contributor's kit without
    # them still configures and builds. The shared kit on the author's machine
    # has all three (docs/building.md, "The Qt Quick window").
    find_package(Qt6 6.12 QUIET OPTIONAL_COMPONENTS Multimedia Sensors TextToSpeech)
    set(WJS_QT_HAS_MULTIMEDIA OFF)
    set(WJS_QT_HAS_SENSORS OFF)
    set(WJS_QT_HAS_TEXTTOSPEECH OFF)
    if(TARGET Qt6::Multimedia)
        set(WJS_QT_HAS_MULTIMEDIA ON)
    endif()
    if(TARGET Qt6::Sensors)
        set(WJS_QT_HAS_SENSORS ON)
    endif()
    if(TARGET Qt6::TextToSpeech)
        set(WJS_QT_HAS_TEXTTOSPEECH ON)
    endif()
endif()

# ---------------------------------------------------------------------------
# The third-party notices this build carries (cmake/Notices.cmake): written once
# at configure time, installed beside the programs and embedded in the
# application as :/notices/NOTICES.txt, which the About screen shows. After Qt,
# whose version a build with the application names in them; before the targets,
# which refer to the generated file.
# ---------------------------------------------------------------------------
include(Notices)
wjs_write_notices()

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

# The scanner: finds a board in a camera frame and reads its letters. Its own
# library, so the solver library above stays free of image code, and with no
# dependency of its own to fetch.
add_subdirectory(src/scan)

find_program(WJS_PWSH NAMES pwsh)
if(WJS_REQUIRE_REVIEWED_PHONICS)
    if(NOT WJS_PWSH)
        message(FATAL_ERROR
            "WJS_REQUIRE_REVIEWED_PHONICS is ON, and it is checked by "
            "tools/data/check_phonics.ps1, which needs PowerShell 7 (pwsh). "
            "Install it, or configure with -DWJS_REQUIRE_REVIEWED_PHONICS=OFF "
            "for a build that is not to be released.")
    endif()
    # Always out of date, so every build looks at the data as it is now, not as
    # it was when CMake last ran. The library depends on it, so building any
    # target that links the library is gated.
    add_custom_target(wjs_phonics_review_gate ALL
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/data/check_phonics.ps1"
            -Root "${CMAKE_CURRENT_SOURCE_DIR}" -RequireReviewed
        COMMENT "Checking that the phonics data has been reviewed"
        VERBATIM)
    add_dependencies(wjs_lib wjs_phonics_review_gate)
endif()

if(WJS_BUILD_CLI)
    add_subdirectory(src/cli)
endif()

# The application is a thin Qt Quick layer over the library.
if(WJS_BUILD_APP)
    add_subdirectory(src/app)
elseif(WJS_BUILD_SHAKE_PROTOTYPE)
    message(WARNING "WJS_BUILD_SHAKE_PROTOTYPE needs WJS_BUILD_APP=ON; the prototype is not built.")
endif()

if(WJS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# CPack. Included after every install() rule has been declared - in src/cli,
# src/app and cmake/Notices.cmake - because the packages it makes are exactly
# those rules.
# ---------------------------------------------------------------------------
include(Packaging)

message(STATUS "WordJewelsSolver ${PROJECT_VERSION} for ${WJS_PLATFORM_DISPLAY_NAME}")
message(STATUS "  Build CLI   : ${WJS_BUILD_CLI}")
message(STATUS "  Build app   : ${WJS_BUILD_APP}")
message(STATUS "  Build tests : ${WJS_BUILD_TESTS}")
message(STATUS "  Tracy       : ${WJS_ENABLE_TRACY}")
message(STATUS "  Reviewed phonics data required : ${WJS_REQUIRE_REVIEWED_PHONICS}")
message(STATUS "  Build shake prototype : ${WJS_BUILD_SHAKE_PROTOTYPE}")
message(STATUS "  Package generators : ${WJS_PACKAGE_GENERATORS}")
if(WJS_BUILD_APP)
    message(STATUS "  Qt feedback modules : Multimedia=${WJS_QT_HAS_MULTIMEDIA}"
                   " Sensors=${WJS_QT_HAS_SENSORS} TextToSpeech=${WJS_QT_HAS_TEXTTOSPEECH}")
endif()
