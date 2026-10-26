# SPDX-License-Identifier: GPL-3.0-or-later
# The web: the application built to WebAssembly with Emscripten and Qt for WebAssembly, and run in
# a browser. Included by cmake/Platform.cmake; the contract it fills in is described at the top of
# that file.
#
# What a browser has and does not have decides most of this file:
#
#   * One thread. The single-threaded Qt kit is used (wasm_singlethread). The multithreaded kit
#     needs SharedArrayBuffer, which a browser grants only to a page served with the
#     Cross-Origin-Opener-Policy and Cross-Origin-Embedder-Policy headers, and GitHub Pages
#     cannot send headers. The game has no thread of its own (apps/app/src/scan/scan_model.hpp's
#     pool belongs to the camera path, which has no camera here), so nothing is lost.
#   * No system fonts. Every face the interface uses is bundled in the program's resources
#     (assets/fonts, registered by main.cpp). Hebrew, Chinese and Japanese need the Noto subsets
#     of the fonts work (docs/design/toybox/DESIGN.md, 12.4) and show a missing-glyph box until
#     those are in; with Qt's default local-fonts setting a browser that has such a font may
#     supply it.
#   * No Qt Multimedia, Qt Sensors or Qt Speech in the kit, so the variants the application
#     already has for a kit without them are the ones built: no sound, no spoken word, no
#     accelerometer shake (the Shake button, Space and a drag are the ways in, as on a desktop)
#     and no camera (the Family Assistant's scan screens say so and offer typing the board).
#     docs/ci.md, "Web", lists them as what the web has not got yet.
#   * No installed data directory. The English dictionary and the dice are compiled into the
#     program's resources and laid down in the in-memory file system at start (paths_web.cpp), as
#     the Android build does. What the person keeps (the one store file) lives under /persist,
#     which sns-boot.js mounts as IndexedDB before the program starts, so progress survives a
#     reload in the same browser profile and is gone with that profile's site data.
#   * The board scene is the drawn 2D one on every platform (Qt Quick alone), so the web
#     program loads no 3D pipeline and no shaders.
#
# Needs the Qt for WebAssembly kit (QT_WEB_DIR / the `web-release` preset), a desktop Qt of the
# same version for the host tools (QT_HOST_PATH) and the Emscripten SDK the kit was built with
# (EMSDK; Qt 6.12 wants 5.0.5), docs/building.md, "Web". The bundle is the directory
# <build>/web-bundle: serve it over HTTP or HTTPS and open index.html.

set(SNS_PLATFORM_DISPLAY_NAME "Web")

# No installers and no command-line program: the bundle is a directory of static files.
set(SNS_PLATFORM_DESKTOP OFF)
set(SNS_PLATFORM_EXECUTABLE_FORMAT "wasm")

# A web build is never `cmake --install`ed; this is the name an install rule would use, and
# the name of the resource prefix paths_web.cpp unpacks.
set(SNS_PLATFORM_INSTALL_APP_DATA_DIR "assets")

# Qt is linked statically into the .wasm; there is nothing for an install-time deploy to copy.
set(SNS_PLATFORM_QT_DEPLOY_AT_INSTALL OFF)

# Qt for WebAssembly when no QT_ROOT_DIR names it (the aqtinstall layout:
# Qt/6.12.0/wasm_singlethread).
set(SNS_PLATFORM_QT_SEARCH_ROOTS "$ENV{HOME}/Qt" "/opt/Qt")
set(SNS_PLATFORM_QT_ARCH_DIRS wasm_singlethread)

# No test runs in a browser here.
set(SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT "")

# A browser has no desktop Qt to build against: the application is the whole build.
string(TOUPPER "${SNS_BUILD_APP}" _sns_web_build_app)
if(_sns_web_build_app MATCHES "^(OFF|FALSE|NO|N|0)$")
    message(FATAL_ERROR
        "A web build is the Qt application: SNS_BUILD_APP=${SNS_BUILD_APP} leaves nothing to "
        "bundle. Use the web-release preset (docs/building.md, \"Web\").")
endif()
unset(_sns_web_build_app)

# detail/ is what no platform differs in: the portable board reader, no camera backend (Qt
# Multimedia is not in the kit; the scan screens say so), no window chrome. The paths, the
# vibration (navigator.vibrate) and the fatal error are the browser's own; the console is the
# POSIX one (Emscripten's libc is, and there is nothing to switch on).
set(SNS_PLATFORM_SOURCES
    detail/board_reader_portable.cpp
    detail/camera_unavailable.cpp
    detail/window_chrome_none.cpp
    posix/console_posix.cpp
    web/fatal_error_web.cpp
    web/haptics_web.cpp
    web/paths_web.cpp)

# paths_web.cpp reads the unpacked data out of Qt's resource system. Emscripten's own JS
# library calls (EM_ASM) need no library to link.
set(SNS_PLATFORM_LINK_LIBRARIES Qt6::Core)
set(SNS_PLATFORM_COMPILE_DEFINITIONS "")

# Where the bundle's pages and scripts are, and where the bundle is made (captured here: inside a
# function CMAKE_CURRENT_LIST_DIR is the caller's).
set(SNS_WEB_PACKAGING_DIR "${CMAKE_CURRENT_LIST_DIR}/packaging")
set(SNS_WEB_BUNDLE_DIR "${CMAKE_BINARY_DIR}/web-bundle")

# The Qt Quick plugins the application imports, as the targets Qt's own finalisation would link
# had it found them (see qml_plugins_web.cpp, which names their classes). Basic is the one Qt Quick
# Controls style: the interface is drawn by the kit and uses the controls' behaviour only.
set(SNS_WEB_QML_PLUGIN_TARGETS
    Qt6::qmlplugin
    Qt6::modelsplugin
    Qt6::qtquick2plugin
    Qt6::quickwindow
    Qt6::qquicklayoutsplugin
    Qt6::qmlshapesplugin
    Qt6::qtquicktemplates2plugin
    Qt6::qtquickcontrols2plugin
    Qt6::qtquickcontrols2implplugin
    Qt6::qtquickcontrols2basicstyleplugin
    Qt6::qtquickcontrols2basicstyleimplplugin)

# The persistent directory the boot script mounts (sns-boot.js) and the environment variable that
# makes Qt's per-user data directory a place under it (XDG_DATA_HOME). One name, in three files
# (this one, paths_web.cpp, sns-boot.js); tests/platform/check_web.cmake holds them to each other.
set(SNS_WEB_PERSISTENT_DIR "/persist")

# The data the program reads beside its executable on a desktop is compiled into the
# application's resources here and unpacked on start (paths_web.cpp, which names this prefix).
set(SNS_WEB_DATA_RESOURCE_PREFIX "/sns-web-data")

# What makes the application target a page of a browser: its own page and loader instead of
# Qt's demonstration ones, the exports the loader needs, IndexedDB's file system, and the data.
function(sns_platform_configure_app target)
    # Qt writes a demonstration page (Qt's logo and the words "Qt for WebAssembly") and copies
    # its loader beside the program unless told not to; the bundle has its own page below.
    set_target_properties(${target} PROPERTIES
        NO_WASM_DEFAULT_FILES TRUE
        # ENV: sns-boot.js sets XDG_DATA_HOME before main() runs. (FS, which it also needs, is
        # exported by Qt already.)
        QT_WASM_EXTRA_EXPORTED_METHODS "ENV")
    # The IndexedDB-backed file system, mounted by sns-boot.js at SNS_WEB_PERSISTENT_DIR.
    target_link_options(${target} PRIVATE "-lidbfs.js")
    # No eval and no `new Function` in the program's script (embind builds its call stubs with
    # them otherwise), so the page can say script-src 'self' and WebAssembly with nothing more
    # (index.html.in's Content-Security-Policy).
    target_link_options(${target} PRIVATE "SHELL:-s DYNAMIC_EXECUTION=0")

    # The QML modules are named by hand (qml_plugins_web.cpp), not found by the import scan: the scan
    # reads every .qml under apps/app, the tests' included, and links what it
    # finds. tests/platform/check_web.cmake holds the list to the application's imports.
    set_target_properties(${target} PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN TRUE)
    target_sources(${target} PRIVATE "${PROJECT_SOURCE_DIR}/libs/platforms/web/qml_plugins_web.cpp")
    target_link_libraries(${target} PRIVATE ${SNS_WEB_QML_PLUGIN_TARGETS})

    # The dictionary and the dice, as one resource tree. Listed here, once, with the files
    # apps/app/CMakeLists.txt copies beside a desktop executable and the Android resources.
    set(_data "${PROJECT_SOURCE_DIR}/data")
    qt_add_resources(${target} sns_web_data
        PREFIX "${SNS_WEB_DATA_RESOURCE_PREFIX}"
        BASE "${_data}"
        FILES
            "${_data}/dictionaries/en.txt"
            "${_data}/dice/3x3.txt"
            "${_data}/dice/4x4.txt"
            "${_data}/dice/5x5.txt"
            "${_data}/dice/6x6.txt")

    # The bundle: the page, the boot script, the manifest and icons from packaging/ and the
    # program the link made, in one directory that a static web server can serve as it is.
    # The page and the manifest are written at configure time (the name, the version, the
    # colours); the program and Qt's loader are copied after every link.
    set(SNS_WEB_APP_NAME "${target}")
    set(SNS_WEB_ENTRY_NAME "${target}_entry")
    set(SNS_WEB_PAGE_TITLE "Snap and Spell")
    configure_file("${SNS_WEB_PACKAGING_DIR}/index.html.in" "${SNS_WEB_BUNDLE_DIR}/index.html" @ONLY)
    configure_file("${SNS_WEB_PACKAGING_DIR}/manifest.webmanifest.in"
        "${SNS_WEB_BUNDLE_DIR}/manifest.webmanifest" @ONLY)
    configure_file("${SNS_WEB_PACKAGING_DIR}/sns-boot.js" "${SNS_WEB_BUNDLE_DIR}/sns-boot.js" COPYONLY)
    configure_file("${PROJECT_SOURCE_DIR}/assets/icon/app-256.png" "${SNS_WEB_BUNDLE_DIR}/icon-256.png" COPYONLY)
    configure_file("${PROJECT_SOURCE_DIR}/assets/icon/app-64.png" "${SNS_WEB_BUNDLE_DIR}/icon-64.png" COPYONLY)
    # The licence and the third-party notices travel with the files (the notices are also in the
    # program's resources, which is what About shows).
    configure_file("${PROJECT_SOURCE_DIR}/LICENSE" "${SNS_WEB_BUNDLE_DIR}/LICENSE.txt" COPYONLY)
    configure_file("${SNS_NOTICES_FILE}" "${SNS_WEB_BUNDLE_DIR}/NOTICES.txt" COPYONLY)

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE_DIR:${target}>/${target}.js"
            "$<TARGET_FILE_DIR:${target}>/${target}.wasm"
            "${QT6_INSTALL_PREFIX}/plugins/platforms/qtloader.js"
            "${SNS_WEB_BUNDLE_DIR}"
        COMMENT "Making the web bundle in ${SNS_WEB_BUNDLE_DIR}"
        VERBATIM)
endfunction()

# Qt is linked into the .wasm statically, with its plugins and the QML modules the application
# imports (Qt's own finalisation, as for iOS), so there is no runtime to put beside the program.
function(sns_platform_deploy_qt_runtime target qml_dir)
    set_target_properties(${target} PROPERTIES QT_QML_ROOT_PATH "${qml_dir};${ARGN}")
endfunction()

# No CPack generator: the bundle is a directory, zipped by the release workflow.
function(sns_platform_configure_packaging)
endfunction()
