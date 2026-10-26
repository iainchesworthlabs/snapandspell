// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtQml/qqmlextensionplugin.h>

// The QML modules the application imports, linked into the .wasm. Qt's own finalisation would find
// them by scanning every .qml file under apps/app, and that scan also finds what the web build
// leaves out on purpose: Qt Quick 3D (the 3D scene's, which is not built here), Qt Test (the UI
// suites'), Qt Multimedia (the camera's) and the other four Qt Quick Controls styles. Each of those
// is megabytes of WebAssembly that a child waits for, so the scan is off (platform.cmake,
// QT_QML_MODULE_NO_IMPORT_SCAN) and the modules are named here and in WJS_WEB_QML_PLUGIN_TARGETS.
//
// tests/platform/check_web.cmake holds this list to the imports of the application's QML: a module
// imported and not named here is a build that fails the check, not a page that fails to start.

// Qt's.
Q_IMPORT_QML_PLUGIN(QtQmlPlugin)
Q_IMPORT_QML_PLUGIN(QtQmlModelsPlugin)
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QtQuick_WindowPlugin)
Q_IMPORT_QML_PLUGIN(QtQuickLayoutsPlugin)
Q_IMPORT_QML_PLUGIN(QmlShapesPlugin)
Q_IMPORT_QML_PLUGIN(QtQuickTemplates2Plugin)
Q_IMPORT_QML_PLUGIN(QtQuickControls2Plugin)
Q_IMPORT_QML_PLUGIN(QtQuickControls2ImplPlugin)
Q_IMPORT_QML_PLUGIN(QtQuickControls2BasicStylePlugin)
Q_IMPORT_QML_PLUGIN(QtQuickControls2BasicStyleImplPlugin)

// The project's: the parent area, the Family Assistant's screens and the board scene.
Q_IMPORT_QML_PLUGIN(WordJewelsSolver_ParentPlugin)
Q_IMPORT_QML_PLUGIN(WordJewelsSolver_ScanPlugin)
Q_IMPORT_QML_PLUGIN(WordJewelsSolver_ScenePlugin)
