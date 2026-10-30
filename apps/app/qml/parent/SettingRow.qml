// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

// A name, what it does, and the control: side by side where there is room, one
// above the other where there is not. A row of a SettingGroup's panel: no edge of its own, a dashed
// line above it that divides it from the row before; the name and the hint are Labels.
Item {
    id: root

    property string title
    property string hint
    default property alias control: slot.data

    readonly property bool narrow: width < 560

    implicitHeight: layout.implicitHeight + 2 * Theme.space4

    // The dashed line above the row.
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: Theme.outlinePanel
        spacing: Theme.space2
        Accessible.ignored: true

        Repeater {
            model: Math.ceil((root.width - 2 * Theme.outlinePanel) / (Theme.space3 + Theme.space2))

            Rectangle {
                width: Theme.space3
                height: Theme.outlineControl
                color: Theme.surfaceDim
            }
        }
    }

    GridLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.space4
        columns: root.narrow ? 1 : 2
        columnSpacing: Theme.space6
        rowSpacing: Theme.space2

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: root.narrow ? root.width : root.width * 0.4
            spacing: Theme.space1
            Label {
                Layout.fillWidth: true
                role: "label"
                text: root.title
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                visible: root.hint.length > 0
                role: "caption"
                text: root.hint
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            id: slot
            Layout.fillWidth: true
            Layout.preferredWidth: root.narrow ? root.width : root.width * 0.6
            Layout.alignment: Qt.AlignVCenter
            spacing: Theme.space2
        }
    }
}
