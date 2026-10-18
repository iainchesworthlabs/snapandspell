# Qt6 is found by the top-level CMakeLists.txt, which now finds Qt Quick 3D for
# the application too: the 3D board scene is part of it, so the binary this
# directory makes is GPL-3.0 only (the recorded decision, docs/design/board.md,
# "Rendered 3D, or drawn in 2D", and D2 in docs/roadmap.md).

# AUTOMOC and friends, and the QML module plumbing.
qt_standard_project_setup(REQUIRES 6.8)

# The interface's language: the catalogue installed on the application, the
# layout direction, and the eight codes the parent area offers
# (src/app/language_manager.hpp). A library of its own because the tests that
# load the screens in each language need it, and because the .qm files it
# carries are the application's only translated resources: lrelease compiles
# them from i18n/ at build time and they are embedded as :/i18n/wjs_<code>.qm.
add_library(wjs_i18n STATIC
    language_manager.cpp
    language_manager.hpp)
set_target_properties(wjs_i18n PROPERTIES AUTOMOC ON)
target_include_directories(wjs_i18n PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(wjs_i18n
    PUBLIC
        wjs::lib
        Qt6::Core
        Qt6::Gui
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
wjs_add_translations(wjs_i18n)

# The board scene: one QML module (`WordJewelsSolver.Scene`) with the 3D scene,
# the dice and the script, built in scene/. The application and the prototype
# harness both link it, so both show the same scene.
add_subdirectory(scene)
set(WJS_SCENE_IMPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/scene")

# The touch half of making a word (PathTracer), plain C++ and shared by the two
# modes' models: Solo Play's SoloModel and the Family Assistant's AssistModel.
# A library of its own so the scan module can use it without a second copy of the
# same object in one application.
add_library(wjs_trace STATIC
    solo/path_tracer.cpp)
target_include_directories(wjs_trace PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/solo")
target_compile_features(wjs_trace PUBLIC cxx_std_23)
target_link_libraries(wjs_trace
    PUBLIC
        wjs::lib
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

qt_add_executable(wjs_app
    main.cpp)
set_target_properties(wjs_app PROPERTIES OUTPUT_NAME WordJewelsSolver)

# The kit's singletons: tokens, navigation, the sound and touch seam, and the
# two wording files of the child-facing screens.
set_source_files_properties(
    qml/HomeWording.qml
    qml/kit/Feedback.qml
    qml/kit/Navigation.qml
    qml/kit/Theme.qml
    qml/solo/SoloWording.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Main.qml shows the parent area and the Family Assistant's screens, modules of
# their own that are built under parent/ and scan/ (added below), and SoloScreen
# uses the scene module built above: qmllint looks for them there.
qt_add_qml_module(wjs_app
    URI WordJewelsSolver
    VERSION 1.0
    IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/parent"
    IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/scan"
    IMPORT_PATH "${WJS_SCENE_IMPORT_DIR}"
    QML_FILES
        qml/Home.qml
        qml/HomeWording.qml
        qml/Main.qml
        qml/kit/AppButton.qml
        qml/kit/Feedback.qml
        qml/kit/HoldButton.qml
        qml/kit/Icon.qml
        qml/kit/Jewel.qml
        qml/kit/Navigation.qml
        qml/kit/Theme.qml
        qml/kit/TileFace.qml
        qml/solo/BoardThumb.qml
        qml/solo/HistorySheet.qml
        qml/solo/PlayingPanel.qml
        qml/solo/SoloBox.qml
        qml/solo/SoloResting.qml
        qml/solo/SoloScreen.qml
        qml/solo/SoloWording.qml
        qml/solo/TraceLayer.qml
    SOURCES
        frame_profiler.cpp
        frame_profiler.hpp
        solo/board_layout.cpp
        solo/board_layout.hpp
        solo/fake_solo_game.cpp
        solo/fake_solo_game.hpp
        solo/path_tracer.hpp
        solo/real_solo_game.cpp
        solo/real_solo_game.hpp
        solo/solo_game.cpp
        solo/solo_game.hpp
        solo/solo_game_factory.cpp
        solo/solo_game_factory.hpp
        solo/solo_game_setup.cpp
        solo/solo_game_setup.hpp
        solo/solo_layout.cpp
        solo/solo_layout.hpp
        solo/solo_model.cpp
        solo/solo_model.hpp
        solver_model.cpp
        solver_model.hpp
        system_colour_scheme.cpp
        system_colour_scheme.hpp
        system_motion.cpp
        system_motion.hpp)

# The QML compiler's generated C++ includes src/app/solo's headers by bare name
# (the type registrations), so that directory is on the include path.
target_include_directories(wjs_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/solo")

# The C++ the QML compiler writes for each QML file is built with the target's
# warnings as errors, and MSVC's code generator reports C4702 (unreachable code)
# from inside Qt's own headers while compiling it. /external:W0 does not silence
# a warning raised after the front end, so those files, and only those, are told
# to leave C4702 alone; the code written by hand keeps it.
get_target_property(_wjs_app_sources wjs_app SOURCES)
foreach(_wjs_source IN LISTS _wjs_app_sources)
    if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_wjs_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

# The parent area: the gate, the dashboard and the settings, a module of its own
# (src/app/parent) for the screen shell to show when a parent asks.
add_subdirectory(parent)

# Mode A, the Family Assistant: the scan and check screens and the assistant's
# board, a module of their own (src/app/scan) for the screen shell to show.
add_subdirectory(scan)

target_link_libraries(wjs_app
    PRIVATE
        wjs_parentplugin
        wjs_scan_uiplugin
        wjs_sceneplugin
        wjs_i18n
        wjs_trace
        wjs::lib
        wjs::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Quick3D
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

# What makes it a program of the platform's desktop, and its Qt runtime beside
# it: both are the selected platform's to say (src/platforms/<name>/).
wjs_platform_configure_app(wjs_app)
wjs_platform_deploy_qt_runtime(wjs_app "${CMAKE_CURRENT_SOURCE_DIR}/qml")
wjs_deploy_sanitizer_runtime(wjs_app)

# The default dictionary the model looks for, and the standard board's dice the
# game is played with: dictionaries/en.txt and dice/3x3.txt to 6x6.txt beside
# the executable (see SolverModel::loadDefaultDictionary and SoloGameSetup).
add_custom_command(TARGET wjs_app POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:wjs_app>/dictionaries"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        "$<TARGET_FILE_DIR:wjs_app>/dictionaries/en.txt"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:wjs_app>/dice"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/3x3.txt"
        "$<TARGET_FILE_DIR:wjs_app>/dice/3x3.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/4x4.txt"
        "$<TARGET_FILE_DIR:wjs_app>/dice/4x4.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/5x5.txt"
        "$<TARGET_FILE_DIR:wjs_app>/dice/5x5.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/6x6.txt"
        "$<TARGET_FILE_DIR:wjs_app>/dice/6x6.txt"
    COMMENT "Placing the English dictionary and the dice beside the application")

# ---------------------------------------------------------------------------
# The install rules (docs/building.md, "Packaging"). The installed program is
# the built one plus what it reads and the Qt runtime it loads; a package is
# this tree plus src/cli's, with the notices (cmake/Notices.cmake).
#
# The data goes where the program looks for it: beside its own executable,
# which is CMAKE_INSTALL_BINDIR for an ordinary executable and
# Contents/MacOS inside a bundle on macOS - the platform fragment declares it
# (WJS_PLATFORM_INSTALL_APP_DATA_DIR, cmake/Platform.cmake). The dictionary's
# rule here covers an application-only build and a bundle; when the
# command-line program is built too and both share the bindir, its own rule
# installs the one file and this one stays out of the way.
# ---------------------------------------------------------------------------
install(TARGETS wjs_app
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION ".")
install(FILES
    "${PROJECT_SOURCE_DIR}/data/dice/3x3.txt"
    "${PROJECT_SOURCE_DIR}/data/dice/4x4.txt"
    "${PROJECT_SOURCE_DIR}/data/dice/5x5.txt"
    "${PROJECT_SOURCE_DIR}/data/dice/6x6.txt"
    DESTINATION "${WJS_PLATFORM_INSTALL_APP_DATA_DIR}/dice")
if(NOT WJS_PLATFORM_INSTALL_APP_DATA_DIR STREQUAL CMAKE_INSTALL_BINDIR OR NOT WJS_BUILD_CLI)
    install(FILES "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        DESTINATION "${WJS_PLATFORM_INSTALL_APP_DATA_DIR}/dictionaries")
endif()

# The Qt runtime, plugins and QML modules of the INSTALLED program, with Qt's
# own QML-aware deploy script. It reads the built binary and scans the QML it
# imports, so the installed copy carries the Quick 3D plugin and the modules
# the scene and screens import, not a list kept here.
#
# Not the plain qt_generate_deploy_app_script(): that one deploys library
# dependencies only, and a package made with it has an empty qml/ directory
# (the sibling projects measured that). The QML-aware sibling is the one that
# runs qt6_deploy_qml_imports.
#
# NO_TRANSLATIONS: the interface's own catalogue is embedded in the program,
# and Qt's catalogues are neither installed nor merged (docs/licensing.md).
# INCLUDE_PLUGINS qoffscreen: the packaged program's headless smoke
# (tools/package/check_package.ps1, and CI) runs with QT_QPA_PLATFORM=offscreen,
# and windeployqt otherwise ships only the platform plugin the desktop uses.
# EXCLUDE_PLUGIN_TYPES qmltooling: the build-time deploy above says why - the
# GPL-3.0-only Quick 3D profiler plugin must not ship.
#
# Only where the installed program carries Qt (WJS_PLATFORM_QT_DEPLOY_AT_INSTALL):
# on Linux the system's Qt is used and a deploy script's qt.conf would break it
# (src/platforms/linux/platform.cmake says where that was measured).
# ---------------------------------------------------------------------------
if(WJS_PLATFORM_QT_DEPLOY_AT_INSTALL)
    qt_generate_deploy_qml_app_script(
        TARGET wjs_app
        OUTPUT_SCRIPT WJS_APP_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
        NO_TRANSLATIONS
        INCLUDE_PLUGINS qoffscreen
        EXCLUDE_PLUGIN_TYPES qmltooling)
    install(SCRIPT "${WJS_APP_DEPLOY_SCRIPT}")
endif()

# The third-party notices, as :/notices/NOTICES.txt, for the About screen's
# Licences view (cmake/Notices.cmake; the file itself is installed beside the
# programs by wjs_install_notices()).
wjs_add_notices_resource(wjs_app)

# The harness around the 3D board scene is a program of its own, behind its own
# option. It links the same scene module as the application (and so Qt Quick 3D,
# which the application now links too), and is for timing and stills.
if(WJS_BUILD_SHAKE_PROTOTYPE)
    add_subdirectory(prototype/shake)
endif()
