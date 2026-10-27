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
if(NOT DEFINED SNS_BUILD_TESTS OR SNS_BUILD_TESTS)
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
    set(_sns_tracy_default OFF)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_sns_tracy_default ON)
else()
    set(_sns_tracy_default OFF)
endif()
option(SNS_ENABLE_TRACY
    "Enable Tracy profiler instrumentation (on by default for a Debug build; never for a shipped build)"
    ${_sns_tracy_default})
unset(_sns_tracy_default)

if(SNS_ENABLE_TRACY)
    list(APPEND VCPKG_MANIFEST_FEATURES "profiling")
endif()

project(SnapAndSpell
    VERSION 0.1.0
    DESCRIPTION "Finds every word on a Boggle board"
    HOMEPAGE_URL "https://github.com/iainchesworthlabs/snapandspell"
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
# another process" failed a release build linking sns_perf_tests and
# sns_frame_timing_tests together. Compiling, which is where the time goes,
# stays parallel. Other generators ignore the pool.
set_property(GLOBAL APPEND PROPERTY JOB_POOLS sns_link=1)
set(CMAKE_JOB_POOL_LINK sns_link)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# The one place the target OS is decided (cmake/Platform.cmake); every other
# CMake file reads what the platform's fragment declared.
include(Platform)
include(CompilerWarnings)
include(Hardening)
include(Sanitizers)
include(Coverage)
include(Tracy)
include(Fuzzing)
# The interface's languages (cmake/Translations.cmake): the lists lupdate scans
# and the catalogues lrelease compiles. The function itself is called by
# apps/app/src once Qt's Linguist tools have been found.
include(Translations)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(SNS_BUILD_CLI "Build the sns-cli command-line solver" ON)
# The application is the product, but it is the one target that needs a
# prebuilt Qt 6.12 kit (cmake/FindQt6.cmake), which vcpkg does not build. AUTO
# builds it when such a kit is found and leaves it out when it is not, so one
# preset is useful both on a machine that only builds the solver and on a
# desktop with Qt; ON makes a missing Qt an error, and OFF never looks for it.
# The resolved value is ON or OFF from the Qt section below onwards.
set(SNS_BUILD_APP "AUTO" CACHE STRING
    "Build the Qt Quick application: AUTO when Qt 6.12 is found, ON to require it, OFF to skip it")
set_property(CACHE SNS_BUILD_APP PROPERTY STRINGS AUTO ON OFF)
option(SNS_BUILD_TESTS "Build the unit, integration, system and UI test suites" ON)

# The review gate. Phonics data (data/phonics) is written and reviewed by a
# person qualified to do it, and a file says so in its header (reviewed-by and
# reviewed-on). A file without them is test data. With this on, the build runs
# tools/data/check_phonics_review.cmake before the library is built and
# stops if any file is test data, so a release cannot carry unreviewed content.
# On by default for the release build types, and for any multi-config generator
# (which has no build type at configure time, so it is the developer who says
# "not a release" by passing -DSNS_REQUIRE_REVIEWED_PHONICS=OFF).
if(CMAKE_CONFIGURATION_TYPES)
    set(SNS_REQUIRE_REVIEWED_PHONICS_DEFAULT ON)
elseif(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(SNS_REQUIRE_REVIEWED_PHONICS_DEFAULT ON)
else()
    set(SNS_REQUIRE_REVIEWED_PHONICS_DEFAULT OFF)
endif()
option(SNS_REQUIRE_REVIEWED_PHONICS
    "Refuse to build while data/phonics holds a file without a review header"
    ${SNS_REQUIRE_REVIEWED_PHONICS_DEFAULT})

# ---------------------------------------------------------------------------
# Qt, when the application is built. Found here, before any target and before
# the notices: the third-party notice names Qt's version (cmake/Notices.cmake),
# and imported targets are directory-scoped, so a find_package() at the top
# level is visible to tests/ and to every module under apps/app/src.
# LinguistTools is the catalogues' (lupdate/lrelease).
#
# SNS_BUILD_APP is resolved here first: AUTO looks for the kit and builds the
# application when it is there, ON requires it, OFF never looks. Every use
# after this point sees ON or OFF, whichever the person asked for.
# ---------------------------------------------------------------------------
string(TOUPPER "${SNS_BUILD_APP}" SNS_BUILD_APP_MODE)
if(SNS_BUILD_APP_MODE STREQUAL "AUTO")
    set(_sns_qt_requirement QUIET)
elseif(SNS_BUILD_APP)
    set(_sns_qt_requirement REQUIRED)
elseif(SNS_BUILD_APP_MODE MATCHES "^(OFF|FALSE|NO|N|0)$")
    set(_sns_qt_requirement "")
else()
    message(FATAL_ERROR
        "SNS_BUILD_APP is '${SNS_BUILD_APP}'. It takes AUTO (build the "
        "application when Qt 6.12 is found), ON (require Qt) or OFF (never "
        "build the application).")
endif()

set(SNS_QT_COMPONENTS Core Gui Qml Quick QuickControls2 QuickLayouts QuickShapes LinguistTools)
if(_sns_qt_requirement)
    find_package(Qt6 6.12 ${_sns_qt_requirement} COMPONENTS ${SNS_QT_COMPONENTS})
endif()

if(SNS_BUILD_APP_MODE STREQUAL "AUTO")
    if(Qt6_FOUND)
        set(SNS_BUILD_APP ON)
        message(STATUS
            "Qt ${Qt6_VERSION} found: the Qt Quick application is built "
            "(SNS_BUILD_APP=AUTO). Pass -DSNS_BUILD_APP=OFF to leave it out.")
    else()
        set(SNS_BUILD_APP OFF)
        message(STATUS
            "Qt 6.12 with the modules the application needs was not found: the "
            "Qt Quick application is not built (SNS_BUILD_APP=AUTO). Point "
            "QT_ROOT_DIR or CMAKE_PREFIX_PATH at a Qt 6.12 kit, or pass "
            "-DSNS_BUILD_APP=ON to make its absence an error.")
    endif()
elseif(SNS_BUILD_APP)
    set(SNS_BUILD_APP ON)
else()
    set(SNS_BUILD_APP OFF)
endif()
unset(_sns_qt_requirement)

if(SNS_BUILD_APP)
    # The feedback engines' modules are optional: the application's sound,
    # speech and motion have silent variants selected by CMake when a module is
    # absent (apps/app/src/feedback/CMakeLists.txt), so a contributor's kit without
    # them still configures and builds. The shared kit on the author's machine
    # has all three (docs/building.md, "The Qt Quick window").
    find_package(Qt6 6.12 QUIET OPTIONAL_COMPONENTS Multimedia Sensors TextToSpeech)
    set(SNS_QT_HAS_MULTIMEDIA OFF)
    set(SNS_QT_HAS_SENSORS OFF)
    set(SNS_QT_HAS_TEXTTOSPEECH OFF)
    if(TARGET Qt6::Multimedia)
        set(SNS_QT_HAS_MULTIMEDIA ON)
        # Mode A's camera is a Qt Multimedia backend where the module is, and a
        # none backend otherwise (the variant pattern; apps/app/src/camera). The
        # directories are chosen here, in the scope the tests share, because
        # tests/ compiles the same source and needs the backend's path.
        set(SNS_CAMERA_VARIANT_DIR
            "${PROJECT_SOURCE_DIR}/apps/app/src/variants/camera-multimedia")
        set(SNS_CAMERA_PREVIEW_QML "${SNS_CAMERA_VARIANT_DIR}/CameraPreview.qml")
    else()
        set(SNS_QT_HAS_MULTIMEDIA OFF)
        set(SNS_CAMERA_VARIANT_DIR
            "${PROJECT_SOURCE_DIR}/apps/app/src/variants/camera-none")
        set(SNS_CAMERA_PREVIEW_QML "${SNS_CAMERA_VARIANT_DIR}/CameraPreview.qml")
    endif()
    if(TARGET Qt6::Sensors)
        set(SNS_QT_HAS_SENSORS ON)
    endif()
    if(TARGET Qt6::TextToSpeech)
        set(SNS_QT_HAS_TEXTTOSPEECH ON)
    endif()
    # clang-cl under the Visual Studio generator (the coverage preset) does not
    # treat Qt's headers as system headers by itself (cmake/CompilerWarnings.cmake).
    sns_treat_qt_headers_as_system()
endif()

# ---------------------------------------------------------------------------
# The third-party notices this build carries (cmake/Notices.cmake): written once
# at configure time, installed beside the programs and embedded in the
# application as :/notices/NOTICES.txt, which the About screen shows. After Qt,
# whose version a build with the application names in them; before the targets,
# which refer to the generated file.
# ---------------------------------------------------------------------------
include(Notices)
sns_write_notices()

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
# The tests the libraries register themselves (the fuzz regression replays, libs/<lib>/fuzz) need
# CTest on before their directories are added; the suites in tests/ come last.
if(SNS_BUILD_TESTS OR SNS_ENABLE_FUZZING)
    enable_testing()
endif()

# Everything that differs between operating systems, behind the interfaces in
# libs/platforms/interfaces. Compiled from the sources the selected platform's
# platform.cmake names.
add_subdirectory(libs/platforms)

# The library holds ALL of the game logic and is completely GUI-free and
# platform-free.
add_subdirectory(libs/core)

# The scanner: finds a board in a camera frame and reads its letters. Its own
# library, so the solver library above stays free of image code, and with no
# dependency of its own to fetch.
add_subdirectory(libs/scan)

if(SNS_REQUIRE_REVIEWED_PHONICS)
    # A CMake script, so that a release build needs nothing but CMake: it holds
    # the review headers (tools/data/check_phonics_review.cmake); the lines of the
    # data are the `data.phonics` ctest case's (tools/data/check_phonics.ps1).
    # Always out of date, so every build looks at the data as it is now, not as
    # it was when CMake last ran. The library depends on it, so building any
    # target that links the library is gated.
    add_custom_target(sns_phonics_review_gate ALL
        COMMAND "${CMAKE_COMMAND}"
            "-DROOT=${CMAKE_CURRENT_SOURCE_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/data/check_phonics_review.cmake"
        COMMENT "Checking that the phonics data has been reviewed"
        VERBATIM)
    add_dependencies(sns_lib sns_phonics_review_gate)
endif()

if(SNS_BUILD_CLI)
    add_subdirectory(apps/cli)
endif()

# The application is a thin Qt Quick layer over the library.
if(SNS_BUILD_APP)
    add_subdirectory(apps/app)
endif()

if(SNS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# CPack. Included after every install() rule has been declared - in apps/cli,
# apps/app/src and cmake/Notices.cmake - because the packages it makes are exactly
# those rules.
# ---------------------------------------------------------------------------
include(Packaging)

# ---------------------------------------------------------------------------
# How an IDE shows the target list. FOLDER is what Visual Studio's CMake
# Targets View (and the solution generators) group targets by; it changes
# nothing about what is built. The programs and the application's modules come
# first, then the libraries they are made of, then the test harnesses and the
# one-off tools, so opening this tree shows SnapAndSpell rather than a flat
# list of sns_* targets. Conditional targets are guarded by if(TARGET) because
# this runs once all the subdirectories have had their say.
# ---------------------------------------------------------------------------
function(sns_set_target_folder folder)
    foreach(target IN LISTS ARGN)
        if(TARGET "${target}")
            set_target_properties("${target}" PROPERTIES FOLDER "${folder}")
        endif()
    endforeach()
endfunction()

sns_set_target_folder("Application"
    SnapAndSpell
    sns_scene
    sns_sceneplugin
    sns_parent
    sns_parentplugin
    sns_scan_ui
    sns_scan_uiplugin
    sns_i18n
    sns_trace
    sns_feedback)
sns_set_target_folder("Command line" sns-cli)
sns_set_target_folder("Libraries"
    sns_lib
    sns_platform
    sns_scan
    sns_tracy
    sns_profiling
    sns_warnings
    sns_sanitizers
    sns_coverage)
sns_set_target_folder("Tests"
    sns_unit_tests
    sns_perf_tests
    sns_scan_tests
    sns_scan_synth
    sns_app_tests
    sns_scene_script_tests
    sns_frame_timing_tests
    sns_parent_preview
    sns_scan_preview
    sns_quick_tests
    sns_quick_tests_support
    sns_quick_test_scene
    sns_quick_test_sceneplugin)
sns_set_target_folder("Tools" sns_scan_render)
sns_set_target_folder("Packaging" "package-${PROJECT_NAME}")

message(STATUS "SnapAndSpell ${PROJECT_VERSION} for ${SNS_PLATFORM_DISPLAY_NAME}")
message(STATUS "  Build CLI   : ${SNS_BUILD_CLI}")
message(STATUS "  Build app   : ${SNS_BUILD_APP} (requested ${SNS_BUILD_APP_MODE})")
message(STATUS "  Build tests : ${SNS_BUILD_TESTS}")
message(STATUS "  Tracy       : ${SNS_ENABLE_TRACY} (default: ON for a Debug build type)")
message(STATUS "  Reviewed phonics data required : ${SNS_REQUIRE_REVIEWED_PHONICS}")
message(STATUS "  Package generators : ${SNS_PACKAGE_GENERATORS}")
if(SNS_BUILD_APP)
    message(STATUS "  Qt feedback modules : Multimedia=${SNS_QT_HAS_MULTIMEDIA}"
                   " Sensors=${SNS_QT_HAS_SENSORS} TextToSpeech=${SNS_QT_HAS_TEXTTOSPEECH}")
endif()
