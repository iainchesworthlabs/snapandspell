# Linux. Included by cmake/Platform.cmake; the contract it fills in is
# described at the top of that file.

set(WJS_PLATFORM_DISPLAY_NAME "Linux")
set(WJS_PLATFORM_DESKTOP ON)

set(WJS_PLATFORM_QT_SEARCH_ROOTS
    "$ENV{HOME}/Qt" "/opt/Qt" "/usr/lib/qt6" "/usr/lib/x86_64-linux-gnu/qt6")
set(WJS_PLATFORM_QT_ARCH_DIRS gcc_64)

set(WJS_PLATFORM_SOURCES
    linux/paths_linux.cpp
    posix/console_posix.cpp)

set(WJS_PLATFORM_LINK_LIBRARIES "")
set(WJS_PLATFORM_COMPILE_DEFINITIONS "")

# Nothing to deploy: Qt comes from the system or from CMAKE_PREFIX_PATH and is
# found at run time the same way.
function(wjs_platform_deploy_qt_runtime target)
endfunction()
