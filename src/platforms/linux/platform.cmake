# Linux. Included by cmake/Platform.cmake; the contract it fills in is
# described at the top of that file.

set(WJS_PLATFORM_DISPLAY_NAME "Linux")
set(WJS_PLATFORM_DESKTOP ON)

# The application looks for its dictionary and dice beside its own executable,
# and an installed executable is in the bindir.
set(WJS_PLATFORM_INSTALL_APP_DATA_DIR "${CMAKE_INSTALL_BINDIR}")

# Qt comes from the system, or from the CMAKE_PREFIX_PATH kit, and is found at
# run time the same way it was found at build time. Qt's install-time deploy
# script would write a qt.conf beside the installed program, which misdirects
# that system Qt's QML import path ("/usr/qml" rather than the multiarch one) and
# breaks QtQuick.Controls at startup; the DEB/RPM generators declare the
# runtime packages instead (cmake/Packaging.cmake).
set(WJS_PLATFORM_QT_DEPLOY_AT_INSTALL OFF)

set(WJS_PLATFORM_QT_SEARCH_ROOTS
    "$ENV{HOME}/Qt" "/opt/Qt" "/usr/lib/qt6" "/usr/lib/x86_64-linux-gnu/qt6")
set(WJS_PLATFORM_QT_ARCH_DIRS gcc_64)

# The offscreen platform plugin takes its fonts from fontconfig.
set(WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT "")

# detail/ is what no platform differs in: no camera backend yet, the portable
# board reader (docs/design/scanner.md), and the no-op haptics every desktop
# answers with (src/platforms/interfaces/wjs/platform/haptics.hpp).
set(WJS_PLATFORM_SOURCES
    detail/board_reader_portable.cpp
    detail/camera_unavailable.cpp
    detail/haptics_none.cpp
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

# A tarball needs no tool installed. The distribution packages are offered only
# when the tool that writes one is there, so a machine without dpkg-dev or rpm
# still gets a ZIP and a TGZ rather than a failed configure (docs/building.md,
# "Packaging"). dpkg-shlibdeps is asked for beside dpkg-deb because the DEB
# generator is configured to work out the shared-library dependencies, and
# without it the packaging would stop half way.
function(wjs_platform_configure_packaging)
    list(APPEND CPACK_GENERATOR "TGZ")
    find_program(WJS_DPKG_DEB_EXECUTABLE NAMES dpkg-deb)
    find_program(WJS_DPKG_SHLIBDEPS_EXECUTABLE NAMES dpkg-shlibdeps)
    if(WJS_DPKG_DEB_EXECUTABLE AND WJS_DPKG_SHLIBDEPS_EXECUTABLE)
        list(APPEND CPACK_GENERATOR "DEB")
    endif()
    find_program(WJS_RPMBUILD_EXECUTABLE NAMES rpmbuild)
    if(WJS_RPMBUILD_EXECUTABLE)
        list(APPEND CPACK_GENERATOR "RPM")
    endif()
    message(STATUS "Packaging: ${CPACK_GENERATOR}")
    set(CPACK_GENERATOR "${CPACK_GENERATOR}" PARENT_SCOPE)
endfunction()
