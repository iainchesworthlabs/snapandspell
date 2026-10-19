pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// The box's panel: choose a board, shake it, and see how the game works. The
// sizes are words and pictures together, not four near-identical grids.
Item {
    id: root

    required property var solo
    required property var layoutRule
    required property bool wide

    signal shakeRequested

    readonly property var offered: solo.sizes.filter(size => size <= layoutRule.largestOffered)
    readonly property bool mirrored: Theme.rightToLeft

    component SizeCard: Rectangle {
        id: choice
        required property int boardSize
        property bool checkable: true
        property bool checked: root.solo.boardSize === boardSize
        width: 88
        height: 88
        radius: Theme.radius
        color: choice.checked ? Theme.accentSoft : Theme.card
        border.width: choice.checked ? 3 : 1
        border.color: choice.checked ? Theme.accent : Theme.line
        activeFocusOnTab: true

        Accessible.role: Accessible.RadioButton
        Accessible.name: SoloWording.sizeBoard(choice.boardSize)
        Accessible.checkable: true
        Accessible.checked: choice.checked
        Accessible.onPressAction: root.solo.chooseSize(choice.boardSize)

        Keys.onSpacePressed: root.solo.chooseSize(choice.boardSize)
        Keys.onReturnPressed: root.solo.chooseSize(choice.boardSize)

        MouseArea {
            anchors.fill: parent
            onClicked: {
                Feedback.cue("speak", SoloWording.sizeBoard(choice.boardSize));
                root.solo.chooseSize(choice.boardSize);
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 4
            Grid {
                anchors.horizontalCenter: parent.horizontalCenter
                columns: choice.boardSize
                spacing: 2
                Repeater {
                    model: choice.boardSize * choice.boardSize
                    Rectangle {
                        width: Math.max(2, (choice.width * 0.38 - (choice.boardSize - 1) * 2) / choice.boardSize)
                        height: width
                        radius: 1
                        color: Theme.die
                        border.width: 1
                        border.color: Theme.dieEdge
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: choice.boardSize + " × " + choice.boardSize
                font.family: Theme.uiFont
                font.pixelSize: Theme.labelSize
                font.weight: Font.DemiBold
                color: Theme.ink
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
        radius: 0
    }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.space6
        spacing: Theme.space4

        Column {
            width: parent.width
            spacing: 2
            Text {
                text: qsTr("Choose a board")
                font.family: Theme.uiFont
                font.pixelSize: Theme.headingSize
                font.weight: Font.ExtraBold
                color: Theme.ink
            }
            Text {
                width: parent.width
                text: qsTr("3 × 3 is a good start. Bigger boards hold more words.")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.bodySize
                color: Theme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        Flow {
            width: parent.width
            spacing: Theme.space2
            Repeater {
                model: root.offered
                SizeCard {
                    required property int modelData
                    boardSize: modelData
                }
            }
        }

        AppButton {
            width: parent.width
            variant: "primary"
            size: "primary"
            iconName: "shake"
            label: SoloWording.shake
            accessibleName: SoloWording.shake
            speaks: SoloWording.shake
            onClicked: root.shakeRequested()
        }

        Rectangle {
            width: parent.width
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.line
            implicitHeight: howTo.implicitHeight + 2 * Theme.space4

            Column {
                id: howTo
                anchors.fill: parent
                anchors.margins: Theme.space4
                spacing: Theme.space2

                Text {
                    text: qsTr("How it works")
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.labelSize
                    font.weight: Font.DemiBold
                    color: Theme.ink
                }
                Text {
                    text: qsTr("1   Shake the box.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                }
                Text {
                    text: qsTr("2   Find a word.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                }
                Text {
                    text: qsTr("3   Collect the jewels.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                }
            }
        }

        Item { width: 1; height: 1 }
    }
}
