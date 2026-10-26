// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import SnapAndSpell

// The well of the tray, Toybox: a teal checker with one cell behind each die (DESIGN.md, section 6).
// It shows where a die is missing - the box before the drop, a die that has not landed - and a fringe
// round the dice. FlatBoard and the board scene both draw it, at the tiles' own positions, so the
// board does not change as the scene hands over to the flat board.
Item {
    id: root

    property int rows: 4
    property int columns: 4
    property real tile: 1
    property real pitch: 1

    Repeater {
        model: root.rows * root.columns

        Rectangle {
            required property int index
            readonly property int row: Math.floor(index / root.columns)
            readonly property int column: index % root.columns

            x: column * root.pitch
            y: row * root.pitch
            width: root.tile
            height: root.tile
            radius: Theme.radiusXs
            color: (row + column) % 2 === 0 ? Theme.well : Theme.wellCell
        }
    }
}
