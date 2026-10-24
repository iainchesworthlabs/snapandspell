# The harness around the 3D board scene (docs/design/shake-prototype.md).
#
# The scene itself is the application's now: the QML module WordJewelsSolver.Scene
# in apps/app/src/scene (with its QML in apps/app/qml/scene), linked here as it is
# linked into WordJewelsSolver. This directory is only the window that drives it:
# a Shake button, the mouse drag that stands in for the accelerometer, the frame
# timing, and the library's dice roll (dice_roller.*), which gives the scene real
# dice to draw. Built only with -DWJS_BUILD_SHAKE_PROTOTYPE=ON (and
# WJS_BUILD_APP=ON, which finds Qt). It links Qt Quick 3D through the scene, so
# it is GPL-3.0 only, as the application now is (docs/design/board.md).

# The project's own dice (data/dice), embedded, so the boards the prototype rolls
# are the library's roll of the files the tests read, in the module's dice/
# directory (dice_roller.cpp).
set(WJS_SHAKE_DICE_FILES "")
foreach(side 3 4 5 6)
    set(dice_file "${PROJECT_SOURCE_DIR}/data/dice/${side}x${side}.txt")
    set_source_files_properties("${dice_file}" PROPERTIES
        QT_RESOURCE_ALIAS "dice/${side}x${side}.txt")
    list(APPEND WJS_SHAKE_DICE_FILES "${dice_file}")
endforeach()

qt_add_executable(ShakePrototype
    main.cpp)

qt_add_qml_module(ShakePrototype
    URI ShakePrototype
    VERSION 1.0
    IMPORT_PATH "${WJS_SCENE_IMPORT_DIR}"
    QML_FILES
        qml/ShakeWindow.qml
    RESOURCES
        ${WJS_SHAKE_DICE_FILES}
    SOURCES
        dice_roller.cpp
        dice_roller.hpp
        frame_stats.cpp
        frame_stats.hpp)

target_link_libraries(ShakePrototype
    PRIVATE
        wjs_sceneplugin
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Quick3D
        wjs::lib
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

# The scene's QML is in apps/app/qml/scene: that is where windeployqt has to look
# for the Qt Quick 3D modules the harness now shows through the scene.
wjs_platform_deploy_qt_runtime(ShakePrototype "${PROJECT_SOURCE_DIR}/apps/app/qml")
wjs_deploy_sanitizer_runtime(ShakePrototype)
