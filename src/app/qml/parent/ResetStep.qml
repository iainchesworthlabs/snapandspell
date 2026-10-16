import QtQuick
import QtQuick.Layouts

// One of the parent area's two resets (docs/design/dashboard.md, "Reset"): a
// button that names what it clears, and, once pressed, says what it will do and
// asks for a second, deliberate press. Nothing is cleared by the first.
ColumnLayout {
    id: root

    property string label
    property string says
    property bool done: false

    signal confirmed

    property bool asking: false

    spacing: ParentTheme.space2

    PButton {
        visible: !root.asking
        text: root.label
        onClicked: {
            root.done = false;
            root.asking = true;
        }
    }

    Rectangle {
        Layout.fillWidth: true
        visible: root.asking
        implicitHeight: asked.implicitHeight + 2 * ParentTheme.space3
        radius: ParentTheme.radius
        color: ParentTheme.note
        border.width: ParentTheme.hair * 2
        border.color: ParentTheme.outline

        ColumnLayout {
            id: asked
            anchors.fill: parent
            anchors.margins: ParentTheme.space3
            spacing: ParentTheme.space3

            Text {
                Layout.fillWidth: true
                text: root.says
                font.pixelSize: ParentTheme.sizeBody
                color: ParentTheme.noteInk
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            Flow {
                Layout.fillWidth: true
                spacing: ParentTheme.space2

                PButton {
                    text: ParentWording.resetKeep
                    onClicked: root.asking = false
                }
                PButton {
                    text: ParentWording.resetConfirm
                    primary: true
                    Accessible.name: root.label
                    onClicked: {
                        root.asking = false;
                        root.done = true;
                        root.confirmed();
                    }
                }
            }
        }
    }

    Text {
        visible: root.done && !root.asking
        text: ParentWording.resetDone
        font.pixelSize: ParentTheme.sizeSmall
        color: ParentTheme.inkSecondary
    }
}
