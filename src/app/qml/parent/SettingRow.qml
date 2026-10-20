import QtQuick
import QtQuick.Layouts

// A name, what it does, and the control: side by side where there is room, one
// above the other where there is not.
Rectangle {
    id: root

    property string title
    property string hint
    default property alias control: slot.data

    readonly property bool narrow: width < 560

    implicitHeight: layout.implicitHeight + 2 * ParentTheme.space4
    radius: ParentTheme.radius
    color: ParentTheme.surface
    border.width: 1
    border.color: ParentTheme.line

    GridLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: ParentTheme.space4
        columns: root.narrow ? 1 : 2
        columnSpacing: ParentTheme.space6
        rowSpacing: ParentTheme.space2

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: root.narrow ? root.width : root.width * 0.4
            spacing: ParentTheme.space1
            Text {
                Layout.fillWidth: true
                text: root.title
                font.family: ParentTheme.uiFont
                font.pixelSize: ParentTheme.sizeBody
                font.weight: Font.DemiBold
                color: ParentTheme.ink
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: root.hint.length > 0
                text: root.hint
                font.family: ParentTheme.uiFont
                font.pixelSize: ParentTheme.sizeSmall
                color: ParentTheme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            id: slot
            Layout.fillWidth: true
            Layout.preferredWidth: root.narrow ? root.width : root.width * 0.6
            Layout.alignment: Qt.AlignVCenter
            spacing: ParentTheme.space2
        }
    }
}
