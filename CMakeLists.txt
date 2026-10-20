# CMake 4 is required; the version range keeps policy behaviour current up to 4.4.
cmake_minimum_required(VERSION 3.28...4.4)

# ---------------------------------------------------------------------------
# The shared vcpkg checkout, verified before project() (cmake/Vcpkg.cmake): the
# toolchain file is found through the presets' CMAKE_TOOLCHAIN_FILE, and the
# checkout must contain the commit in cmake/vcpkg-commit.txt.
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Vcpkg.cmake")

# ---------------------------------------------------------------------------
# Select vcpkg manifest features BEFORE project() (the vcpkg toolchain reads
# VCPKG_MANIFEST_FEATURES when it is loaded at project() time). Qt is a
# prebuilt dependency found via CMAKE_PREFIX_PATH (see cmake/FindQt6.cmake),
# not built by vcpkg.
# ---------------------------------------------------------------------------
if(NOT DEFINED WJS_BUILD_TESTS OR WJS_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

# Tracy, and the one switch for it, before project(): the vcpkg manifest
# feature must be chosen before the toolchain reads the manifest at project()
# time. A Debug build is instrumented by default, so a regular development
# build can be connected to by the profiler without any option; every
# configuration a package can be made from (Release and the rest) is not,
# unless a person turns the option on. Multi-config generators have no build
# type at configure time and are opt-in: the coverage preset is that shape,
# and Tracy would distort coverage. cmake/Tracy.cmake makes the calls real
# from this one switch, and tests/profiling/check_presets.cmake holds the
# policy.
if(CMAKE_CONFIGURATION_TYPES)
    set(_wjs_tracy_default OFF)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_wjs_tracy_default ON)
else()
    set(_wjs_tracy_default OFF)
endif()
option(WJS_ENABLE_TRACY
    "Enable Tracy profiler instrumentation (on by default for a Debug build; never for a shipped build)"
    ${_wjs_tracy_default})
unset(_wjs_tracy_default)

if(WJS_ENABLE_TRACY)
    list(APPEND VCPKG_MANIFEST_FEATURES "profiling")
endif()

project(WordJewelsSolver
    VERSION 0.1.0
    DESCRIPTION "Finds every word on a Boggle board"
    HOMEPAGE_URL "https://github.com/iainchesworthlabs/wordjewelssolver"
    LANGUAGES CXX)

# What the build was built from, for the version block (cmake/GitProvenance.cmake).
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GitProvenance.cmake")

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

# One link at a time under Ninja. Every program's link ends in post-build steps
# that copy what it needs (the vcpkg DLLs, Qt's runtime, the sanitizer runtime)
# into that one directory, and two of them at once copy the same files over
# each other: "The process cannot access the file because it is being used by
# another process" failed a release build linking wjs_perf_tests and
# wjs_frame_timing_tests together. Compiling, which is where the time goes,
# stays parallel. Other generators ignore the pool.
set_property(GLOBAL APPEND PROPERTY JOB_POOLS wjs_link=1)
set(CMAKE_JOB_POOL_LINK wjs_link)

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
# The application is the product, but it is the one target that needs a
# prebuilt Qt 6.12 kit (cmake/FindQt6.cmake), which vcpkg does not build. AUTO
# builds it when such a kit is found and leaves it out when it is not, so one
# preset is useful both on a machine that only builds the solver and on a
# desktop with Qt; ON makes a missing Qt an error, and OFF never looks for it.
# The resolved value is ON or OFF from the Qt section below onwards. The
# application links Qt Quick 3D for the board scene, so the binary it makes is
# GPL-3.0 only (the author's decision of 2026-10-16; docs/design/board.md).
set(WJS_BUILD_APP "AUTO" CACHE STRING
    "Build the Qt Quick application: AUTO when Qt 6.12 is found, ON to require it, OFF to skip it")
set_property(CACHE WJS_BUILD_APP PROPERTY STRINGS AUTO ON OFF)
option(WJS_BUILD_TESTS "Build the unit, integration, system and UI test suites" ON)
# The harness around the 3D board scene (src/app/prototype/shake, and
# docs/design/shake-prototype.md): a window with a Shake button, a drag standing
# in for the accelerometer and the frame timing, over the scene module the
# application also links (WordJewelsSolver.Scene, GPL-3.0 only). A program of its
# own, not part of the application. Needs the application built
# (WJS_BUILD_APP=AUTO with a Qt 6.12 kit found, or ON).
option(WJS_BUILD_SHAKE_PROTOTYPE "Build the 3D board-scene prototype harness (needs Qt Quick 3D; GPL-3.0 only)" OFF)

# The review gate. Phonics data (data/phonics) is written and reviewed by a
# person qualified to do it, and a file says so in its header (reviewed-by and
# reviewed-on). A file without them is test data. With this on, the build runs
# tools/data/check_phonics_review.cmake before the library is built and
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
#
# WJS_BUILD_APP is resolved here first: AUTO looks for the kit and builds the
# application when it is there, ON requires it, OFF never looks. Every use
# after this point sees ON or OFF, whichever the person asked for.
# ---------------------------------------------------------------------------
string(TOUPPER "${WJS_BUILD_APP}" WJS_BUILD_APP_MODE)
if(WJS_BUILD_APP_MODE STREQUAL "AUTO")
    set(_wjs_qt_requirement QUIET)
elseif(WJS_BUILD_APP)
    set(_wjs_qt_requirement REQUIRED)
elseif(WJS_BUILD_APP_MODE MATCHES "^(OFF|FALSE|NO|N|0)$")
    set(_wjs_qt_requirement "")
else()
    message(FATAL_ERROR
        "WJS_BUILD_APP is '${WJS_BUILD_APP}'. It takes AUTO (build the "
        "application when Qt 6.12 is found), ON (require Qt) or OFF (never "
        "build the application).")
endif()

set(WJS_QT_COMPONENTS Core Gui Qml Quick QuickControls2 QuickLayouts QuickShapes Quick3D LinguistTools)
if(_wjs_qt_requirement)
    find_package(Qt6 6.12 ${_wjs_qt_requirement} COMPONENTS ${WJS_QT_COMPONENTS})
endif()

if(WJS_BUILD_APP_MODE STREQUAL "AUTO")
    if(Qt6_FOUND)
        set(WJS_BUILD_APP ON)
        message(STATUS
            "Qt ${Qt6_VERSION} found: the Qt Quick application is built "
            "(WJS_BUILD_APP=AUTO). Pass -DWJS_BUILD_APP=OFF to leave it out.")
    else()
        set(WJS_BUILD_APP OFF)
        message(STATUS
            "Qt 6.12 with the modules the application needs was not found: the "
            "Qt Quick application is not built (WJS_BUILD_APP=AUTO). Point "
            "QT_ROOT_DIR or CMAKE_PREFIX_PATH at a Qt 6.12 kit, or pass "
            "-DWJS_BUILD_APP=ON to make its absence an error.")
    endif()
elseif(WJS_BUILD_APP)
    set(WJS_BUILD_APP ON)
else()
    set(WJS_BUILD_APP OFF)
endif()
unset(_wjs_qt_requirement)

if(WJS_BUILD_APP)
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
        # Mode A's camera is a Qt Multimedia backend where the module is, and a
        # none backend otherwise (the variant pattern; src/app/camera). The
        # directories are chosen here, in the scope the tests share, because
        # tests/ compiles the same source and needs the backend's path.
        set(WJS_CAMERA_VARIANT_DIR
            "${PROJECT_SOURCE_DIR}/src/app/variants/camera-multimedia")
        set(WJS_CAMERA_PREVIEW_QML "${WJS_CAMERA_VARIANT_DIR}/CameraPreview.qml")
    else()
        set(WJS_QT_HAS_MULTIMEDIA OFF)
        set(WJS_CAMERA_VARIANT_DIR
            "${PROJECT_SOURCE_DIR}/src/app/variants/camera-none")
        set(WJS_CAMERA_PREVIEW_QML "${WJS_CAMERA_VARIANT_DIR}/CameraPreview.qml")
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

if(WJS_REQUIRE_REVIEWED_PHONICS)
    # A CMake script, so that a release build needs nothing but CMake: it holds
    # the review headers (tools/data/check_phonics_review.cmake); the lines of the
    # data are the `data.phonics` ctest case's (tools/data/check_phonics.ps1).
    # Always out of date, so every build looks at the data as it is now, not as
    # it was when CMake last ran. The library depends on it, so building any
    # target that links the library is gated.
    add_custom_target(wjs_phonics_review_gate ALL
        COMMAND "${CMAKE_COMMAND}"
            "-DROOT=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/data/check_phonics_review.cmake"
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
    message(WARNING
        "WJS_BUILD_SHAKE_PROTOTYPE needs WJS_BUILD_APP=ON and a Qt 6.12 kit; "
        "the prototype is not built.")
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

# ---------------------------------------------------------------------------
# How an IDE shows the target list. FOLDER is what Visual Studio's CMake
# Targets View (and the solution generators) group targets by; it changes
# nothing about what is built. The programs and the application's modules come
# first, then the libraries they are made of, then the test harnesses and the
# one-off tools, so opening this tree shows WordJewelsSolver rather than a flat
# list of wjs_* targets. Conditional targets are guarded by if(TARGET) because
# this runs once all the subdirectories have had their say.
# ---------------------------------------------------------------------------
function(wjs_set_target_folder folder)
    foreach(target IN LISTS ARGN)
        if(TARGET "${target}")
            set_target_properties("${target}" PROPERTIES FOLDER "${folder}")
        endif()
    endforeach()
endfunction()

wjs_set_target_folder("Application"
    WordJewelsSolver
    wjs_scene
    wjs_sceneplugin
    wjs_parent
    wjs_parentplugin
    wjs_scan_ui
    wjs_scan_uiplugin
    wjs_i18n
    wjs_trace
    wjs_feedback)
wjs_set_target_folder("Command line" wjs-cli)
wjs_set_target_folder("Libraries"
    wjs_lib
    wjs_platform
    wjs_scan
    wjs_dice_world
    wjs_tracy
    wjs_profiling
    wjs_warnings
    wjs_sanitizers
    wjs_coverage)
wjs_set_target_folder("Tests"
    wjs_unit_tests
    wjs_perf_tests
    wjs_scan_tests
    wjs_scan_synth
    wjs_app_tests
    wjs_shake_script_tests
    wjs_frame_timing_tests
    wjs_parent_preview
    wjs_scan_preview
    wjs_quick_tests
    wjs_quick_tests_support
    wjs_quick_test_scene
    wjs_quick_test_sceneplugin
    wjs_dice_tests)
wjs_set_target_folder("Tools" wjs_scan_render ShakePrototype)
wjs_set_target_folder("Packaging" "package-${PROJECT_NAME}")

message(STATUS "WordJewelsSolver ${PROJECT_VERSION} for ${WJS_PLATFORM_DISPLAY_NAME}")
message(STATUS "  Build CLI   : ${WJS_BUILD_CLI}")
message(STATUS "  Build app   : ${WJS_BUILD_APP} (requested ${WJS_BUILD_APP_MODE})")
message(STATUS "  Build tests : ${WJS_BUILD_TESTS}")
message(STATUS "  Tracy       : ${WJS_ENABLE_TRACY} (default: ON for a Debug build type)")
message(STATUS "  Reviewed phonics data required : ${WJS_REQUIRE_REVIEWED_PHONICS}")
message(STATUS "  Build shake prototype : ${WJS_BUILD_SHAKE_PROTOTYPE}")
message(STATUS "  Package generators : ${WJS_PACKAGE_GENERATORS}")
if(WJS_BUILD_APP)
    message(STATUS "  Qt feedback modules : Multimedia=${WJS_QT_HAS_MULTIMEDIA}"
                   " Sensors=${WJS_QT_HAS_SENSORS} TextToSpeech=${WJS_QT_HAS_TEXTTOSPEECH}")
endif()
