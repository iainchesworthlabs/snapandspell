pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// What solo.box holds besides the box itself (docs/design/game.md, "The flow"):
// the board-size choice, as two pictures, and the Shake button. It lives in the
// panel's place, so the board does not move when it gives way to the panel. There
// is no text here: a picture says each thing, and the screen reader has its names.
//
// A size is offered only if its tile is at least 48 dp in this window; one that
// does not fit is not shown, not shown disabled.
Item {
    id: root

    required property var solo
    required property var layoutRule
    required property bool wide

    signal shakeRequested

    // The sizes this window can hold at 48 dp a tile; a size that does not fit is not
    // shown, not shown disabled.
    readonly property var offered: solo.sizes.filter(size => size <= layoutRule.largestOffered)

    // The picture of a size: a small board of that many tiles.
    component SizePicture: AppButton {
        id: choice
        required property int boardSize
        size: "child"
        checkable: true
        checked: root.solo.boardSize === boardSize
        accessibleName: qsTr("%1 by %1 board").arg(boardSize)
        onClicked: root.solo.chooseSize(boardSize)

        // The picture is the board itself, in small: relative to the button.
        Rectangle {
            id: picture
            anchors.centerIn: parent
            width: choice.height * 0.62
            height: width
            color: Theme.tray
            Grid {
                anchors.fill: parent
                anchors.margins: picture.width * 0.075
                columns: choice.boardSize
                spacing: picture.width * 0.05
                Repeater {
                    model: choice.boardSize * choice.boardSize
                    Rectangle {
                        width: (picture.width * 0.85 - (choice.boardSize - 1) * picture.width * 0.05) / choice.boardSize
                        height: width
                        color: Theme.die
                    }
                }
            }
        }
    }

    // Wide: the Shake button across the panel with the sizes beneath it. Tall: the
    // sizes beside it, the button taking the rest, a third of the width at least.
    Row {
        id: sizes
        spacing: Theme.targetGap
        x: root.wide ? (root.width - width) / 2 : Theme.space4
        y: root.wide ? shake.y + shake.height + Theme.space4
                     : (root.height - height) / 2
        Repeater {
            model: root.offered
            SizePicture {
                required property int modelData
                boardSize: modelData
            }
        }
    }

    AppButton {
        id: shake
        variant: "primary"
        size: "primary"
        iconName: "shake"
        accessibleName: qsTr("Shake")
        speaks: "shake"
        x: root.wide ? Theme.space4 : sizes.x + sizes.width + Theme.space4
        y: root.wide ? (root.height - height - Theme.targetChild - Theme.space4) / 2
                     : (root.height - height) / 2
        width: root.wide ? root.width - 2 * Theme.space4
                         : Math.max(root.width / 3, root.width - x - Theme.space4)
        onClicked: root.shakeRequested()
    }
}
