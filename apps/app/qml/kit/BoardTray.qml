// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell

// The tray the board stands in, Toybox: the deep tray colour, a 6 px outline, a hard 12 px drop shadow
// and, when asked for, the darker well inside it. FlatBoard, the jewel box, the scan
// viewfinder and Mode A's assist board all draw this one object, so the board
// is the same toy wherever it appears.
//
// The geometry is the caller's - set x, y, width and height. For a tile board
// the tray surrounds the tiles by `Theme.trayOutset(tileSize, limit)`: pass the
// board area's inset (apps/app/src/solo/board_layout.cpp, board_metrics().inset:
// 12 below a 600 px area and 24 above) as `limit` so the tray's rim cannot
// overflow its area. Without the limit the outset is max(10, 8% of the tile),
// which is 2 to 6 px more than the inset allows (the clipped tray at 900x620).
//
// The default content is drawn inside the tray, above the well.
Rectangle {
    id: root

    // The gold rim; the jewel box draws a heavier one than the flat board.
    property real rim: Theme.trayRim
    property bool showWell: false
    property real wellInset: Theme.trayWellInset

    default property alias content: inner.data

    radius: Theme.radiusLarge
    color: Theme.tray
    border.width: root.rim

    // The drop, in the shadow colour, behind the tray.
    Rectangle {
        z: -1
        y: Theme.shadowFrame
        width: parent.width
        height: parent.height
        radius: root.radius
        color: Theme.shadow
    }
    border.color: Theme.trayEdge

    Rectangle {
        visible: root.showWell
        anchors.fill: parent
        anchors.margins: root.wellInset
        radius: Theme.radiusLarge - Theme.radiusXs
        color: Theme.well
        border.width: Theme.hair
        border.color: Theme.trayLine
    }

    Item {
        id: inner
        anchors.fill: parent
    }
}
