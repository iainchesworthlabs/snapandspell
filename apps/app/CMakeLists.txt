# Qt6 is found by the top-level CMakeLists.txt. The binary this directory makes is
# GPL-3.0-or-later like the sources: the board scene is 2D (Qt Quick alone), and no
# Qt module the program links is offered under the GPL only.

# AUTOMOC and friends, and the QML module plumbing.
qt_standard_project_setup(REQUIRES 6.8)

# The interface's language: the catalogue installed on the application, the
# layout direction, and the eight codes the parent area offers
# (apps/app/src/language_manager.hpp). A library of its own because the tests that
# load the screens in each language need it, and because the .qm files it
# carries are the application's only translated resources: lrelease compiles
# them from i18n/ at build time and they are embedded as :/i18n/sns_<code>.qm.
add_library(sns_i18n STATIC
    src/language_manager.cpp
    src/language_manager.hpp)
set_target_properties(sns_i18n PROPERTIES AUTOMOC ON)
target_include_directories(sns_i18n PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(sns_i18n
    PUBLIC
        sns::lib
        Qt6::Core
        Qt6::Gui
    PRIVATE
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)
sns_add_translations(sns_i18n)

# The profiling seam: one small QML module (`SnapAndSpell.Scene`) with the
# Profiler singleton, built in scene/. (The board scene itself is the application's own
# QML, qml/solo/BoardScene.qml: it draws with the kit, which a module built before the
# application cannot import.)
add_subdirectory(src/scene)
set(SNS_SCENE_IMPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/scene")

# The touch half of making a word (PathTracer), plain C++ and shared by the two
# modes' models: Solo Play's SoloModel and the Family Assistant's AssistModel.
# A library of its own so the scan module can use it without a second copy of the
# same object in one application.
add_library(sns_trace STATIC
    src/solo/path_tracer.cpp)
target_include_directories(sns_trace PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/solo")
target_compile_features(sns_trace PUBLIC cxx_std_23)
target_link_libraries(sns_trace
    PUBLIC
        sns::lib
    PRIVATE
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)

# Sound, speech and motion: one backend each, chosen by the optional Qt modules
# the top-level CMakeLists.txt found (the variant pattern; see
# feedback/CMakeLists.txt). The boards' cues and the settings' answers are the
# pure library policy in sns/feedback.hpp.
if(SNS_QT_HAS_MULTIMEDIA)
    set(SNS_SOUND_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/sound-multimedia")
elseif(EMSCRIPTEN)
    # A browser has Web Audio and speech synthesis whatever Qt's kit lacks (docs/web.md).
    set(SNS_SOUND_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/sound-web")
else()
    set(SNS_SOUND_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/sound-silent")
endif()
if(SNS_QT_HAS_TEXTTOSPEECH)
    set(SNS_SPEECH_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/speech-texttospeech")
elseif(EMSCRIPTEN)
    set(SNS_SPEECH_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/speech-web")
else()
    set(SNS_SPEECH_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/speech-silent")
endif()
if(SNS_QT_HAS_SENSORS)
    set(SNS_MOTION_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/motion-sensors")
else()
    set(SNS_MOTION_VARIANT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/variants/motion-silent")
endif()
add_subdirectory(src/feedback)

# Mode A's camera: the shared CameraSource and conversion, with the multimedia
# or none backend the top-level CMakeLists.txt selected. The QML module in scan/
# compiles it (apps/app/src/camera/camera_source.hpp is a QML element), so it is added
# to the source list there rather than made a library of its own.
add_subdirectory(src/camera)

# The target is named after the program it makes, so the application is the
# obvious entry in an IDE's target list and `--target SnapAndSpell` builds
# it alone. On Windows it is a GUI-subsystem program and on macOS a bundle;
# that is the platform fragment's to say (sns_platform_configure_app below).
qt_add_executable(SnapAndSpell
    src/main.cpp)

# Whether the window is a desktop window, one the person sizes (and which has a
# least size), or the whole screen of a phone, a tablet or a browser page. The
# platform fragment says (SNS_PLATFORM_DESKTOP); the source never asks the
# operating system (docs/platform-isolation.md).
target_compile_definitions(SnapAndSpell PRIVATE
    SNS_DESKTOP_WINDOW=$<BOOL:${SNS_PLATFORM_DESKTOP}>)

# The kit's singletons: tokens, navigation, the sound and touch seam, and the
# two wording files of the child-facing screens.
set_source_files_properties(
    qml/HomeWording.qml
    qml/kit/Feedback.qml
    qml/kit/Navigation.qml
    qml/kit/Theme.qml
    qml/solo/JewelsWording.qml
    qml/solo/SoloWording.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Main.qml shows the parent area and the Family Assistant's screens, modules of
# their own that are built under parent/ and scan/ (added below), and SoloScreen
# uses the scene module built above: qmllint looks for them there. The type
# compiler cannot compile those two (they instantiate types of other modules,
# whose compiled C++ classes are not theirs to reference), so they are skipped
# from qmltc - their scripts are still compiled to bytecode by qmlcachegen -
# and they instantiate their screens dynamically.
set_source_files_properties(
    qml/Main.qml
    qml/solo/SoloScreen.qml
    PROPERTIES QT_QML_SKIP_TYPE_COMPILER TRUE)

# The rest of the skips are things the shared aqtinstall kit cannot link:
# - FlatBoard's Timer needs qqmltimer_p.h, and its old-form Connections needs
#   qqmlconnections_p.h, neither of which the kit ships;
# - Home, PlayingPanel and JewelsScreen set LayoutMirroring, whose compiled
#   private class (QQuickLayoutMirroringAttached) the kit's QtQuick DLL does not
#   export, so qmltc's C++ for them cannot link.
set_source_files_properties(
    qml/Home.qml
    qml/solo/FlatBoard.qml
    qml/solo/JewelsScreen.qml
    qml/solo/PlayingPanel.qml
    PROPERTIES QT_QML_SKIP_TYPE_COMPILER TRUE)

# The wording singletons carry qsTr(): compiled by qmltc, a translation is
# captured once at creation, before the catalogue is installed, and is never
# retranslated when the parent changes the language. They are made dynamically;
# the tokens (Theme) and the state (Navigation) hold no qsTr and stay compiled.
set_source_files_properties(
    qml/HomeWording.qml
    qml/solo/JewelsWording.qml
    qml/solo/SoloWording.qml
    PROPERTIES QT_QML_SKIP_TYPE_COMPILER TRUE)

# The kit's files are listed once, in qml/kit/kit_files.cmake: the previews and
# the Qt Quick Test module embed the same set.
include("${CMAKE_CURRENT_SOURCE_DIR}/qml/kit/kit_files.cmake")
set(SNS_APP_KIT_SOURCES "")
foreach(_sns_kit_file IN LISTS SNS_KIT_QML_FILES)
    list(APPEND SNS_APP_KIT_SOURCES "qml/kit/${_sns_kit_file}")
endforeach()

qt_add_qml_module(SnapAndSpell
    URI SnapAndSpell
    VERSION 1.0
    ENABLE_TYPE_COMPILER
    IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/src/parent"
    IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/src/scan"
    IMPORT_PATH "${SNS_SCENE_IMPORT_DIR}"
    QML_FILES
        qml/Home.qml
        qml/HomeWording.qml
        qml/Main.qml
        ${SNS_APP_KIT_SOURCES}
        qml/solo/BoardScene.qml
        qml/solo/BoardThumb.qml
        qml/solo/FlatBoard.qml
        qml/solo/HistorySheet.qml
        qml/solo/JewelsScreen.qml
        qml/solo/JewelsWording.qml
        qml/solo/PlayingPanel.qml
        qml/solo/SceneDie.qml
        qml/solo/SceneScript.js
        qml/solo/ScrollCue.qml
        qml/solo/SoloBox.qml
        qml/solo/SoloScreen.qml
        qml/solo/SoloWording.qml
    SOURCES
        src/frame_profiler.cpp
        src/frame_profiler.hpp
        src/solo/board_layout.cpp
        src/solo/board_layout.hpp
        src/solo/path_tracer.hpp
        src/solo/real_solo_game.cpp
        src/solo/real_solo_game.hpp
        src/solo/solo_game.cpp
        src/solo/solo_game.hpp
        src/solo/solo_game_factory.cpp
        src/solo/solo_game_factory.hpp
        src/solo/solo_game_setup.cpp
        src/solo/solo_game_setup.hpp
        src/solo/solo_layout.cpp
        src/solo/solo_layout.hpp
        src/solo/solo_model.cpp
        src/solo/solo_model.hpp
        src/startup_failure.cpp
        src/startup_failure.hpp
        src/system_colour_scheme.cpp
        src/system_colour_scheme.hpp
        src/system_motion.cpp
        src/system_motion.hpp)

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(SnapAndSpell)

# The type compiler's generated C++ reads Qt's private headers (cmake/FindQt6.cmake).
sns_enable_qmltc_private_headers(SnapAndSpell)

# The feedback sounds, embedded as :/sounds/<name>.wav. They are placeholders
# made by tools/sounds/make_sounds.ps1 and owned by the project (notices/
# fragments/assets.txt); the phoneme clips and cheers the reviewer will record
# replace them later. A build without Qt Multimedia still emits no sound, but
# the files travel in the application either way: they are tiny, and the
# silent variant is a build fact, not a different game.
set(SNS_SOUND_NAMES tap rattle letter land cheer tick chime cleared)
set(SNS_SOUND_FILES "")
foreach(SNS_SOUND_NAME IN LISTS SNS_SOUND_NAMES)
    set(SNS_SOUND_FILE "${PROJECT_SOURCE_DIR}/assets/sounds/${SNS_SOUND_NAME}.wav")
    if(NOT EXISTS "${SNS_SOUND_FILE}")
        message(FATAL_ERROR
            "The feedback sound ${SNS_SOUND_FILE} is missing. Regenerate the "
            "placeholders with tools/sounds/make_sounds.ps1 (docs/building.md, "
            "\"The Qt Quick window\").")
    endif()
    list(APPEND SNS_SOUND_FILES "${SNS_SOUND_FILE}")
endforeach()
qt_add_resources(SnapAndSpell sns_sounds
    PREFIX "/sounds"
    BASE "${PROJECT_SOURCE_DIR}/assets/sounds"
    FILES ${SNS_SOUND_FILES})

# The fonts the screens use: the Toybox faces (Pixelify Sans for the chrome, Silkscreen
# for the upper-case labels) and Andika for the learning text, the dice and every
# numeral (docs/design/toybox/DESIGN.md, section 4), and Noto Sans SC, JP and Hebrew,
# subset to the catalogues' characters (tools/design/make_noto_subsets.py), for the
# languages the first three lack (the web has no system fonts). They
# are SIL Open Font Licence works; the notices carry the licence text. They are
# compiled in rather than installed beside the program, so a package cannot be
# missing one and QML's font lookup is the same on every target.
set(SNS_FONT_FILES
    "${PROJECT_SOURCE_DIR}/assets/fonts/PixelifySans-Medium.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/PixelifySans-Bold.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/Silkscreen-Regular.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/Andika-Regular.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/Andika-Bold.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansSC-Regular.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansSC-Bold.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansJP-Regular.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansJP-Bold.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansHebrew-Regular.ttf"
    "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansHebrew-Bold.ttf")
foreach(SNS_FONT_FILE IN LISTS SNS_FONT_FILES)
    if(NOT EXISTS "${SNS_FONT_FILE}")
        message(FATAL_ERROR "The application font ${SNS_FONT_FILE} is missing.")
    endif()
endforeach()
qt_add_resources(SnapAndSpell sns_fonts
    PREFIX "/fonts"
    BASE "${PROJECT_SOURCE_DIR}/assets/fonts"
    FILES ${SNS_FONT_FILES})

# The window and taskbar icon. The Windows executable's own icon comes from the
# .rc file below; the QML window also sets it so a bundled Linux/macOS build has
# the same mark.
qt_add_resources(SnapAndSpell sns_icon
    PREFIX "/icons"
    BASE "${PROJECT_SOURCE_DIR}/assets/icon"
    FILES "${PROJECT_SOURCE_DIR}/assets/icon/app.png")
if(SNS_PLATFORM_APP_RC_TEMPLATE)
    file(TO_CMAKE_PATH "${SNS_PLATFORM_APP_ICON}" SNS_APP_ICON)
    configure_file("${SNS_PLATFORM_APP_RC_TEMPLATE}"
        "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
    target_sources(SnapAndSpell PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

# The Qt Multimedia QML module is part of the camera's deployment. A static QML
# module's own imports are not what the application's QML import scanner reads,
# so a one-line resource file that imports it is added to the application when
# the camera is the multimedia one: the deploy scripts then ship
# qml/QtMultimedia beside the window, and the live view loads. A kit without the
# module compiles neither the file nor the preview.
if(SNS_QT_HAS_MULTIMEDIA)
    qt_add_resources(SnapAndSpell sns_camera_import
        PREFIX "/camera"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/src/camera"
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/camera/CameraImport.qml")
endif()

# The QML compiler's generated C++ includes apps/app/src/solo's headers by bare name
# (the type registrations), so that directory is on the include path.
target_include_directories(SnapAndSpell PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/solo")

# The C++ the QML compiler writes for each QML file is built with the target's
# warnings as errors, and MSVC's code generator reports C4702 (unreachable code)
# from inside Qt's own headers while compiling it. /external:W0 does not silence
# a warning raised after the front end, so those files, and only those - the
# bytecode compiler's (qmlcachegen) and the type compiler's (qmltc) output - are
# told to leave C4702 alone; the code written by hand keeps it.
get_target_property(_sns_app_sources SnapAndSpell SOURCES)
foreach(_sns_source IN LISTS _sns_app_sources)
    if(_sns_source MATCHES "/[.]rcc/qmlcache/" OR _sns_source MATCHES "/[.]qmltc/")
        set_source_files_properties("${_sns_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

# The parent area: the gate, the dashboard and the settings, a module of its own
# (apps/app/src/parent) for the screen shell to show when a parent asks.
add_subdirectory(src/parent)

# Mode A, the Family Assistant: the scan and check screens and the assistant's
# board, a module of their own (apps/app/src/scan) for the screen shell to show.
add_subdirectory(src/scan)

target_link_libraries(SnapAndSpell
    PRIVATE
        sns_parentplugin
        sns_scan_uiplugin
        sns_sceneplugin
        sns_feedback
        sns_i18n
        sns_trace
        sns::lib
        sns::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickLayouts
        Qt6::QuickShapes
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)

# What makes it a program of the platform's desktop, and its Qt runtime beside
# it: both are the selected platform's to say (libs/platforms/<name>/).
sns_platform_configure_app(SnapAndSpell)
# The live view imports QtMultimedia from the camera variant, which is not under
# qml/: its directory is a second place windeployqt scans for imports, so a build
# tree has qml/QtMultimedia for the preview (the packaged copy gets it from the
# resource hint above through Qt's install-time deploy).
set(SNS_APP_QML_DEPLOY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/qml")
if(SNS_QT_HAS_MULTIMEDIA)
    list(APPEND SNS_APP_QML_DEPLOY_DIRS "${SNS_CAMERA_VARIANT_DIR}")
endif()
sns_platform_deploy_qt_runtime(SnapAndSpell ${SNS_APP_QML_DEPLOY_DIRS})
sns_deploy_sanitizer_runtime(SnapAndSpell)

# The default dictionary and the standard board's dice the game is played with:
# dictionaries/en.txt and dice/3x3.txt to 6x6.txt beside the executable (see
# SoloGameSetup and main.cpp).
add_custom_command(TARGET SnapAndSpell POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:SnapAndSpell>/dictionaries"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        "$<TARGET_FILE_DIR:SnapAndSpell>/dictionaries/en.txt"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:SnapAndSpell>/dice"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/3x3.txt"
        "$<TARGET_FILE_DIR:SnapAndSpell>/dice/3x3.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/4x4.txt"
        "$<TARGET_FILE_DIR:SnapAndSpell>/dice/4x4.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/5x5.txt"
        "$<TARGET_FILE_DIR:SnapAndSpell>/dice/5x5.txt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dice/6x6.txt"
        "$<TARGET_FILE_DIR:SnapAndSpell>/dice/6x6.txt"
    COMMENT "Placing the English dictionary and the dice beside the application")

# ---------------------------------------------------------------------------
# The install rules (docs/building.md, "Packaging"). The installed program is
# the built one plus what it reads and the Qt runtime it loads; a package is
# this tree plus apps/cli's, with the notices (cmake/Notices.cmake).
#
# The data goes where the program looks for it: beside its own executable,
# which is CMAKE_INSTALL_BINDIR for an ordinary executable and
# Contents/MacOS inside a bundle on macOS - the platform fragment declares it
# (SNS_PLATFORM_INSTALL_APP_DATA_DIR, cmake/Platform.cmake). The dictionary's
# rule here covers an application-only build and a bundle; when the
# command-line program is built too and both share the bindir, its own rule
# installs the one file and this one stays out of the way.
# ---------------------------------------------------------------------------
# LIBRARY: on Android the application target is a module library (a shared object
# the activity loads); the rule is never used there, but the generator insists on it.
install(TARGETS SnapAndSpell
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION "."
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES
    "${PROJECT_SOURCE_DIR}/data/dice/3x3.txt"
    "${PROJECT_SOURCE_DIR}/data/dice/4x4.txt"
    "${PROJECT_SOURCE_DIR}/data/dice/5x5.txt"
    "${PROJECT_SOURCE_DIR}/data/dice/6x6.txt"
    DESTINATION "${SNS_PLATFORM_INSTALL_APP_DATA_DIR}/dice")
if(NOT SNS_PLATFORM_INSTALL_APP_DATA_DIR STREQUAL CMAKE_INSTALL_BINDIR OR NOT SNS_BUILD_CLI)
    install(FILES "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        DESTINATION "${SNS_PLATFORM_INSTALL_APP_DATA_DIR}/dictionaries")
endif()

# The Qt runtime, plugins and QML modules of the INSTALLED program, with Qt's
# own QML-aware deploy script. It reads the built binary and scans the QML it
# imports, so the installed copy carries the modules
# the screens import, not a list kept here.
#
# Not the plain qt_generate_deploy_app_script(): that one deploys library
# dependencies only, and a package made with it has an empty qml/ directory
# (the sibling projects measured that). The QML-aware sibling is the one that
# runs qt6_deploy_qml_imports.
#
# NO_TRANSLATIONS: the interface's own catalogue is embedded in the program,
# and Qt's catalogues are neither installed nor merged (docs/licensing.md).
# INCLUDE_PLUGINS qoffscreen: the packaged program's headless smoke
# (tools/package/check_package.ps1, and CI) runs with QT_QPA_PLATFORM=offscreen,
# and windeployqt otherwise ships only the platform plugin the desktop uses.
# EXCLUDE_PLUGIN_TYPES qmltooling: the build-time deploy above says why - the QML
# debugger and profiler plugins have no place in a package.
#
# Only where the installed program carries Qt (SNS_PLATFORM_QT_DEPLOY_AT_INSTALL):
# on Linux the system's Qt is used and a deploy script's qt.conf would break it
# (libs/platforms/linux/platform.cmake says where that was measured).
# ---------------------------------------------------------------------------
if(SNS_PLATFORM_QT_DEPLOY_AT_INSTALL)
    qt_generate_deploy_qml_app_script(
        TARGET SnapAndSpell
        OUTPUT_SCRIPT SNS_APP_DEPLOY_SCRIPT
        NO_UNSUPPORTED_PLATFORM_ERROR
        NO_TRANSLATIONS
        INCLUDE_PLUGINS qoffscreen
        EXCLUDE_PLUGIN_TYPES qmltooling)
    install(SCRIPT "${SNS_APP_DEPLOY_SCRIPT}")
endif()

# The third-party notices, as :/notices/NOTICES.txt, for the About screen's
# Licences view (cmake/Notices.cmake; the file itself is installed beside the
# programs by sns_install_notices()).
sns_add_notices_resource(SnapAndSpell)


