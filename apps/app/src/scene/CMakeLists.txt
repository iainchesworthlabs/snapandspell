# The profiling seam: a QML module of its own (`SnapAndSpell.Scene`) that holds the Profiler
# singleton (docs/profiling.md, "QML and the scene"), imported by the application and by the parts
# that open Tracy zones from QML:
#
#   import SnapAndSpell.Scene
#   Profiler.begin("ui.trace")
#
# The board scene itself is not here. It is the application's own QML (apps/app/qml/solo/BoardScene.qml):
# it draws with the kit, which a module built before the application cannot import, and it is plain
# 2D, so it needs nothing of Qt but Qt Quick (docs/design/board.md, "The shake and the drop").

# The application's QML module sets these for its own target; a module compiled
# in a directory of its own sets them again (as the parent area's does).
set(CMAKE_AUTOMOC ON)
qt_policy(SET QTP0001 NEW)

qt_add_qml_module(sns_scene
    STATIC
    URI SnapAndSpell.Scene
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/SnapAndSpell/Scene"
    SOURCES
        profiler.cpp
        profiler.hpp)

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(sns_scene)

target_include_directories(sns_scene PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# The C++ the QML compiler writes is built with the target's warnings as errors,
# and MSVC's code generator reports C4702 (unreachable code) from inside Qt's own
# headers while compiling it. The application's own QML target does the same for
# its generated files; this module's are told to leave C4702 alone too, and only
# those.
get_target_property(_sns_scene_sources sns_scene SOURCES)
foreach(_sns_source IN LISTS _sns_scene_sources)
    if(_sns_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_sns_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

target_link_libraries(sns_scene
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    PRIVATE
        # The wrapper (sns/profiling.hpp) and, in a profiling build, Tracy: this
        # module does not link the library, so it carries the interface itself.
        sns::profiling
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)
