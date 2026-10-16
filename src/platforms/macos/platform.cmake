# macOS. Included by cmake/Platform.cmake; the contract it fills in is
# described at the top of that file.

set(WJS_PLATFORM_DISPLAY_NAME "macOS")
set(WJS_PLATFORM_DESKTOP ON)

set(WJS_PLATFORM_QT_SEARCH_ROOTS
    "$ENV{HOME}/Qt" "/opt/Qt" "/usr/local/opt/qt6" "/opt/homebrew/opt/qt6")
set(WJS_PLATFORM_QT_ARCH_DIRS macos clang_64)

# The offscreen platform plugin takes its fonts from the system's.
set(WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT "")

# detail/ is what no platform differs in: no camera backend yet, and the portable
# board reader (docs/design/scanner.md).
set(WJS_PLATFORM_SOURCES
    detail/board_reader_portable.cpp
    detail/camera_unavailable.cpp
    macos/paths_macos.cpp
    posix/console_posix.cpp)

set(WJS_PLATFORM_LINK_LIBRARIES "")
set(WJS_PLATFORM_COMPILE_DEFINITIONS "")

# A graphical program on macOS is an application bundle.
function(wjs_platform_configure_app target)
    set_target_properties(${target} PROPERTIES MACOSX_BUNDLE ON)
endfunction()

# Nothing to deploy yet: the application bundle gets macdeployqt with the
# packaging work (docs/roadmap.md).
function(wjs_platform_deploy_qt_runtime target qml_dir)
endfunction()
