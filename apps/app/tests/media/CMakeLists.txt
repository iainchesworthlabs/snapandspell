# Whether Qt Multimedia has a backend here, for the tests that show the camera's live view
# (media_probe.hpp). The Qt Multimedia half is compiled only into a kit that has the module, the
# other is the answer of a kit without it; the library has the same header either way.
if(WJS_QT_HAS_MULTIMEDIA)
    set(WJS_MEDIA_PROBE_SOURCE media_probe_multimedia.cpp)
else()
    set(WJS_MEDIA_PROBE_SOURCE media_probe_none.cpp)
endif()

add_library(wjs_media_probe STATIC "${WJS_MEDIA_PROBE_SOURCE}" media_probe.hpp)
target_include_directories(wjs_media_probe PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(wjs_media_probe
    PUBLIC
        Qt6::Core
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
if(WJS_QT_HAS_MULTIMEDIA)
    target_link_libraries(wjs_media_probe PRIVATE Qt6::Multimedia)
endif()
