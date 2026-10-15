# macOS. Included by cmake/Platform.cmake; the contract it fills in is
# described at the top of that file.

set(WJS_PLATFORM_DISPLAY_NAME "macOS")
set(WJS_PLATFORM_DESKTOP ON)

set(WJS_PLATFORM_QT_SEARCH_ROOTS
    "$ENV{HOME}/Qt" "/opt/Qt" "/usr/local/opt/qt6" "/opt/homebrew/opt/qt6")
set(WJS_PLATFORM_QT_ARCH_DIRS macos clang_64)

set(WJS_PLATFORM_SOURCES
    macos/paths_macos.cpp
    posix/console_posix.cpp)

set(WJS_PLATFORM_LINK_LIBRARIES "")
set(WJS_PLATFORM_COMPILE_DEFINITIONS "")

# Nothing to deploy yet: the application bundle gets macdeployqt with the
# packaging work (docs/roadmap.md).
function(wjs_platform_deploy_qt_runtime target)
endfunction()
