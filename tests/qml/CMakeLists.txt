# ---------------------------------------------------------------------------
# The interactive QML at run time: Qt Quick Test suites, one ctest entry each,
# under the offscreen platform (docs/testing.md, "The interactive QML"). What
# qmllint cannot say - that a press releases inside, that the gate opens on two
# spots held, that the board's split mirrors while its tiles do not - a suite
# says by driving the application's own screens.
#
# A directory of its own rather than a section of tests/CMakeLists.txt, and not
# only for tidiness: a QML module's generated qrc is named after its URI
# ("qmake_WordJewelsSolver.qrc") and written to the directory scope's build
# directory. The scan preview's stub shell (tests/CMakeLists.txt) builds a
# module of the same URI, so in one directory the two would overwrite each
# other's qrc and the preview would load this module's qmldir - measured: the
# preview failed with "Module "WordJewelsSolver" contains no type named
# "PreviewHost"". One directory down, each has its own.
#
# The QML under test is the application's own files, not a copy: the same
# QT_RESOURCE_ALIAS approach the two stub shells in tests/app use, because the
# application's QML module is compiled into its executable and cannot be linked.
# The kit, Home and Solo Play's screens are embedded here a third time (the scan
# shell already embeds the kit and BoardThumb), with the parent area's and the
# Family Assistant's static modules linked exactly as the previews link them.
# Main.qml itself is not here: it imports the parent, scan and scene modules and
# makes the window, and the suites test the screens it would show.
#
# What is test support rather than the application is the Scene module.
# WordJewelsSolver.Scene is the 3D scene and needs a display (and Qt Quick 3D),
# so a static QML module of the same URI is built here from scene_stub: a
# BoardScene that draws nothing, keeps the same interface (prepare/show/play/
# pushMotion/settled/busy/boardRect/tileSize/tileGap) and records the board and
# seed it is handed. tests/app/check_scene_contract.cmake already holds the two
# real BoardScenes to that interface; no suite needs the 3D one. The stub
# imports nothing from the application, so it can be built before the module
# that imports it.
#
# QT_QPA_PLATFORM is set in the harness's main() before the application is
# created as well as on every ctest entry, so running the binary by hand behaves
# headlessly too.
# ---------------------------------------------------------------------------
find_package(Qt6 6.12 REQUIRED COMPONENTS QuickTest)

qt_policy(SET QTP0001 NEW)

set(WJS_QUICK_TEST_SCENE_IMPORT "${CMAKE_CURRENT_BINARY_DIR}/quick-scene")
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/scene_stub/BoardScene.qml"
    PROPERTIES QT_RESOURCE_ALIAS "BoardScene.qml")
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/scene_stub/Profiler.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Profiler.qml" QT_QML_SINGLETON_TYPE TRUE)
# The library is made first so that AUTOMOC is on it before the QML module is:
# the module's type registration extracts metatypes at once, and without AUTOMOC
# the configure stops there.
qt_add_library(wjs_quick_test_scene STATIC)
set_target_properties(wjs_quick_test_scene PROPERTIES AUTOMOC ON)
qt_add_qml_module(wjs_quick_test_scene
    URI WordJewelsSolver.Scene
    VERSION 1.0
    OUTPUT_DIRECTORY "${WJS_QUICK_TEST_SCENE_IMPORT}/WordJewelsSolver/Scene"
    QML_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/scene_stub/BoardScene.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/scene_stub/Profiler.qml")
target_link_libraries(wjs_quick_test_scene PUBLIC Qt6::Qml Qt6::Quick)

# The models the screens bind to, and the fixture that hands them to a suite, in
# a library of its own rather than in the QML module's target. The Family
# Assistant's own models and their games come from its module (wjs_scan_uiplugin,
# linked by the test executable below): compiling them here as well put two
# copies of their moc in the program once the fixture also used the module's
# CameraSource. The parent area's controller and the Solo Play models are
# compiled here, as the parent preview does.
add_library(wjs_quick_tests_support STATIC
    "${PROJECT_SOURCE_DIR}/tests/app/parent_ui/demo_data.cpp"
    quick_fixture.cpp
    quick_fixture.hpp
    "${PROJECT_SOURCE_DIR}/src/app/parent/parent_controller.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/solo/fake_solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/solo/solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/solo/solo_game_factory.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/solo/solo_model.cpp")
set_target_properties(wjs_quick_tests_support PROPERTIES AUTOMOC ON)
target_include_directories(wjs_quick_tests_support
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/src/app"
        "${PROJECT_SOURCE_DIR}/src/app/camera"
        "${PROJECT_SOURCE_DIR}/src/app/parent"
        "${PROJECT_SOURCE_DIR}/src/app/scan"
        "${PROJECT_SOURCE_DIR}/src/app/solo"
        "${PROJECT_SOURCE_DIR}/tests/app/parent_ui")
target_link_libraries(wjs_quick_tests_support
    PUBLIC
        wjs_i18n
        wjs_trace
        wjs::lib
        wjs::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

qt_add_executable(wjs_quick_tests quicktest_main.cpp)
# A console program, so that the suites' output reaches ctest's log.
set_target_properties(wjs_quick_tests PROPERTIES AUTOMOC ON WIN32_EXECUTABLE OFF)
target_compile_definitions(wjs_quick_tests PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# The QML the modules above import from the application: the kit, Home and Solo
# Play's screens, each aliased to its own name in the module. The aliases are
# flat (AppButton.qml, not qml/kit/AppButton.qml) so no extra directory needs a
# qmldir; a type is named by its file's basename either way.
set(WJS_QUICK_TEST_QML_DIR "${PROJECT_SOURCE_DIR}/src/app/qml")
set(WJS_QUICK_TEST_QML_SOURCES "")

set(WJS_QUICK_TEST_KIT_FILES
    AppBar.qml
    AppButton.qml
    Feedback.qml
    HoldButton.qml
    Icon.qml
    Jewel.qml
    JewelBox.qml
    ModeCard.qml
    Navigation.qml
    Theme.qml
    TileFace.qml
    Wordmark.qml)
foreach(_file IN LISTS WJS_QUICK_TEST_KIT_FILES)
    set_source_files_properties("${WJS_QUICK_TEST_QML_DIR}/kit/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_QUICK_TEST_QML_SOURCES "${WJS_QUICK_TEST_QML_DIR}/kit/${_file}")
endforeach()

set(WJS_QUICK_TEST_SOLO_FILES
    BoardThumb.qml
    FlatBoard.qml
    HistorySheet.qml
    PlayingPanel.qml
    SoloBox.qml
    SoloResting.qml
    SoloScreen.qml
    SoloWording.qml
    TraceLayer.qml)
foreach(_file IN LISTS WJS_QUICK_TEST_SOLO_FILES)
    set_source_files_properties("${WJS_QUICK_TEST_QML_DIR}/solo/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_QUICK_TEST_QML_SOURCES "${WJS_QUICK_TEST_QML_DIR}/solo/${_file}")
endforeach()

foreach(_file IN ITEMS Home.qml HomeWording.qml)
    set_source_files_properties("${WJS_QUICK_TEST_QML_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_QUICK_TEST_QML_SOURCES "${WJS_QUICK_TEST_QML_DIR}/${_file}")
endforeach()

# The kit's and the screens' singletons are singletons in this module too: a
# pragma Singleton file is not one without this.
set_source_files_properties(
    "${WJS_QUICK_TEST_QML_DIR}/kit/Feedback.qml"
    "${WJS_QUICK_TEST_QML_DIR}/kit/Navigation.qml"
    "${WJS_QUICK_TEST_QML_DIR}/kit/Theme.qml"
    "${WJS_QUICK_TEST_QML_DIR}/solo/SoloWording.qml"
    "${WJS_QUICK_TEST_QML_DIR}/HomeWording.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(wjs_quick_tests
    URI WordJewelsSolver
    VERSION 1.0
    # A directory of this target's own: the scan preview embeds the same URI in
    # the build directory above.
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/quick-qml/WordJewelsSolver"
    # Where qmlcachegen finds WordJewelsSolver.Scene while compiling the
    # screens: the stub module's output above, not the application's 3D one.
    IMPORT_PATH "${WJS_QUICK_TEST_SCENE_IMPORT}"
    QML_FILES ${WJS_QUICK_TEST_QML_SOURCES}
    SOURCES
        "${PROJECT_SOURCE_DIR}/src/app/solo/board_layout.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_layout.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/system_colour_scheme.cpp")

target_link_libraries(wjs_quick_tests
    PRIVATE
        wjs_quick_tests_support
        wjs_parentplugin
        wjs_scan_uiplugin
        wjs_quick_test_sceneplugin
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickTest
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

# The QML compiler's generated C++ and MSVC's C4702, as for the application's
# own QML target and the two stub shells.
get_target_property(_wjs_quick_tests_sources wjs_quick_tests SOURCES)
foreach(_wjs_source IN LISTS _wjs_quick_tests_sources)
    if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_wjs_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

wjs_deploy_sanitizer_runtime(wjs_quick_tests)
# The scan screen's live view imports QtMultimedia from the camera variant; the
# suites' own qml directory is not where that file is.
set(WJS_QUICK_TESTS_DEPLOY_QML_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
if(WJS_QT_HAS_MULTIMEDIA)
    list(APPEND WJS_QUICK_TESTS_DEPLOY_QML_DIRS "${WJS_CAMERA_VARIANT_DIR}")
endif()
wjs_platform_deploy_qt_runtime(wjs_quick_tests ${WJS_QUICK_TESTS_DEPLOY_QML_DIRS})
# One windeployqt at a time into the shared bin/ (the race the sibling projects'
# comments describe): this one deploys QtTest and the modules the suites import,
# after the scan shell's own deploy has finished.
add_dependencies(wjs_quick_tests wjs_scan_preview)

# One ctest entry per tst_*.qml file, as the house pattern has it: a failure
# names the suite, each suite gets its own process and its own models, and
# `ctest -R quick` is the whole tier. The label is `gui` for the house's sake;
# the platform plugin is offscreen and the environment carries what Qt's
# offscreen plugin needs on this platform (the font directory on Windows).
# QT_FORCE_STDERR_LOGGING makes QtTest's own output and every qWarning reach
# ctest's log rather than OutputDebugString.
file(GLOB WJS_QUICK_TEST_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tst_*.qml")
foreach(WJS_QUICK_TEST_FILE IN LISTS WJS_QUICK_TEST_FILES)
    get_filename_component(WJS_QUICK_TEST_NAME "${WJS_QUICK_TEST_FILE}" NAME_WE)
    string(REGEX REPLACE "^tst_" "" WJS_QUICK_TEST_SUITE "${WJS_QUICK_TEST_NAME}")
    add_test(NAME "quick.${WJS_QUICK_TEST_SUITE}"
        COMMAND wjs_quick_tests -input "${WJS_QUICK_TEST_FILE}" -platform offscreen)
    set_tests_properties("quick.${WJS_QUICK_TEST_SUITE}" PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 300
        LABELS "gui")
endforeach()
