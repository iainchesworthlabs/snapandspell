# The test tree. Each directory owns its targets and registrations; this file
# holds what they share (Catch2, the variables below) and the order they are
# added in.
find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

# The repository checks (system, quality) stay here, in the topic directories. The
# tests of a library or an application live beside it: libs/core/tests/unit,
# libs/core/bench (the performance tier), libs/scan/tests, apps/app/tests/unit,
# apps/app/tests/prototype and apps/app/ui-tests. This file adds them in the order
# they need (Catch2 and the variables below are shared), each with a build
# directory that mirrors its source directory.
set(WJS_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# A source archive (git archive) leaves libs/core/bench out (.gitattributes), and the
# application's tests include its budgets: that tree builds without tests.
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/core/bench/CMakeLists.txt")
    message(FATAL_ERROR
        "libs/core/bench is missing, as it is in a source archive. Configure with "
        "-DWJS_BUILD_TESTS=OFF to build the program without the test suites.")
endif()

# add_subdirectory for a test directory outside this one: the build directory
# mirrors the source directory. A macro, so that what a directory sets for its
# parent reaches this scope.
macro(wjs_add_tests relative)
    add_subdirectory("${PROJECT_SOURCE_DIR}/${relative}" "${PROJECT_BINARY_DIR}/${relative}")
endmacro()

# Unit tier: wjs_unit_tests.
wjs_add_tests(libs/core/tests/unit)

# The scanner: its library, the camera and board_reader interfaces, and the
# synthetic corpus that holds it to its accuracy target. Before the directories
# that link wjs_scan_synth.
wjs_add_tests(libs/scan/tests)

# Performance tier: work budgets and a few relative timings (wjs_perf_tests).
wjs_add_tests(libs/core/bench)

# The shake prototype's dice simulation, when the prototype is built.
wjs_add_tests(apps/app/tests/prototype)

if(WJS_BUILD_APP)
    # The application's model tier, the scene script, the window's frame budget
    # and the two stub shells the screens are checked in. Qt is only here when
    # the application is.
    #
    # Whether Qt Multimedia has a backend here, before the directories whose
    # camera cases skip without one.
    wjs_add_tests(apps/app/tests/media)
    # The house faces, for the test programs that start Qt Quick.
    wjs_add_tests(apps/app/tests/fonts)
    wjs_add_tests(apps/app/tests/unit)

    # The interactive QML at run time: Qt Quick Test suites, one ctest entry
    # each, under the offscreen platform (docs/testing.md, "The interactive
    # QML"). Its own directory scope, not only for tidiness: a QML module's
    # generated qrc is named after its URI and written to the directory scope's
    # build directory, and the scan preview in apps/app/tests/unit builds a module of the
    # same URI (measured: "Module "WordJewelsSolver" contains no type named
    # "PreviewHost"" when the two shared a scope).
    #
    # The QML under test aliases the kit and Solo Play files under qml/kit/ and
    # qml/solo/ into the module. Qt's policy QTP0004 (a qmldir for every such
    # directory) is NEW; its previous behaviour is deprecated. Set here, in the
    # scope the directory inherits, as it is in apps/app/tests/unit for the preview.
    qt_policy(SET QTP0004 NEW)
    wjs_add_tests(apps/app/ui-tests)
endif()

# The system tier: the command-line solver, the window's start-up contract and
# the install rules, as programs.
add_subdirectory(system)

# The quality tier: checks about the tree, the build and the shipped data.
add_subdirectory(checks)
