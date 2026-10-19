pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// The jewel box: a deep tray, cream dice and a soft lid. It is 2D on purpose:
// it is the object a child sees before the shake, and the calm object on the
// resting screen. The 3D scene is the shake itself, not this picture.
//
// The dice are sized from the well, not only from the tray's width. The old
// rule made a 3 x 3 grid wider than the tray was tall, so the bottom row hung
// over the gold edge and a decorative gold line ran through the letters.
Item {
    id: root

    property bool closed: true
    property bool asleep: false
    property var tiles: ["S", "O", "S", "S", "A", "N", "N", "S", "L"]
    property bool showDice: true
    // The board size the closed box is waiting for. It changes the picture of
    // the compartments when the child chooses 4 x 4, 5 x 5 or 6 x 6.
    property int boardSize: 3

    implicitWidth: 360
    implicitHeight: 300

    readonly property int side: Math.max(1, boardSize)
    readonly property real trayWidth: Math.min(width * 0.90, height * 1.16)
    readonly property real trayHeight: trayWidth * 0.78
    readonly property real spacing: Math.max(3, trayWidth * 0.016)
    // The largest square grid that fits inside the tray's padded well.
    readonly property real cell: Math.max(1, (Math.min(trayWidth, trayHeight) * 0.76
                                               - (side - 1) * spacing) / side)

    Rectangle {
        // The shadow under the box.
        x: (root.width - root.trayWidth) / 2 + 10
        y: (root.height - root.trayHeight) / 2 + 16
        width: root.trayWidth
        height: root.trayHeight
        radius: Theme.radiusLarge
        color: "#0A1113"
        opacity: 0.18
    }

    Rectangle {
        id: tray
        x: (root.width - root.trayWidth) / 2
        y: (root.height - root.trayHeight) / 2
        width: root.trayWidth
        height: root.trayHeight
        radius: Theme.radiusLarge
        color: Theme.tray
        border.width: 3
        border.color: Theme.gold

        Rectangle {
            anchors.fill: parent
            anchors.margins: 8
            radius: Theme.radiusLarge - 4
            color: Theme.well
            border.width: 1
            border.color: "#2A4A52"
        }

        // The closed box shows the compartments of the size it is set to, so
        // choosing 6 x 6 really does change the object on the left.
        Grid {
            visible: root.closed
            anchors.centerIn: parent
            columns: root.side
            spacing: root.spacing
            opacity: 0.55
            Repeater {
                model: root.side * root.side
                Rectangle {
                    width: root.cell
                    height: width
                    radius: Theme.radiusSmall
                    color: "#1F3F49"
                    border.width: 1
                    border.color: "#2A4A52"
                }
            }
        }

        Grid {
            visible: root.showDice && !root.closed
            anchors.centerIn: parent
            columns: Math.max(1, Math.min(root.side, root.tiles.length))
            spacing: root.spacing
            Repeater {
                model: root.tiles
                TileFace {
                    required property string modelData
                    width: root.cell
                    height: width
                    letters: modelData
                }
            }
        }
    }

    // The lid: a cream slab with a gold rim, hinged at the back. It rests over
    // the top of the tray; the wells below stay visible, so a closed box reads
    // as a box rather than a blank screen.
    Rectangle {
        id: lid
        visible: root.closed
        x: tray.x - 6
        y: tray.y - 16
        width: tray.width + 12
        height: Math.max(46, tray.height * 0.28)
        radius: Theme.radius
        color: Theme.lid
        border.width: 3
        border.color: Theme.gold
        rotation: -2.5
        transformOrigin: Item.Center

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: parent.height * 0.28
            radius: parent.radius
            color: "#FFFFFF"
            opacity: 0.22
        }

        // The box's jewel mark.
        Jewel {
            visible: !root.asleep
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.16, 50)
            height: width
            tier: "power"
            color: Theme.gold
        }

        // A small gold latch on the lid's leading edge.
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            width: 34
            height: 7
            radius: 3
            color: Theme.gold
        }

        // Two closed eyes, when the box is asleep.
        Row {
            visible: root.asleep
            anchors.centerIn: parent
            spacing: parent.width * 0.08
            Repeater {
                model: 2
                Rectangle {
                    width: parent.parent.width * 0.06
                    height: 3
                    radius: 2
                    color: Theme.dieEdge
                }
            }
        }
    }

    // A thin gold reflection under the tray's leading edge, never across a
    // letter.
    Rectangle {
        x: tray.x + 16
        y: tray.y + tray.height + 3
        width: tray.width - 32
        height: 3
        radius: 2
        color: Theme.gold
        opacity: 0.4
    }
}
