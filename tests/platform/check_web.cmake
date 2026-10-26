# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# The web target, held together: the files it is made of are there, the three places that name the
# persistent directory name the same one, the QML modules the application imports are all linked
# into the program (the web build turns Qt's import scan off, so a module nobody lists is a page
# that does not start), the presets that build it exist with the settings that make them web
# builds, and the pins file and the workflow install the SDK and the kit the build wants.
#
#   cmake -DREPO_ROOT=<repository root> -P tests/platform/check_web.cmake
#
# Everything here is read from files, so it runs on every host and in every CI leg (the web build
# itself runs only where Emscripten and Qt's WebAssembly kit are: .github/workflows/web-build.yml).
# What it cannot say is whether the page starts: that is the workflow's browser run, and a person's.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

if(NOT REPO_ROOT)
    message(FATAL_ERROR "Pass -DREPO_ROOT=<repository root>.")
endif()

set(_failures "")
function(_fail text)
    set(_failures "${_failures}\n  ${text}" PARENT_SCOPE)
endfunction()

function(_read file result)
    if(NOT EXISTS "${REPO_ROOT}/${file}")
        _fail("${file} does not exist")
        set(${result} "" PARENT_SCOPE)
        return()
    endif()
    file(READ "${REPO_ROOT}/${file}" _content)
    string(REPLACE "\r\n" "\n" _content "${_content}")
    set(${result} "${_content}" PARENT_SCOPE)
endfunction()

# ---- the files ----------------------------------------------------------------

set(_dir "libs/platforms/web")
foreach(_file IN ITEMS
        "${_dir}/platform.cmake"
        "${_dir}/paths_web.cpp"
        "${_dir}/haptics_web.cpp"
        "${_dir}/fatal_error_web.cpp"
        "${_dir}/qml_plugins_web.cpp"
        "${_dir}/packaging/index.html.in"
        "${_dir}/packaging/wjs-boot.js"
        "${_dir}/packaging/manifest.webmanifest.in"
        "${_dir}/packaging/verify_web.sh"
        "${_dir}/packaging/smoke_web.sh"
        "notices/fragments/application-web.txt"
        "notices/fragments/qt-web.txt"
        ".github/workflows/web-build.yml")
    if(NOT EXISTS "${REPO_ROOT}/${_file}")
        _fail("${_file} is missing")
    endif()
endforeach()

# ---- the fragment says what a browser is ----------------------------------------

_read("${_dir}/platform.cmake" _fragment)
foreach(_pair IN ITEMS
        "WJS_PLATFORM_EXECUTABLE_FORMAT|wasm"
        "WJS_PLATFORM_DESKTOP|OFF"
        "WJS_PLATFORM_QT_DEPLOY_AT_INSTALL|OFF"
        "WJS_PLATFORM_QT_ARCH_DIRS|wasm_singlethread")
    string(REPLACE "|" ";" _parts "${_pair}")
    list(GET _parts 0 _name)
    list(GET _parts 1 _value)
    if(NOT _fragment MATCHES "set[(]${_name} \"?${_value}\"?[)]")
        _fail("platform.cmake does not set ${_name} to ${_value}")
    endif()
endforeach()
# One thread (the multithreaded kit needs headers GitHub Pages cannot send), no Tracy (its client
# listens on the network), and no Quick 3D.
if(_fragment MATCHES "wasm_multithread")
    _fail("platform.cmake names the multithreaded kit; a page needs cross-origin isolation headers to use it")
endif()
if(_fragment MATCHES "WJS_ENABLE_TRACY")
    _fail("platform.cmake mentions WJS_ENABLE_TRACY; the preset keeps Tracy off")
endif()
if(_fragment MATCHES "Quick3D|quick3d")
    _fail("platform.cmake mentions Quick 3D; the web build draws the 2D scene")
endif()

# ---- the persistent directory has one name ----------------------------------------

set(_persist "")
if(_fragment MATCHES "set[(]WJS_WEB_PERSISTENT_DIR \"([^\"]+)\"[)]")
    set(_persist "${CMAKE_MATCH_1}")
else()
    _fail("platform.cmake does not set WJS_WEB_PERSISTENT_DIR")
endif()
_read("${_dir}/paths_web.cpp" _paths)
if(NOT _paths MATCHES "persistent_directory = \"([^\"]+)\"" OR NOT CMAKE_MATCH_1 STREQUAL _persist)
    _fail("paths_web.cpp's persistent_directory is not '${_persist}', which platform.cmake mounts")
endif()
_read("${_dir}/packaging/wjs-boot.js" _boot)
if(NOT _boot MATCHES "PERSIST_DIR = '([^']+)'" OR NOT CMAKE_MATCH_1 STREQUAL _persist)
    _fail("wjs-boot.js's PERSIST_DIR is not '${_persist}', which platform.cmake names")
endif()
# The environment variable that makes Qt keep its per-user data below it.
if(NOT _boot MATCHES "XDG_DATA_HOME: PERSIST_DIR")
    _fail("wjs-boot.js does not point XDG_DATA_HOME at PERSIST_DIR, so the store would not be kept")
endif()
# The program has to export what the boot script uses.
if(NOT _fragment MATCHES "QT_WASM_EXTRA_EXPORTED_METHODS \"ENV\"")
    _fail("platform.cmake does not export ENV, which wjs-boot.js sets XDG_DATA_HOME on")
endif()
if(NOT _fragment MATCHES "-lidbfs[.]js")
    _fail("platform.cmake does not link idbfs, which wjs-boot.js mounts")
endif()
# The page's policy allows no eval, which embind's call stubs need unless the program is linked without
# dynamic execution; and it allows connections to the page's own site only.
if(NOT _fragment MATCHES "DYNAMIC_EXECUTION=0")
    _fail("platform.cmake does not link with DYNAMIC_EXECUTION=0, so the page's Content-Security-Policy would stop the program from starting")
endif()
_read("${_dir}/packaging/index.html.in" _page)
if(NOT _page MATCHES "Content-Security-Policy" OR NOT _page MATCHES "connect-src 'self'")
    _fail("index.html.in has no Content-Security-Policy with connect-src 'self'")
endif()
if(_page MATCHES "script-src[^;\"]*'unsafe-(eval|inline)'")
    _fail("index.html.in's script-src allows eval or inline script")
endif()

# ---- every QML module the application imports is linked in -----------------------------

# <module imported by the QML> -> <the plugin class qml_plugins_web.cpp imports and the CMake target
# platform.cmake links>. A module that is the project's own (WordJewelsSolver and its submodules) is
# named by its class; QtQuick.Controls brings its Basic style and the implementation modules.
set(_modules
    "QtQml|QtQmlPlugin|Qt6::qmlplugin"
    "QtQml.Models|QtQmlModelsPlugin|Qt6::modelsplugin"
    "QtQuick|QtQuick2Plugin|Qt6::qtquick2plugin"
    "QtQuick.Window|QtQuick_WindowPlugin|Qt6::quickwindow"
    "QtQuick.Layouts|QtQuickLayoutsPlugin|Qt6::qquicklayoutsplugin"
    "QtQuick.Shapes|QmlShapesPlugin|Qt6::qmlshapesplugin"
    "QtQuick.Templates|QtQuickTemplates2Plugin|Qt6::qtquicktemplates2plugin"
    "QtQuick.Controls|QtQuickControls2Plugin|Qt6::qtquickcontrols2plugin"
    "QtQuick.Controls.impl|QtQuickControls2ImplPlugin|Qt6::qtquickcontrols2implplugin"
    "QtQuick.Controls.Basic|QtQuickControls2BasicStylePlugin|Qt6::qtquickcontrols2basicstyleplugin"
    "QtQuick.Controls.Basic.impl|QtQuickControls2BasicStyleImplPlugin|Qt6::qtquickcontrols2basicstyleimplplugin"
    "WordJewelsSolver.Parent|WordJewelsSolver_ParentPlugin|"
    "WordJewelsSolver.Scan|WordJewelsSolver_ScanPlugin|"
    "WordJewelsSolver.Scene|WordJewelsSolver_ScenePlugin|")
_read("${_dir}/qml_plugins_web.cpp" _plugins)
set(_known_modules "")
foreach(_entry IN LISTS _modules)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _module)
    list(GET _parts 1 _class)
    list(GET _parts 2 _target)
    list(APPEND _known_modules "${_module}")
    if(NOT _plugins MATCHES "Q_IMPORT_QML_PLUGIN[(]${_class}[)]")
        _fail("qml_plugins_web.cpp does not import ${_class} (module ${_module})")
    endif()
    if(_target AND NOT _fragment MATCHES "${_target}")
        _fail("platform.cmake does not link ${_target} (module ${_module})")
    endif()
endforeach()
# Nothing is imported that is not in the table: the plugin names between the parentheses.
string(REGEX MATCHALL "Q_IMPORT_QML_PLUGIN[(][A-Za-z0-9_]+[)]" _imports "${_plugins}")
list(LENGTH _imports _import_count)
list(LENGTH _modules _module_count)
if(NOT _import_count EQUAL _module_count)
    _fail("qml_plugins_web.cpp imports ${_import_count} plugins and this check knows ${_module_count} modules: keep them in step")
endif()

# The application's QML, its modules' QML and the kit's. The tests' QML (apps/app/ui-tests) is
# not shipped on the web, so it is not read here. The camera
# variant is not either: QtMultimedia is not in the kit, and the variant that is built is camera-none.
file(GLOB_RECURSE _qml_files
    "${REPO_ROOT}/apps/app/qml/*.qml"
    "${REPO_ROOT}/apps/app/src/parent/*.qml"
    "${REPO_ROOT}/apps/app/src/scan/*.qml"
    "${REPO_ROOT}/apps/app/src/variants/*-none/*.qml"
    "${REPO_ROOT}/apps/app/src/variants/*-silent/*.qml")
set(_shipped_imports "")
foreach(_qml IN LISTS _qml_files)
    file(STRINGS "${_qml}" _import_lines REGEX "^[ \t]*import[ \t]+[A-Za-z]")
    foreach(_line IN LISTS _import_lines)
        string(REGEX REPLACE "^[ \t]*import[ \t]+([A-Za-z0-9_.]+).*$" "\\1" _imported "${_line}")
        list(APPEND _shipped_imports "${_imported}")
    endforeach()
endforeach()
list(REMOVE_DUPLICATES _shipped_imports)
foreach(_imported IN LISTS _shipped_imports)
    if(_imported STREQUAL "WordJewelsSolver")
        continue()  # the application's own module, registered by the program
    endif()
    if(NOT _imported IN_LIST _known_modules)
        _fail("the application's QML imports ${_imported}, which the web build does not link: add its plugin to qml_plugins_web.cpp and platform.cmake, and to the table in this check")
    endif()
endforeach()

# ---- the presets ------------------------------------------------------------------

_read("CMakePresets.json" _presets)
string(JSON _count LENGTH "${_presets}" configurePresets)
math(EXPR _last "${_count} - 1")
foreach(_i RANGE 0 ${_last})
    string(JSON _name GET "${_presets}" configurePresets ${_i} name)
    set("_INDEX_${_name}" ${_i})
    if(_name MATCHES "^web" AND _name MATCHES "-debug$")
        _fail("the preset '${_name}' is named *-debug, which tests/profiling/check_presets.cmake reads as 'Tracy on'; no web build has Tracy")
    endif()
endforeach()

function(_cache_value preset variable result)
    set(${result} "<unset>" PARENT_SCOPE)
    if(NOT DEFINED _INDEX_${preset})
        return()
    endif()
    string(JSON _type ERROR_VARIABLE _error TYPE "${_presets}" configurePresets ${_INDEX_${preset}}
        cacheVariables ${variable})
    if(_error STREQUAL "NOTFOUND")
        string(JSON _value GET "${_presets}" configurePresets ${_INDEX_${preset}} cacheVariables ${variable})
        set(${result} "${_value}" PARENT_SCOPE)
    endif()
endfunction()

foreach(_preset IN ITEMS web-base web-release)
    if(NOT DEFINED _INDEX_${_preset})
        _fail("CMakePresets.json has no configure preset '${_preset}'")
    endif()
endforeach()

if(DEFINED _INDEX_web-base)
    string(JSON _hidden ERROR_VARIABLE _error GET "${_presets}" configurePresets ${_INDEX_web-base} hidden)
    if(NOT _hidden STREQUAL "ON" AND NOT _hidden STREQUAL "true")
        _fail("web-base is not hidden")
    endif()
    foreach(_pair IN ITEMS
            "VCPKG_TARGET_TRIPLET=wasm32-emscripten"
            "WJS_BUILD_APP=ON"
            "WJS_BUILD_CLI=OFF"
            "WJS_BUILD_TESTS=OFF"
            "WJS_ENABLE_TRACY=OFF"
            "WJS_ENABLE_SANITIZERS=OFF"
            "WJS_ENABLE_HARDENING=OFF")
        string(FIND "${_pair}" "=" _equals)
        string(SUBSTRING "${_pair}" 0 ${_equals} _key)
        math(EXPR _start "${_equals} + 1")
        string(SUBSTRING "${_pair}" ${_start} -1 _expected)
        _cache_value(web-base "${_key}" _value)
        if(NOT _value STREQUAL _expected)
            _fail("web-base sets ${_key} to '${_value}', not '${_expected}'")
        endif()
    endforeach()
    _cache_value(web-base VCPKG_CHAINLOAD_TOOLCHAIN_FILE _chain)
    if(NOT _chain MATCHES "lib/cmake/Qt6/qt[.]toolchain[.]cmake$" OR NOT _chain MATCHES "QT_WEB_DIR")
        _fail("web-base chain-loads '${_chain}'; it must be QT_WEB_DIR's lib/cmake/Qt6/qt.toolchain.cmake")
    endif()
    _cache_value(web-base QT_HOST_PATH _host)
    string(CONCAT _from_environment "$" "env{" "QT_HOST_PATH" "}")
    if(NOT _host STREQUAL _from_environment)
        _fail("web-base does not read QT_HOST_PATH from the environment (it is '${_host}')")
    endif()
endif()

if(DEFINED _INDEX_web-release)
    string(JSON _inherits GET "${_presets}" configurePresets ${_INDEX_web-release} inherits)
    foreach(_parent IN ITEMS web-base release)
        if(NOT _inherits MATCHES "${_parent}")
            _fail("web-release does not inherit '${_parent}'")
        endif()
    endforeach()
endif()

string(JSON _build_count LENGTH "${_presets}" buildPresets)
math(EXPR _build_last "${_build_count} - 1")
set(_build_presets "")
foreach(_i RANGE 0 ${_build_last})
    string(JSON _name GET "${_presets}" buildPresets ${_i} name)
    string(JSON _targets ERROR_VARIABLE _error GET "${_presets}" buildPresets ${_i} targets)
    if(_error STREQUAL "NOTFOUND")
        set("_BUILD_TARGETS_${_name}" "${_targets}")
    endif()
    list(APPEND _build_presets "${_name}")
endforeach()
if(NOT "web-release" IN_LIST _build_presets)
    _fail("CMakePresets.json has no build preset 'web-release'")
elseif(NOT "${_BUILD_TARGETS_web-release}" MATCHES "WordJewelsSolver")
    _fail("the build preset 'web-release' does not build the WordJewelsSolver target")
endif()

# ---- the pins ------------------------------------------------------------------------

_read(".github/toolchain-versions.json" _versions)
_read(".github/workflows/web-build.yml" _workflow)
string(JSON _qt_version GET "${_versions}" qt version)
string(JSON _kit_arch GET "${_versions}" qt kits web arch)
string(JSON _kit_dir GET "${_versions}" qt kits web dir)
string(JSON _kit_host GET "${_versions}" qt kits web host)
string(JSON _kit_hostdir GET "${_versions}" qt kits web hostdir)
string(JSON _em_version GET "${_versions}" emscripten version)
string(JSON _em_commit GET "${_versions}" emscripten emsdk_commit)
string(JSON _aqt GET "${_versions}" qt aqtsource)
string(JSON _action GET "${_versions}" qt action)

if(NOT _kit_arch STREQUAL "wasm_singlethread")
    _fail("toolchain-versions.json qt.kits.web.arch is '${_kit_arch}', not wasm_singlethread")
endif()
if(NOT _workflow MATCHES "uses: ${_action}")
    _fail("web-build.yml does not use the install-qt-action pin of toolchain-versions.json (${_action})")
endif()
foreach(_setting IN ITEMS "version: \"${_qt_version}\"" "host: ${_kit_host}" "target: wasm" "arch: ${_kit_arch}" "aqtsource: \"${_aqt}\"")
    string(FIND "${_workflow}" "${_setting}" _at)
    if(_at EQUAL -1)
        _fail("web-build.yml does not install the kit as toolchain-versions.json says (no '${_setting}')")
    endif()
endforeach()
if(NOT _workflow MATCHES "Qt/${_qt_version}/${_kit_dir}" OR NOT _workflow MATCHES "Qt/${_qt_version}/${_kit_hostdir}")
    _fail("web-build.yml does not name the kit directories ${_kit_dir} and ${_kit_hostdir}")
endif()
if(NOT _workflow MATCHES "emsdk-version: \"${_em_version}\"")
    _fail("web-build.yml installs another Emscripten than toolchain-versions.json (${_em_version})")
endif()
if(NOT _workflow MATCHES "emsdk-repo-commit: \"${_em_commit}\"")
    _fail("web-build.yml names another emsdk commit than toolchain-versions.json (${_em_commit})")
endif()
if(NOT _workflow MATCHES "cmakeVersion: \"")
    _fail("web-build.yml pins no CMake")
endif()
# Pinned to a full commit, no permissions at the top, no secret.
file(STRINGS "${REPO_ROOT}/.github/workflows/web-build.yml" _uses REGEX "^[ \t]*(-[ \t]*)?uses:[ \t]*[^./ ]")
foreach(_line IN LISTS _uses)
    if(NOT _line MATCHES "@[0-9a-f]+ ")
        _fail("web-build.yml has an action that is not pinned to a full commit: ${_line}")
    endif()
endforeach()
if(NOT _workflow MATCHES "\npermissions: {}\n")
    _fail("web-build.yml has permissions at the top level")
endif()
if(_workflow MATCHES "secrets[.]")
    _fail("web-build.yml uses a secret; the web build signs nothing")
endif()
# The multithreaded kit must not creep in through the workflow either.
if(_workflow MATCHES "wasm_multithread")
    _fail("web-build.yml installs the multithreaded kit")
endif()

if(_failures)
    message(FATAL_ERROR "The web target is not held together:${_failures}")
endif()
message(STATUS "web: the files, the one persistent directory, the QML modules, the presets and the pins agree")
