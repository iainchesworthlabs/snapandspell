import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// A name, what it does, and the control: side by side where there is room, one
// above the other where there is not.
Rectangle {
    id: root

    property string title
    property string hint
    default property alias control: slot.data

    readonly property bool narrow: width < 560

    implicitHeight: layout.implicitHeight + 2 * Theme.space4
    radius: Theme.radius
    color: Theme.surface
    border.width: 1
    border.color: Theme.line

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
            Text {
                Layout.fillWidth: true
                text: root.title
                font.family: Theme.uiFont
                font.pixelSize: Theme.bodySize
                font.weight: Font.DemiBold
                color: Theme.ink
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: root.hint.length > 0
                text: root.hint
                font.family: Theme.uiFont
                font.pixelSize: Theme.smallSize
                color: Theme.inkSecondary
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
