# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# Platform.cmake - the ONE place the target operating system is decided.
#
# This file maps the toolchain's own flags (WIN32, APPLE, UNIX) to a platform
# name, then includes that platform's fragment:
#
#     libs/platforms/<name>/platform.cmake
#
# Everything the platform contributes - its source files, system libraries,
# compile definitions and deployment steps - is declared in that fragment, and
# every other CMake file reads it through the variables and hooks below. No
# other CMake file tests the OS, and no source file tests it either
# (docs/platform-isolation.md; tools/checks/check_platform_isolation.ps1 holds
# the source side of that in CI).
#
# Adding a platform is therefore two edits: a libs/platforms/<name>/ directory
# (its implementations plus a platform.cmake that fills in the contract below),
# and one branch in the selector here.
#
# The contract a fragment fills in (checked by cmake/PlatformContract.cmake)
# ------------------------------------------------------------------------
# Facts:
#   SNS_PLATFORM_DISPLAY_NAME     How the platform is written in the version
#                                 block ("Windows", "macOS", "Linux", "iOS",
#                                 "Android", "Web").
#   SNS_PLATFORM_DESKTOP          ON where CPack installers exist (the three
#                                 desktops). OFF on iOS, whose application is
#                                 archived and exported by Xcode, and on
#                                 Android, whose package is made by Qt's
#                                 androiddeployqt, and on the web, whose bundle is
#                                 what Emscripten links, so no CPack configuration
#                                 is written.
#   SNS_PLATFORM_EXECUTABLE_FORMAT
#                                 The format of the platform's programs: elf
#                                 (Linux, Android), pe (Windows), mach-o
#                                 (macOS, iOS) or wasm (the web). The tests that
#                                 read a built program back with a format's own
#                                 tool are registered where it is elf.
#   SNS_PLATFORM_QT_SEARCH_ROOTS, SNS_PLATFORM_QT_ARCH_DIRS
#                                 Where cmake/FindQt6.cmake looks for an
#                                 aqtinstall-style Qt when none is on
#                                 CMAKE_PREFIX_PATH.
#   SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT
#                                 NAME=value entries a Qt Quick test needs
#                                 to draw with no display, besides
#                                 QT_QPA_PLATFORM=offscreen, which the tests
#                                 set themselves ("" if there are none): on
#                                 Windows the offscreen platform has no fonts
#                                 until it is told where the system's are.
#   SNS_PLATFORM_INSTALL_APP_DATA_DIR
#                                 Where the Qt application's data (the English
#                                 dictionary and the dice) is installed relative
#                                 to the install prefix: beside the program's
#                                 own executable, which is where it looks for
#                                 them. The bindir, except on a platform whose
#                                 application is a bundle (macOS, iOS) or a
#                                 package (Android).
#   SNS_PLATFORM_QT_DEPLOY_AT_INSTALL
#                                 ON where an installed copy of the application
#                                 carries Qt's runtime beside it, deployed by
#                                 Qt's own install-time script. OFF where the
#                                 system's Qt is loaded instead, and writing
#                                 that script's qt.conf would misdirect it.
#
# Build inputs (paths are relative to libs/platforms):
#   SNS_PLATFORM_SOURCES          Compiled into sns_platform: one
#                                 implementation of every interfaces/ header.
#   SNS_PLATFORM_LINK_LIBRARIES   System libraries sns_platform needs.
#   SNS_PLATFORM_COMPILE_DEFINITIONS
#                                 Compile definitions for sns_platform (the
#                                 only target that includes OS headers).
#
# Hooks (every platform defines all of them, even as no-ops):
#   sns_platform_configure_app(<target>)
#                                 What makes <target>, the Qt application, a
#                                 program of this platform: a GUI-subsystem
#                                 executable on Windows, an application bundle
#                                 on macOS, a signed-by-Xcode bundle on iOS, an
#                                 APK's native library and manifest on Android.
#   sns_platform_deploy_qt_runtime(<target> <qml_dir> [<more_qml_dirs>...])
#                                 Puts Qt's runtime next to the built program,
#                                 including the QML modules imported by the
#                                 QML sources in <qml_dir> ("" if it has none).
#                                 More directories may be named when a program's
#                                 screens also use QML that lives elsewhere (Mode
#                                 A's live view is a file of the camera variant);
#                                 a platform that does not read them ignores them.
#   sns_platform_configure_packaging()
#                                 Adds this platform's CPack generators to
#                                 CPACK_GENERATOR, whose first entry is always
#                                 ZIP, and says which were found. A generator
#                                 whose tool is missing (makensis, dpkg-deb,
#                                 rpmbuild) is left out rather than turning
#                                 into a failed configure; cmake/Packaging.cmake
#                                 calls this once.
# ---------------------------------------------------------------------------
include_guard(GLOBAL)

# A platform fragment says where the application's data is installed beside its
# executable, which is a bindir path, so GNUInstallDirs is included before the
# fragment is read.
include(GNUInstallDirs)

# EMSCRIPTEN first: Emscripten's toolchain file sets UNIX too, and a browser is
# not a Linux machine. IOS before APPLE: an iOS build is an Apple build too.
# ANDROID before UNIX: the NDK's toolchain file sets UNIX as well, and an
# Android build is not a Linux one.
if(EMSCRIPTEN)
    set(SNS_PLATFORM web)
elseif(IOS)
    set(SNS_PLATFORM ios)
elseif(ANDROID)
    set(SNS_PLATFORM android)
elseif(WIN32)
    set(SNS_PLATFORM windows)
elseif(APPLE)
    set(SNS_PLATFORM macos)
elseif(UNIX)
    set(SNS_PLATFORM linux)
else()
    message(FATAL_ERROR
        "Unsupported target platform. Add libs/platforms/<name>/ with a "
        "platform.cmake (see the contract at the top of cmake/Platform.cmake) "
        "and a branch for it in this file.")
endif()

set(SNS_PLATFORM_DIR "${PROJECT_SOURCE_DIR}/libs/platforms/${SNS_PLATFORM}")
include("${SNS_PLATFORM_DIR}/platform.cmake")

# What the version block says this binary was built for and with (the target's
# own name, word size and compiler). Named here, in the one file that tests the
# OS, so libs/core/CMakeLists.txt reads a plain variable.
set(SNS_VERSION_TARGET "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}, ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# A fragment that forgets part of the contract should fail here, by name, not
# as an empty source list or a missing-command error three files away.
include(PlatformContract)
sns_platform_check_contract("${SNS_PLATFORM_DIR}/platform.cmake"
    "${PROJECT_SOURCE_DIR}/libs/platforms")
