# Whether Qt Multimedia has a backend here, for the tests that show the camera's live view
# (media_probe.hpp). The Qt Multimedia half is compiled only into a kit that has the module, the
# other is the answer of a kit without it; the library has the same header either way.
if(SNS_QT_HAS_MULTIMEDIA)
    set(SNS_MEDIA_PROBE_SOURCE media_probe_multimedia.cpp)
else()
    set(SNS_MEDIA_PROBE_SOURCE media_probe_none.cpp)
endif()

add_library(sns_media_probe STATIC "${SNS_MEDIA_PROBE_SOURCE}" media_probe.hpp)
target_include_directories(sns_media_probe PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(sns_media_probe
    PUBLIC
        Qt6::Core
    PRIVATE
        sns::warnings
        sns::sanitizers
        sns::coverage)
if(SNS_QT_HAS_MULTIMEDIA)
    target_link_libraries(sns_media_probe PRIVATE Qt6::Multimedia)
endif()
