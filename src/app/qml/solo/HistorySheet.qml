pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// Earlier boards, newest first, over whatever screen opened it. It is the child's
// to open on purpose and is never offered after a board. Back, the close button
// and a press on the scrim all close it; nothing in it can be lost or changed.
Item {
    id: root

    required property var solo
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
        Accessible.name: qsTr("Earlier boards")

        AppButton {
            id: close
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.space2
            iconName: "close"
            accessibleName: qsTr("Close")
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
            model: root.solo.history

            delegate: Item {
                required property var modelData
                width: grid.cellWidth
                height: grid.cellHeight
                BoardThumb {
                    anchors.centerIn: parent
                    width: Theme.thumbSheet
                    height: Theme.thumbSheet + jewelRow
                    rows: parent.modelData.rows
                    columns: parent.modelData.columns
                    tiles: parent.modelData.tiles
                    found: parent.modelData.found
                }
            }
        }

        // An empty album says nothing: a still picture of an empty tray.
        BoardThumb {
            visible: root.solo.history.length === 0
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
