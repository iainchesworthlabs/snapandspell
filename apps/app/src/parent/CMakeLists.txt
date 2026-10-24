# The parent area (docs/design/dashboard.md): the gate, the dashboard, the
# settings and the about page, as a QML module of their own, so that the
# application's own CMake file has only to link it. The rules are the library's
# (wjs/parent_gate.hpp, store.hpp, dashboard.hpp); the controller here is the
# shape Qt Quick wants and the screens are in ../../qml/parent.
#
# A screen of the application, imported as
#
#   import WordJewelsSolver.Parent
#   ParentArea { controller: ...; onClosed: ... }
#
# Qt is found by the top-level CMakeLists.txt, and qt_standard_project_setup
# has been called by apps/app/CMakeLists.txt, which adds this directory.

# The module is made here, so what the application's own file sets for its target
# is set again for this one: moc over the controller, and the default resource
# prefix (":/qt/qml/") that Qt's policy QTP0001 names.
set(CMAKE_AUTOMOC ON)
qt_policy(SET QTP0001 NEW)

set(WJS_PARENT_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../qml/parent")

set(WJS_PARENT_QML_FILES
    AboutScreen.qml
    DashboardScreen.qml
    GateScreen.qml
    LicencesScreen.qml
    ParentArea.qml
    ParentWording.qml
    ResetStep.qml
    Section.qml
    SettingRow.qml
    SettingsScreen.qml)

# The module's files are in another directory than this CMake file, so each is
# given the name it has in the module.
set(WJS_PARENT_QML_SOURCES "")
foreach(_file IN LISTS WJS_PARENT_QML_FILES)
    set_source_files_properties("${WJS_PARENT_QML_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_PARENT_QML_SOURCES "${WJS_PARENT_QML_DIR}/${_file}")
endforeach()

# The wording is the one singleton of this module: the tokens are the kit's
# (Theme, in the application's own module).
set_source_files_properties(
    "${WJS_PARENT_QML_DIR}/ParentWording.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(wjs_parent
    STATIC
    URI WordJewelsSolver.Parent
    VERSION 1.0
    # No ENABLE_TYPE_COMPILER here: every screen sets LayoutMirroring and reads
    # the translated wording singleton, which the shared kit's qmltc cannot link
    # or translate; even the small components misbehaved under it in
    # parent.screens (flickering option labels, an engine crash on gate
    # teardown). The parent area is behind the gate and opened rarely, so its
    # first show is the acceptable place for this cost; the child-facing kit
    # and screens are the compiled ones.
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/WordJewelsSolver/Parent"
    # The screens use the kit's tokens and components (Theme, AppBar, AppButton,
    # SegmentedChoice), which are the application's own module, built in apps/app/src:
    # the import path is the directory above this one, where WordJewelsSolver/qmldir
    # lives, as for the scan module. The application imports this module back; QML
    # resolves the two at run time from their resources.
    IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}/../.."
    QML_FILES ${WJS_PARENT_QML_SOURCES}
    SOURCES
        parent_controller.cpp
        parent_controller.hpp)

target_include_directories(wjs_parent
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        # qt_text.hpp, the one place text crosses between the library and Qt.
        "${PROJECT_SOURCE_DIR}/apps/app/src")

target_link_libraries(wjs_parent
    PUBLIC
        wjs::lib
        wjs::platform
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
