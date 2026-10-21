import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// A titled block of a parent screen. The title is a heading for assistive
// technology.
ColumnLayout {
    id: root

    property string title
    default property alias content: body.data

    spacing: Theme.space3

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: Theme.line
        visible: root.title.length > 0
    }

    Text {
        Layout.fillWidth: true
        visible: root.title.length > 0
        text: root.title
        font.family: Theme.uiFont
        font.pixelSize: Theme.headingSize
        font.weight: Font.ExtraBold
        color: Theme.ink
        wrapMode: Text.WordWrap
        Accessible.role: Accessible.Heading
        Accessible.name: root.title
    }

    ColumnLayout {
        id: body
        Layout.fillWidth: true
        spacing: Theme.space2
    }
}
