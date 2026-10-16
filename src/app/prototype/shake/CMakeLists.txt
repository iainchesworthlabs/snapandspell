# The 3D board-scene prototype (docs/design/shake-prototype.md).
#
# A program of its own, not part of the application: it links Qt Quick 3D, which
# Qt offers under the GPL-3.0 only, so this program is GPL-3.0 only
# (docs/design/board.md, "Rendered 3D, or drawn in 2D"). Built only with
# -DWJS_BUILD_SHAKE_PROTOTYPE=ON (and WJS_BUILD_APP=ON, which finds Qt).
#
# The scene is qml/BoardScene.qml and is plain QML over Qt Quick 3D. Everything
# else here is the harness around it: a window with a Shake button, the mouse
# drag that stands in for the accelerometer, the frame timing, and the library's
# dice roll (dice_roller.*), which gives the scene real dice to draw.

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

qt_add_executable(wjs_shake_prototype
    main.cpp)
set_target_properties(wjs_shake_prototype PROPERTIES OUTPUT_NAME ShakePrototype)

qt_add_qml_module(wjs_shake_prototype
    URI ShakePrototype
    VERSION 1.0
    QML_FILES
        qml/BoardScene.qml
        qml/Die.qml
        qml/FaceAtlas.qml
        qml/ShakeScript.js
        qml/ShakeWindow.qml
    RESOURCES
        qml/lightprobe/sky.ktx
        ${WJS_SHAKE_DICE_FILES}
    SOURCES
        dice_roller.cpp
        dice_roller.hpp
        frame_stats.cpp
        frame_stats.hpp)

target_link_libraries(wjs_shake_prototype
    PRIVATE
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

wjs_platform_deploy_qt_runtime(wjs_shake_prototype "${CMAKE_CURRENT_SOURCE_DIR}/qml")
wjs_deploy_sanitizer_runtime(wjs_shake_prototype)
