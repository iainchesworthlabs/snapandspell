import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

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

    spacing: Theme.space2

    AppButton {
        size: "min"
        quiet: true
        visible: !root.asking
        label: root.label
        onClicked: {
            root.done = false;
            root.asking = true;
        }
    }

    Rectangle {
        Layout.fillWidth: true
        visible: root.asking
        implicitHeight: asked.implicitHeight + 2 * Theme.space3
        radius: Theme.radius
        color: Theme.hintCard
        border.width: Theme.hair * 2
        border.color: Theme.outline

        ColumnLayout {
            id: asked
            anchors.fill: parent
            anchors.margins: Theme.space3
            spacing: Theme.space3

            Text {
                Layout.fillWidth: true
                text: root.says
                font.pixelSize: Theme.bodySize
                color: Theme.hintCardInk
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.space2

                AppButton {
                    size: "min"
                    quiet: true
                    label: ParentWording.resetKeep
                    onClicked: root.asking = false
                }
                AppButton {
                    size: "min"
                    quiet: true
                    label: ParentWording.resetConfirm
                    variant: "primary"
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
        font.pixelSize: Theme.smallSize
        color: Theme.inkSecondary
    }
}
