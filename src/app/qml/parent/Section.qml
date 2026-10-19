import QtQuick
import QtQuick.Layouts

// A titled block of a parent screen. The title is a heading for assistive
// technology.
ColumnLayout {
    id: root

    property string title
    default property alias content: body.data

    spacing: ParentTheme.space3

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: ParentTheme.line
        visible: root.title.length > 0
    }

    Text {
        Layout.fillWidth: true
        visible: root.title.length > 0
        text: root.title
        font.family: ParentTheme.uiFont
        font.pixelSize: ParentTheme.sizeLead
        font.weight: Font.ExtraBold
        color: ParentTheme.ink
        wrapMode: Text.WordWrap
        Accessible.role: Accessible.Heading
        Accessible.name: root.title
    }

    ColumnLayout {
        id: body
        Layout.fillWidth: true
        spacing: ParentTheme.space2
    }
}
