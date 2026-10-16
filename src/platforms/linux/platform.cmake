# Linux. Included by cmake/Platform.cmake; the contract it fills in is
# described at the top of that file.

set(WJS_PLATFORM_DISPLAY_NAME "Linux")
set(WJS_PLATFORM_DESKTOP ON)

set(WJS_PLATFORM_QT_SEARCH_ROOTS
    "$ENV{HOME}/Qt" "/opt/Qt" "/usr/lib/qt6" "/usr/lib/x86_64-linux-gnu/qt6")
set(WJS_PLATFORM_QT_ARCH_DIRS gcc_64)

# The offscreen platform plugin takes its fonts from fontconfig.
set(WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT "")

# detail/ is what no platform differs in: no camera backend yet, and the portable
# board reader (docs/design/scanner.md).
set(WJS_PLATFORM_SOURCES
    detail/board_reader_portable.cpp
    detail/camera_unavailable.cpp
    linux/paths_linux.cpp
    posix/console_posix.cpp)

set(WJS_PLATFORM_LINK_LIBRARIES "")
set(WJS_PLATFORM_COMPILE_DEFINITIONS "")

# An ordinary executable is already what a Linux desktop launches.
function(wjs_platform_configure_app target)
endfunction()

# Nothing to deploy: Qt comes from the system or from CMAKE_PREFIX_PATH and is
# found at run time the same way.
function(wjs_platform_deploy_qt_runtime target qml_dir)
endfunction()
