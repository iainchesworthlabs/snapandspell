// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes
import SnapAndSpell
import "PixelArt.js" as PixelArt

// A picture drawn in cells (PixelArt.js, written by tools/design/make_pixel_art.py): one coloured layer
// for each of the characters '#' (an icon), o, f, h and s (a gem), taking its colour from `colors`. The picture is as large as
// the item allows with every cell the same size, and centred in it.
Item {
    id: root

    // The grid: an array of strings, one per row.
    property var rows: []
    // character -> colour; a character with no colour is not drawn.
    property var colors: ({})

    function colorOf(ch) {
        return colors[ch] === undefined ? "transparent" : colors[ch];
    }

    readonly property int columns: rows.length > 0 ? rows[0].length : 0
    readonly property real cell: columns > 0 ? Math.min(width / columns, height / rows.length) : 0

    implicitWidth: columns * 2
    implicitHeight: rows.length * 2
    // A picture; the control that holds it says what it is.
    Accessible.ignored: true

    Shape {
        width: root.columns
        height: root.rows.length
        scale: root.cell
        transformOrigin: Item.TopLeft
        x: (root.width - root.columns * root.cell) / 2
        y: (root.height - root.rows.length * root.cell) / 2
        // Whole cells have hard edges: no curve renderer, no smoothing.
        antialiasing: false

        ShapePath {
            fillColor: root.colorOf("#")
            strokeColor: "transparent"
            strokeWidth: 0
            PathSvg { path: PixelArt.path(root.rows, "#") }
        }
        ShapePath {
            fillColor: root.colorOf("o")
            strokeColor: "transparent"
            strokeWidth: 0
            PathSvg { path: PixelArt.path(root.rows, "o") }
        }
        ShapePath {
            fillColor: root.colorOf("f")
            strokeColor: "transparent"
            strokeWidth: 0
            PathSvg { path: PixelArt.path(root.rows, "f") }
        }
        ShapePath {
            fillColor: root.colorOf("h")
            strokeColor: "transparent"
            strokeWidth: 0
            PathSvg { path: PixelArt.path(root.rows, "h") }
        }
        ShapePath {
            fillColor: root.colorOf("s")
            strokeColor: "transparent"
            strokeWidth: 0
            PathSvg { path: PixelArt.path(root.rows, "s") }
        }
    }
}
