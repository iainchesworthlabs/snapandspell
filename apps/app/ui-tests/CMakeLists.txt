# ---------------------------------------------------------------------------
# The interactive QML at run time: Qt Quick Test suites, one ctest entry each,
# under the offscreen platform (docs/testing.md, "The interactive QML"). What
# qmllint cannot say - that a press releases inside, that the gate opens on two
# spots held, that the board's split mirrors while its tiles do not - a suite
# says by driving the application's own screens.
#
# A directory of its own rather than a section of tests/CMakeLists.txt, and not
# only for tidiness: a QML module's generated qrc is named after its URI
# ("qmake_SnapAndSpell.qrc") and written to the directory scope's build
# directory. The scan preview's stub shell (tests/CMakeLists.txt) builds a
# module of the same URI, so in one directory the two would overwrite each
# other's qrc and the preview would load this module's qmldir - measured: the
# preview failed with "Module "SnapAndSpell" contains no type named
# "PreviewHost"". One directory down, each has its own.
#
# The QML under test is the application's own files, not a copy: the same
# QT_RESOURCE_ALIAS approach the two stub shells in apps/app/tests/unit use, because the
# application's QML module is compiled into its executable and cannot be linked.
# The kit, Home and Solo Play's screens are embedded here a third time (the scan
# shell already embeds the kit and BoardThumb), with the parent area's and the
# Family Assistant's static modules linked exactly as the previews link them.
# Main.qml itself is not here: it imports the parent, scan and scene modules and
# makes the window, and the suites test the screens it would show.
#
# What is test support rather than the application is the Scene module and the
# stub scene. SnapAndSpell.Scene is the profiling seam, a C++ singleton, so a
# static QML module of the same URI is built here from scene_stub (a Profiler that
# does nothing). The board scene is the application's own QML (qml/solo/BoardScene.qml)
# and is tested as it is (tst_scene.qml); the suites of the Solo Play screen give
# the screen StubBoardScene.qml instead, through its `sceneComponent`: a scene that
# draws nothing, keeps the same interface (prepare/show/play/pushMotion/settled/
# busy/boardRect/tileSize/tileGap) and records the board and seed it is handed, so
# a suite does not wait for a drop. apps/app/tests/unit/check_scene_contract.cmake
# holds the real scene and the stub to that interface.
#
# QT_QPA_PLATFORM is set in the harness's main() before the application is
# created as well as on every ctest entry, so running the binary by hand behaves
# headlessly too.
# ---------------------------------------------------------------------------
find_package(Qt6 6.12 REQUIRED COMPONENTS QuickTest)

qt_policy(SET QTP0001 NEW)

set(SNS_QUICK_TEST_SCENE_IMPORT "${CMAKE_CURRENT_BINARY_DIR}/quick-scene")
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/scene_stub/Profiler.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Profiler.qml" QT_QML_SINGLETON_TYPE TRUE)
# The library is made first so that AUTOMOC is on it before the QML module is:
# the module's type registration extracts metatypes at once, and without AUTOMOC
# the configure stops there.
qt_add_library(sns_quick_test_scene STATIC)
set_target_properties(sns_quick_test_scene PROPERTIES AUTOMOC ON)
qt_add_qml_module(sns_quick_test_scene
    URI SnapAndSpell.Scene
    VERSION 1.0
    OUTPUT_DIRECTORY "${SNS_QUICK_TEST_SCENE_IMPORT}/SnapAndSpell/Scene"
    QML_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/scene_stub/Profiler.qml")

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(sns_quick_test_scene)
target_link_libraries(sns_quick_test_scene PUBLIC Qt6::Qml Qt6::Quick)

# The models the screens bind to, and the fixture that hands them to a suite, in
# a library of its own rather than in the QML module's target. The Family
# Assistant's own models and their games come from its module (sns_scan_uiplugin,
# linked by the test executable below): compiling them here as well put two
# copies of their moc in the program once the fixture also used the module's
# CameraSource. The parent area's controller and the Solo Play models are
# compiled here, as the parent preview does.
add_library(sns_quick_tests_support STATIC
    "${PROJECT_SOURCE_DIR}/apps/app/tests/unit/parent_ui/demo_data.cpp"
    quick_fixture.cpp
    quick_fixture.hpp
    "${PROJECT_SOURCE_DIR}/apps/app/src/parent/parent_controller.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/fake_solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_game_factory.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_model.cpp")
set_target_properties(sns_quick_tests_support PROPERTIES AUTOMOC ON)
target_include_directories(sns_quick_tests_support
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/apps/app/src"
        "${PROJECT_SOURCE_DIR}/apps/app/src/camera"
        "${PROJECT_SOURCE_DIR}/apps/app/src/parent"
        "${PROJECT_SOURCE_DIR}/apps/app/src/scan"
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo"
        "${PROJECT_SOURCE_DIR}/apps/app/tests/unit/parent_ui")
target_link_libraries(sns_quick_tests_support
    PUBLIC
        sns_media_probe
        sns_test_fonts
        sns_i18n
        sns_trace
        sns::lib
        sns::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
    PRIVATE
        sns::warnings
        sns::sanitizers
        sns::coverage)

qt_add_executable(sns_quick_tests quicktest_main.cpp)
# A console program, so that the suites' output reaches ctest's log.
set_target_properties(sns_quick_tests PROPERTIES AUTOMOC ON WIN32_EXECUTABLE OFF)
target_compile_definitions(sns_quick_tests PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# The QML the modules above import from the application: the kit, Home and Solo
# Play's screens, each aliased to its own name in the module. The aliases are
# flat (AppButton.qml, not qml/kit/AppButton.qml) so no extra directory needs a
# qmldir; a type is named by its file's basename either way.
set(SNS_QUICK_TEST_QML_DIR "${PROJECT_SOURCE_DIR}/apps/app/qml")
set(SNS_QUICK_TEST_QML_SOURCES "")

# The kit, from the one list the application and the previews use.
include("${SNS_QUICK_TEST_QML_DIR}/kit/kit_files.cmake")
set(SNS_QUICK_TEST_KIT_FILES ${SNS_KIT_QML_FILES})
foreach(_file IN LISTS SNS_QUICK_TEST_KIT_FILES)
    set_source_files_properties("${SNS_QUICK_TEST_QML_DIR}/kit/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND SNS_QUICK_TEST_QML_SOURCES "${SNS_QUICK_TEST_QML_DIR}/kit/${_file}")
endforeach()

set(SNS_QUICK_TEST_SOLO_FILES
    BoardScene.qml
    BoardThumb.qml
    FlatBoard.qml
    HistorySheet.qml
    JewelsScreen.qml
    JewelsWording.qml
    PlayingPanel.qml
    SceneDie.qml
    SceneScript.js
    ScrollCue.qml
    SoloBox.qml
    SoloScreen.qml
    SoloWording.qml)
foreach(_file IN LISTS SNS_QUICK_TEST_SOLO_FILES)
    set_source_files_properties("${SNS_QUICK_TEST_QML_DIR}/solo/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND SNS_QUICK_TEST_QML_SOURCES "${SNS_QUICK_TEST_QML_DIR}/solo/${_file}")
endforeach()

foreach(_file IN ITEMS Home.qml HomeWording.qml)
    set_source_files_properties("${SNS_QUICK_TEST_QML_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND SNS_QUICK_TEST_QML_SOURCES "${SNS_QUICK_TEST_QML_DIR}/${_file}")
endforeach()

# The stub scene is test support: part of the test module, beside the screens that take it.
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/StubBoardScene.qml"
    PROPERTIES QT_RESOURCE_ALIAS "StubBoardScene.qml")
list(APPEND SNS_QUICK_TEST_QML_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/StubBoardScene.qml")

# The kit's and the screens' singletons are singletons in this module too: a
# pragma Singleton file is not one without this.
foreach(_file IN LISTS SNS_KIT_QML_SINGLETONS)
    set_source_files_properties("${SNS_QUICK_TEST_QML_DIR}/kit/${_file}"
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
endforeach()
set_source_files_properties(
    "${SNS_QUICK_TEST_QML_DIR}/solo/SoloWording.qml"
    "${SNS_QUICK_TEST_QML_DIR}/solo/JewelsWording.qml"
    "${SNS_QUICK_TEST_QML_DIR}/HomeWording.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(sns_quick_tests
    URI SnapAndSpell
    VERSION 1.0
    # A directory of this target's own: the scan preview embeds the same URI in
    # the build directory above.
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/quick-qml/SnapAndSpell"
    # Where qmlcachegen finds SnapAndSpell.Scene while compiling the
    # screens: the stub module's output above, not the application's 3D one.
    IMPORT_PATH "${SNS_QUICK_TEST_SCENE_IMPORT}"
    QML_FILES ${SNS_QUICK_TEST_QML_SOURCES}
    SOURCES
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo/board_layout.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_layout.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.cpp")

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(sns_quick_tests)

target_link_libraries(sns_quick_tests
    PRIVATE
        sns_quick_tests_support
        sns_parentplugin
        sns_scan_uiplugin
        sns_quick_test_sceneplugin
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickTest
        sns::warnings
        sns::sanitizers
        sns::coverage)

# The QML compiler's generated C++ and MSVC's C4702, as for the application's
# own QML target and the two stub shells.
get_target_property(_sns_quick_tests_sources sns_quick_tests SOURCES)
foreach(_sns_source IN LISTS _sns_quick_tests_sources)
    if(_sns_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_sns_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

sns_deploy_sanitizer_runtime(sns_quick_tests)
# The scan screen's live view imports QtMultimedia from the camera variant; the
# suites' own qml directory is not where that file is.
set(SNS_QUICK_TESTS_DEPLOY_QML_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
if(SNS_QT_HAS_MULTIMEDIA)
    list(APPEND SNS_QUICK_TESTS_DEPLOY_QML_DIRS "${SNS_CAMERA_VARIANT_DIR}")
endif()
sns_platform_deploy_qt_runtime(sns_quick_tests ${SNS_QUICK_TESTS_DEPLOY_QML_DIRS})
# One windeployqt at a time into the shared bin/ (the race the sibling projects'
# comments describe): this one deploys QtTest and the modules the suites import,
# after the scan shell's own deploy has finished.
add_dependencies(sns_quick_tests sns_scan_preview)

# One ctest entry per tst_*.qml file, as the house pattern has it: a failure
# names the suite, each suite gets its own process and its own models, and
# `ctest -R quick` is the whole tier. The label is `gui` for the house's sake;
# the platform plugin is offscreen and the environment carries what Qt's
# offscreen plugin needs on this platform (the font directory on Windows).
# QT_FORCE_STDERR_LOGGING makes QtTest's own output and every qWarning reach
# ctest's log rather than OutputDebugString.
file(GLOB SNS_QUICK_TEST_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tst_*.qml")
foreach(SNS_QUICK_TEST_FILE IN LISTS SNS_QUICK_TEST_FILES)
    get_filename_component(SNS_QUICK_TEST_NAME "${SNS_QUICK_TEST_FILE}" NAME_WE)
    string(REGEX REPLACE "^tst_" "" SNS_QUICK_TEST_SUITE "${SNS_QUICK_TEST_NAME}")
    add_test(NAME "quick.${SNS_QUICK_TEST_SUITE}"
        COMMAND sns_quick_tests -input "${SNS_QUICK_TEST_FILE}" -platform offscreen)
    # A size matrix makes every screen at every shape: on a hosted Windows runner that is minutes, not seconds.
    set(SNS_QUICK_TEST_TIMEOUT 300)
    if(SNS_QUICK_TEST_SUITE MATCHES "^sizematrix")
        set(SNS_QUICK_TEST_TIMEOUT 900)
    endif()
    set_tests_properties("quick.${SNS_QUICK_TEST_SUITE}" PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1;${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT ${SNS_QUICK_TEST_TIMEOUT}
        LABELS "system;ui;gui")
endforeach()

# A picture of every screen and dialog (capture_screens.qml): the QML the suites test, drawn by the
# offscreen platform and written as PNG files into <build>/screenshots (or SNS_SHOTS_DIR). Not a test and
# not part of `all`: `cmake --build --preset <preset> --target sns_screenshots` (docs/testing.md,
# "Screenshots").
add_custom_target(sns_screenshots
    COMMAND "${CMAKE_COMMAND}" -E env
        QT_QPA_PLATFORM=offscreen
        QT_FORCE_STDERR_LOGGING=1
        ${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}
        "SNS_SHOTS_DIR=${PROJECT_BINARY_DIR}/screenshots"
        $<TARGET_FILE:sns_quick_tests>
        -input "${CMAKE_CURRENT_SOURCE_DIR}/capture_screens.qml" -platform offscreen
    DEPENDS sns_quick_tests
    USES_TERMINAL
    COMMENT "Taking a picture of every screen")

# The design system's guard rail over the QML sources: no pixel size, colour or
# bare Text in a screen, no deleted names (apps/app/ui-tests/check_qml_literals.cmake),
# with an allow-list of the offences still in the tree that the screen streams
# shrink (apps/app/ui-tests/qml_lint_allowlist.cmake), and the check held to cases of its
# own. They read sources only and need no display, so they could run on a leg
# that does not build the application; they are here because the QML tier is.
add_test(NAME quick.lint
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_qml_literals.cmake")
add_test(NAME quick.lint.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/lint-selftest"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/test_check_qml_literals.cmake")
set_tests_properties(quick.lint quick.lint.selftest PROPERTIES LABELS "quality;ui")
