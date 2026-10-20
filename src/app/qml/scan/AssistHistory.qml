pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// The Family Assistant's earlier boards, newest first, over `assist.playing`
// (docs/design/game.md, "The panel": the same history as Solo Play's). It is the
// child's to open on purpose; Back, the close button and a press on the scrim all
// close it, and nothing in it can be lost or changed.
//
// The boards are the store's, as `AssistModel::history` gives them; a board in
// play is not in the list until the round is left. The picture is BoardThumb, the
// same one Solo Play's history uses; every sentence is ScanWording's.
Item {
    id: root

    required property AssistModel assist
    property bool open: false

    signal closed

    visible: open
    anchors.fill: parent
    z: 20

    function close() {
        root.open = false;
        root.closed();
    }

    Keys.onEscapePressed: root.close()
    onOpenChanged: {
        if (open) {
            forceActiveFocus();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    Rectangle {
        id: sheet
        readonly property real margin: Theme.space6
        width: Math.min(root.width - 2 * margin, Theme.sheetMaxWidth)
        height: Math.min(root.height - 2 * margin, Theme.sheetMaxHeight)
        anchors.centerIn: parent
        color: Theme.page
        border.width: Theme.rule
        border.color: Theme.outline

        Accessible.role: Accessible.Pane
        Accessible.name: ScanWording.earlierBoards

        AppButton {
            id: close
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.space2
            size: "min"
            iconName: "close"
            accessibleName: ScanWording.closePicker
            onClicked: root.close()
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: Theme.space4
            anchors.topMargin: Theme.barHeight + Theme.space4
            clip: true
            cellWidth: Theme.thumbSheetCell
            cellHeight: Theme.thumbSheetCell + Theme.space6
            model: root.assist.history

            delegate: Item {
                id: thumbCell
                required property var modelData

                width: grid.cellWidth
                height: grid.cellHeight

                BoardThumb {
                    anchors.centerIn: parent
                    width: Theme.thumbSheet
                    height: Theme.thumbSheet + jewelRow
                    rows: thumbCell.modelData.rows
                    columns: thumbCell.modelData.columns
                    tiles: thumbCell.modelData.tiles
                    found: thumbCell.modelData.found
                }
            }
        }

        // An empty history says nothing: a still picture of an empty tray.
        BoardThumb {
            visible: root.assist.history.length === 0
            anchors.centerIn: parent
            width: Theme.thumbSheetCell
            height: Theme.thumbSheetCell
            rows: 3
            columns: 3
            tiles: []
            showJewels: false
        }
    }
}
