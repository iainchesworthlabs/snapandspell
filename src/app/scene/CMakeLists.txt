# The board scene (docs/design/board.md, "The shake and the drop"), a QML module
# of its own: the 3D shake, the dice, the atlas and the script the two draw from.
# It is imported as
#
#   import WordJewelsSolver.Scene
#   BoardScene { ... }
#
# by the application (`qml/solo/SoloScreen.qml`) and by the prototype harness
# (`prototype/shake/qml/ShakeWindow.qml`). One module, so one BoardScene, one
# ShakeScript.js and one set of dice types: the harness measures the same scene
# the application plays, and there is no copy of either.
#
# It links Qt Quick 3D, which Qt offers under the GPL-3.0 only: every program
# that links this module is GPL-3.0 only (docs/design/board.md, "Rendered 3D, or
# drawn in 2D", and docs/design/shake-prototype.md).

# The application's QML module sets these for its own target; a module compiled
# in a directory of its own sets them again (as the parent area's does).
set(CMAKE_AUTOMOC ON)
qt_policy(SET QTP0001 NEW)

# The dice's plain-C++ simulation (no Qt, no renderer), behind the QML object the
# script drives.
add_subdirectory(sim)

# The QML is in src/app/qml/scene, beside the application's other modules; each
# file is given the name it has in this module.
set(WJS_SCENE_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../qml/scene")
set(WJS_SCENE_QML_NAMES
    BoardScene.qml
    Die.qml
    FaceAtlas.qml
    ShakeScript.js)
set(WJS_SCENE_QML_SOURCES "")
foreach(_file IN LISTS WJS_SCENE_QML_NAMES)
    set_source_files_properties("${WJS_SCENE_QML_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_SCENE_QML_SOURCES "${WJS_SCENE_QML_DIR}/${_file}")
endforeach()

# The light probe is read by BoardScene.qml as "lightprobe/sky.ktx", relative to
# the scene's own resource directory.
set_source_files_properties("${WJS_SCENE_QML_DIR}/lightprobe/sky.ktx"
    PROPERTIES QT_RESOURCE_ALIAS "lightprobe/sky.ktx")

qt_add_qml_module(wjs_scene
    STATIC
    URI WordJewelsSolver.Scene
    VERSION 1.0
    # No ENABLE_TYPE_COMPILER here: the dice build on QQuick3DGeometry, a C++
    # base type qmltc cannot see in its bare mode. The scene's first cost is the
    # shaders, which the persistent shader caches and the deferred loader in
    # SoloScreen cover; its scripts are still bytecode-compiled by qmlcachegen.
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/WordJewelsSolver/Scene"
    QML_FILES ${WJS_SCENE_QML_SOURCES}
    RESOURCES
        "${WJS_SCENE_QML_DIR}/lightprobe/sky.ktx"
    SOURCES
        dice_sim.cpp
        dice_sim.hpp
        die_geometry.cpp
        die_geometry.hpp
        profiler.cpp
        profiler.hpp)

target_include_directories(wjs_scene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# The C++ the QML compiler writes is built with the target's warnings as errors,
# and MSVC's code generator reports C4702 (unreachable code) from inside Qt's own
# headers while compiling it. The application's own QML target does the same for
# its generated files; this module's are told to leave C4702 alone too, and only
# those.
get_target_property(_wjs_scene_sources wjs_scene SOURCES)
foreach(_wjs_source IN LISTS _wjs_scene_sources)
    if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_wjs_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

target_link_libraries(wjs_scene
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Quick3D
        wjs_dice_world
    PRIVATE
        # The wrapper (wjs/profiling.hpp) and, in a profiling build, Tracy: this
        # module does not link the library, so it carries the interface itself.
        wjs::profiling
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
