# Mode A, the Family Assistant (docs/design/game.md, "Mode A: the Family
# Assistant and Scanner"): the scan and check screens, and the assistant's board
# and panel, as a QML module of their own, so that the application's CMake file
# has only to link it. The scanner's rules are the library's
# (wjs/scan/confirmation.hpp), the words' are wjs/game_rules.hpp; the two models
# here are the shape Qt Quick wants and nothing more.
#
# A screen of the application, imported as
#
#   import WordJewelsSolver.Scan
#   ScanScreen { scan: ...; assist: ...; onClosed: ... }
#
# The screens use the kit's tokens (WordJewelsSolver: Theme, AppButton, TileFace,
# Jewel, SoloLayout), so this module imports the application's own module; the
# application imports this one back. QML resolves that at run time from the two
# modules' resources, and the build links both plugins into the program.
#
# Qt is found by the top-level CMakeLists.txt, and qt_standard_project_setup
# has been called by apps/app/CMakeLists.txt, which adds this directory.

# The module is made here, so what the application's own file sets for its target
# is set again for this one: moc over the models, and the default resource prefix
# (":/qt/qml/") that Qt's policy QTP0001 names.
set(CMAKE_AUTOMOC ON)
qt_policy(SET QTP0001 NEW)

set(WJS_SCAN_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../qml/scan")

set(WJS_SCAN_QML_FILES
    AssistBoard.qml
    AssistHistory.qml
    AssistScreen.qml
    CameraOffArt.qml
    CheckScreen.qml
    LetterPicker.qml
    RingedQuestion.qml
    ScanScreen.qml
    ScanStepper.qml
    ScanWording.qml)

# The module's files are in another directory than this CMake file, so each is
# given the name it has in the module.
set(WJS_SCAN_QML_SOURCES "")
foreach(_file IN LISTS WJS_SCAN_QML_FILES)
    set_source_files_properties("${WJS_SCAN_QML_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_SCAN_QML_SOURCES "${WJS_SCAN_QML_DIR}/${_file}")
endforeach()

# The camera's live view is a QML file of the camera variant: a VideoOutput where
# the kit has Qt Multimedia, and an empty Item where it does not. ScanScreen.qml
# is one file either way. CMake selected the file with the backend
# (apps/app/CMakeLists.txt).
set_source_files_properties("${WJS_CAMERA_PREVIEW_QML}"
    PROPERTIES QT_RESOURCE_ALIAS "CameraPreview.qml")
list(APPEND WJS_SCAN_QML_SOURCES "${WJS_CAMERA_PREVIEW_QML}")

# The wording: every sentence and accessible name, in one singleton for the
# later i18n pass.
set_source_files_properties("${WJS_SCAN_QML_DIR}/ScanWording.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(wjs_scan_ui
    STATIC
    URI WordJewelsSolver.Scan
    VERSION 1.0
    # No ENABLE_TYPE_COMPILER here: the screens instantiate the kit's types
    # (AppBar, AppButton, BoardThumb) from the WordJewelsSolver module, and one
    # uses a ListModel, neither of which qmltc can compile across modules.
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/WordJewelsSolver/Scan"
    # The kit is the application's own module, built in apps/app/src: the import path
    # is the directory above this one, where WordJewelsSolver/qmldir lives.
    IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/../.."
    QML_FILES ${WJS_SCAN_QML_SOURCES}
    SOURCES
        assist_game.cpp
        assist_game.hpp
        assist_game_factory.cpp
        assist_game_factory.hpp
        assist_model.cpp
        assist_model.hpp
        scan_model.cpp
        scan_model.hpp
        ${WJS_CAMERA_SOURCES})

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(wjs_scan_ui)

# The C++ the QML compiler writes for this module's files is built with the
# target's warnings as errors, and MSVC's code generator reports C4702
# (unreachable code) from inside Qt's own headers while compiling it. The
# application's own QML target leaves it alone for its generated files; this
# module's are told the same, and only those.
get_target_property(_wjs_scan_ui_sources wjs_scan_ui SOURCES)
foreach(_wjs_source IN LISTS _wjs_scan_ui_sources)
    if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_wjs_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

target_include_directories(wjs_scan_ui
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        # scan_model.hpp includes camera_source.hpp: a consumer of the model's
        # header needs the camera's include directory too.
        "${PROJECT_SOURCE_DIR}/apps/app/src/camera"
    PRIVATE
        # qt_text.hpp, the one place text crosses between the library and Qt.
        "${PROJECT_SOURCE_DIR}/apps/app/src")

target_link_libraries(wjs_scan_ui
    PUBLIC
        wjs::lib
        wjs::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    PRIVATE
        wjs_trace
        wjs::warnings
        wjs::hardening
        wjs::sanitizers
        wjs::coverage)

# The camera's multimedia backend is linked only where it was found: the none
# variant has no Qt Multimedia to load (apps/app/src/camera/CMakeLists.txt).
if(WJS_QT_HAS_MULTIMEDIA)
    target_link_libraries(wjs_scan_ui PRIVATE Qt6::Multimedia)
endif()
