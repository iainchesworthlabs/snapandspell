# The camera, shared by Mode A's QML module and the tests: the Qt-facing
# CameraSource and the picture conversion. The backend is a variant chosen by
# CMake (apps/app/CMakeLists.txt):
#
#   sns/app/camera_backend.hpp   the interface (apps/app/src/camera)
#   variants/camera-multimedia   QCamera over Qt Multimedia, if the kit has it
#   variants/camera-none         there is no camera in this kit
#
# Only the variant's .cpp is compiled, so a kit without Qt Multimedia builds the
# same files without a #if anywhere (the variant pattern of
# apps/app/src/feedback/CMakeLists.txt).

set(SNS_CAMERA_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_backend.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_source.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/camera_source.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/frame_conversion.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/frame_conversion.hpp"
    "${SNS_CAMERA_VARIANT_DIR}/sns/app/camera_backend.cpp"
    PARENT_SCOPE)
