# Windows. Included by cmake/Platform.cmake; the contract it fills in is
# described at the top of that file.

set(WJS_PLATFORM_DISPLAY_NAME "Windows")
set(WJS_PLATFORM_DESKTOP ON)

set(WJS_PLATFORM_QT_SEARCH_ROOTS "C:/Qt" "$ENV{USERPROFILE}/Qt")
set(WJS_PLATFORM_QT_ARCH_DIRS msvc2022_64 msvc2019_64)

set(WJS_PLATFORM_SOURCES
    windows/console_windows.cpp
    windows/paths_windows.cpp)

set(WJS_PLATFORM_LINK_LIBRARIES "")

# <windows.h> without the parts and macros nobody asked for (min/max, the
# rarely used APIs) and with the wide-character API as the default. Passed by
# the build, so no source file declares them.
set(WJS_PLATFORM_COMPILE_DEFINITIONS WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

# Qt's DLLs are not next to the executable or on PATH by default, so it fails
# to start (STATUS_DLL_NOT_FOUND) unless launched from a shell that already has
# Qt's bin directory on PATH. windeployqt copies what the executable needs (Qt
# DLLs, platform and style plugins) next to it after every build, so it runs
# standalone however it is launched.
function(wjs_platform_deploy_qt_runtime target)
    find_program(WJS_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt6 windeployqt
        HINTS "${Qt6_DIR}/../../../bin")
    if(NOT WJS_WINDEPLOYQT_EXECUTABLE)
        message(WARNING
            "windeployqt was not found; ${target} will only start from a shell "
            "that has Qt's bin directory on PATH.")
        return()
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${WJS_WINDEPLOYQT_EXECUTABLE}" --no-translations "$<TARGET_FILE:${target}>"
        COMMENT "Deploying the Qt runtime beside ${target}")
endfunction()
