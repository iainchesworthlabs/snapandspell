# The test tree. Each directory owns its targets and registrations; this file
# holds what they share (Catch2, the variables below) and the order they are
# added in.
find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

# The tree's own root, for the directories that read a sibling's files (the
# application tests include the perf tier's budgets and the scanner's corpus;
# the repo checks run the scripts in the topic directories).
set(WJS_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Unit tier: wjs_unit_tests.
add_subdirectory(unit)

# The scanner: its library, the camera and board_reader interfaces, and the
# synthetic corpus that holds it to its accuracy target. Before the directories
# that link wjs_scan_synth.
add_subdirectory(scan)

# Performance tier: work budgets and a few relative timings (wjs_perf_tests).
add_subdirectory(perf)

# The shake prototype's dice simulation, when the prototype is built.
add_subdirectory(prototype)

if(WJS_BUILD_APP)
    # The application's model tier, the scene script, the window's frame budget
    # and the two stub shells the screens are checked in. Qt is only here when
    # the application is.
    add_subdirectory(app)

    # The interactive QML at run time: Qt Quick Test suites, one ctest entry
    # each, under the offscreen platform (docs/testing.md, "The interactive
    # QML"). Its own directory scope, not only for tidiness: a QML module's
    # generated qrc is named after its URI and written to the directory scope's
    # build directory, and the scan preview in tests/app builds a module of the
    # same URI (measured: "Module "WordJewelsSolver" contains no type named
    # "PreviewHost"" when the two shared a scope).
    #
    # The QML under test aliases the kit and Solo Play files under qml/kit/ and
    # qml/solo/ as plain resources, loaded by URL and never as module types, so
    # the extra directories need no qmldir: this is the policy's previous
    # behaviour. Set here, in the scope the directory inherits, as it is in
    # tests/app for the preview.
    qt_policy(SET QTP0004 OLD)
    add_subdirectory(qml)
endif()

# The system tier: the command-line solver, the window's start-up contract and
# the install rules, as programs.
add_subdirectory(system)

# The quality tier: checks about the tree, the build and the shipped data.
add_subdirectory(checks)
