# ---------------------------------------------------------------------------
# Packaging.cmake - CPack. Included once, from the top-level CMakeLists.txt,
# after every target's install() rules have been declared: the packages it makes
# are exactly those rules (src/cli/CMakeLists.txt, src/app/CMakeLists.txt and
# cmake/Notices.cmake), so nothing here lists a file a second time.
#
# One package, holding both programs, deliberately. A package is the two
# programs and the files they read - wjs-cli is small, the window cannot run
# without the dictionary and the dice, and a person who downloads either wants
# the solver that finds the words. Splitting them into a component each would
# duplicate the dictionary and the notices, which is the only thing a
# per-component split would buy here. The library is not installed at all: it is
# a set of headers and a static archive for this tree's own programs, not a
# third-party SDK (docs/faq.md says the same).
#
# A ZIP is always offered: it needs no tool beyond CPack itself, on every host
# and as the fallback where a platform's own packager is not installed. The
# platform's native formats are added by the platform fragment's
# wjs_platform_configure_packaging() hook (src/platforms/<name>/platform.cmake):
# NSIS on Windows only when makensis is found, DragNDrop on macOS, and TGZ/DEB/
# RPM on Linux when their tools are there. A missing tool drops its generator
# and says so; it never fails the configure or the build, so `cpack -G ZIP`
# works on a machine that has nothing extra installed.
#
# Which generators `cpack` runs is CPACK_GENERATOR below. The package presets in
# CMakePresets.json deliberately carry no "generators" field: `cpack --preset`
# would pass that to cpack as -G and override this computed list, so the
# graceful degradation would apply to a bare `cpack` only. A -G on the command
# line still overrides it, which is what `cpack -G ZIP` in CI and docs uses.
#
# The generated packages land in build/<preset>/packages/ (CPACK_PACKAGE_DIRECTORY
# below, all generators), with CPack's staging tree in _CPack_Packages/ beside
# them.
# ---------------------------------------------------------------------------
include_guard(GLOBAL)

set(CPACK_PACKAGE_NAME "WordJewelsSolver")
set(CPACK_PACKAGE_VENDOR "Iain Chesworth")
set(CPACK_PACKAGE_CONTACT "Iain Chesworth")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "WordJewelsSolver")
# The GPL text: the installer shows it before installing, and
# cmake/Notices.cmake installs it as LICENSE.txt beside the programs (the
# application's own licence is GPL-3.0-only, which NOTICES.txt states).
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_VERBATIM_VARIABLES ON)
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

set(CPACK_GENERATOR "ZIP")

# The notice and the licence go beside the programs (cmake/Notices.cmake) on
# every platform this project can be installed on.
wjs_install_notices()

# A build with Tracy in it is a development build, and its programs load Tracy's
# client library at start-up: where Tracy is a shared library (vcpkg's dynamic
# Windows triplet) the package has to carry it beside them, or the packaged
# programs do not start (STATUS_DLL_NOT_FOUND). Only a development package is
# ever made this way (cmake/PackagePreflight.cmake); a Release build links no
# Tracy, and where Tracy is a static library there is nothing to install.
if(WJS_ENABLE_TRACY AND TARGET Tracy::TracyClient)
    get_target_property(_wjs_tracy_type Tracy::TracyClient TYPE)
    if(_wjs_tracy_type STREQUAL "SHARED_LIBRARY")
        install(IMPORTED_RUNTIME_ARTIFACTS Tracy::TracyClient
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
    unset(_wjs_tracy_type)
endif()

if(NOT WJS_PLATFORM_DESKTOP)
    message(STATUS
        "Packaging: ${WJS_PLATFORM_DISPLAY_NAME} is not a desktop this project "
        "packages; no CPack configuration is written.")
    return()
endif()

# The platform's own generators, if its tools are there (see the header).
wjs_platform_configure_packaging()

# A ZIP's contents are under one directory named after the package (CPack's
# default for the archive generators, which is left alone here - the DEB and RPM
# generators default the other way and must keep doing so, or the package would
# install into /WordJewelsSolver-.../). Unpacking the archive therefore makes
# one folder, `WordJewelsSolver-<version>-<system>/`, with bin/ inside it.

# ---------------------------------------------------------------------------
# NSIS, on Windows, when makensis was found. The two programs are installed
# side by side in the bindir, so the Start Menu entry for the window resolves
# to $INSTDIR\bin\WordJewelsSolver.exe (CPACK_NSIS_EXECUTABLES_DIRECTORY
# defaults to "bin", the same directory the install rules use). The console
# program gets no shortcut of its own: a .lnk to it would open a console, print
# the usage and close before anyone could read it.
# ---------------------------------------------------------------------------
if("NSIS" IN_LIST CPACK_GENERATOR)
    set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${PROJECT_VERSION}")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    if(TARGET WordJewelsSolver)
        set(CPACK_PACKAGE_EXECUTABLES "WordJewelsSolver" "WordJewelsSolver")
    endif()
endif()

# ---------------------------------------------------------------------------
# DEB, on Linux, when dpkg-deb and dpkg-shlibdeps were found. The package name
# is lower case, as Debian requires, and the file name is dpkg's own
# <name>_<version>_<arch>.deb form. dpkg-shlibdeps writes the shared-library
# dependencies it can resolve; Qt's QML modules are not libraries the loader
# sees, so a package that carries the application states them by hand - and
# this is the one part of the Linux packaging no leg of this project has built,
# because Qt is not installed on a Linux leg yet (docs/ci.md). The names are
# Debian/Ubuntu's, reasoned, not measured.
# ---------------------------------------------------------------------------
if("DEB" IN_LIST CPACK_GENERATOR)
    set(CPACK_DEBIAN_PACKAGE_NAME "wordjewelssolver")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    if(WJS_BUILD_APP)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS
            "libqt6core6 (>= 6.8), libqt6gui6 (>= 6.8), libqt6qml6 (>= 6.8), libqt6quick6 (>= 6.8), libqt6quickcontrols2-6 (>= 6.8), libqt6quick3d6 (>= 6.8), qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-templates, qml6-module-qtquick-window, qml6-module-qtqml-models, qml6-module-qtqml-workerscript, qml6-module-qtquick3d")
    endif()
endif()

# ---------------------------------------------------------------------------
# RPM, on Linux, when rpmbuild was found. The licence field names the
# application's GPL-3.0-only when the package carries the application (Qt Quick
# 3D is why), and the repository's GPL-3.0-or-later when it carries only
# wjs-cli. AUTOREQPROV writes the shared-library
# requirements it can resolve; the Qt modules the application needs are stated
# by hand for the same reason as the DEB block above, and with the same caveat:
# Fedora/RHEL's package names, reasoned, not measured on an RPM host.
# ---------------------------------------------------------------------------
if("RPM" IN_LIST CPACK_GENERATOR)
    if(WJS_BUILD_APP)
        set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-only")
    else()
        set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
    endif()
    set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
    set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    if(WJS_BUILD_APP)
        set(CPACK_RPM_PACKAGE_REQUIRES
            "qt6-qtbase >= 6.8, qt6-qtdeclarative >= 6.8, qt6-qtquick3d >= 6.8, qt6-qtquickcontrols2 >= 6.8")
    endif()
endif()

# ---------------------------------------------------------------------------
# DragNDrop, on macOS: the .app and the programs in a disk image with the usual
# Applications link. The application's dictionary and dice are inside the
# bundle (WJS_PLATFORM_INSTALL_APP_DATA_DIR, src/platforms/macos), because the
# program looks for them beside its own executable. No macOS host has built
# the application with Qt yet, so the image itself is unverified (docs/ci.md).
# ---------------------------------------------------------------------------
if("DragNDrop" IN_LIST CPACK_GENERATOR)
    set(CPACK_DMG_VOLUME_NAME "${CPACK_PACKAGE_NAME} ${PROJECT_VERSION}")
endif()

# Remembered before include(CPack): the module ends by pointing CPACK_GENERATOR
# at CPACK_SOURCE_GENERATOR for the source package it also configures, so after
# it returns CPACK_GENERATOR no longer names the binary package's generators.
# The generated CPackConfig.cmake has the right value either way; this is what
# the messages and the target below read.
set(WJS_PACKAGE_GENERATORS "${CPACK_GENERATOR}")

include(CPack)

# `cmake --build <build> --target package` runs cpack from the build tree with
# the configuration above, for an IDE or a one-word command; cpack itself and
# the presets are documented in docs/building.md.
add_custom_target(package-${PROJECT_NAME}
    COMMAND "${CMAKE_CPACK_COMMAND}" -C "$<CONFIG>" --config "${CPACK_OUTPUT_CONFIG_FILE}"
    COMMENT "Running CPack (${WJS_PACKAGE_GENERATORS}) in ${CPACK_PACKAGE_DIRECTORY}"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    VERBATIM)
set_target_properties(package-${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)

message(STATUS "Packaging: ${WJS_PACKAGE_GENERATORS} -> ${CPACK_PACKAGE_DIRECTORY}")
