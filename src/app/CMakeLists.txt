# Qt6 is found by the top-level CMakeLists.txt.

# AUTOMOC and friends, and the QML module plumbing.
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(wjs_app
    main.cpp)
set_target_properties(wjs_app PROPERTIES OUTPUT_NAME WordJewelsSolver)

# The kit's singletons: tokens, navigation and the sound and touch seam.
set_source_files_properties(
    qml/kit/Feedback.qml
    qml/kit/Navigation.qml
    qml/kit/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(wjs_app
    URI WordJewelsSolver
    VERSION 1.0
    QML_FILES
        qml/Home.qml
        qml/Main.qml
        qml/kit/AppButton.qml
        qml/kit/Feedback.qml
        qml/kit/HoldButton.qml
        qml/kit/Icon.qml
        qml/kit/Jewel.qml
        qml/kit/Navigation.qml
        qml/kit/Theme.qml
        qml/kit/TileFace.qml
        qml/scene/BoardScene.qml
        qml/scene/SceneDie.qml
        qml/scene/SceneScript.js
        qml/solo/BoardThumb.qml
        qml/solo/HistorySheet.qml
        qml/solo/PlayingPanel.qml
        qml/solo/SoloBox.qml
        qml/solo/SoloResting.qml
        qml/solo/SoloScreen.qml
        qml/solo/TraceLayer.qml
    SOURCES
        solo/board_layout.cpp
        solo/board_layout.hpp
        solo/fake_solo_game.cpp
        solo/fake_solo_game.hpp
        solo/path_tracer.cpp
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
        system_colour_scheme.hpp)

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

target_link_libraries(wjs_app
    PRIVATE
        wjs_parentplugin
        wjs::lib
        wjs::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

# What makes it a program of the platform's desktop, and its Qt runtime beside
# it: both are the selected platform's to say (src/platforms/<name>/).
wjs_platform_configure_app(wjs_app)
wjs_platform_deploy_qt_runtime(wjs_app "${CMAKE_CURRENT_SOURCE_DIR}/qml")
wjs_deploy_sanitizer_runtime(wjs_app)

# The default dictionary the model looks for: dictionaries/en.txt beside the
# executable (see SolverModel::loadDefaultDictionary).
add_custom_command(TARGET wjs_app POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:wjs_app>/dictionaries"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        "$<TARGET_FILE_DIR:wjs_app>/dictionaries/en.txt"
    COMMENT "Placing the English dictionary beside the application")

# The 3D board-scene prototype is a program of its own (it links Qt Quick 3D,
# which the application must not), behind its own option.
if(WJS_BUILD_SHAKE_PROTOTYPE)
    add_subdirectory(prototype/shake)
endif()
