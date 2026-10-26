// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell
import "PixelArt.js" as PixelArt

// Pixel icons on a 12 x 12 grid (PixelArt.js), one colour, taking a colour token. A picture, never
// text: the app's buttons for a child say what they do with a drawing. The navigation arrows (back,
// forward) turn round when the layout is mirrored; nothing else does.
Item {
    id: root

    property string name: ""
    property color color: Theme.ink

    readonly property bool mirrorsInRtl: name === "back" || name === "forward"
    // The attached property belongs to an Item, so it is read here.
    readonly property bool layoutMirrored: Theme.rightToLeft

    // The drawing grid; the item scales it to whatever size it is given.
    readonly property real grid: 12

    implicitWidth: 24
    implicitHeight: 24

    // The picture's cells as an SVG path ("" when there is no such icon).
    function pathFor(icon) {
        const rows = PixelArt.icons[icon];
        return rows === undefined ? "" : PixelArt.path(rows, "#");
    }

    PixelPicture {
        anchors.fill: parent
        rows: PixelArt.icons[root.name] === undefined ? [] : PixelArt.icons[root.name]
        colors: ({ "#": root.color })
        transform: Scale {
            origin.x: root.width / 2
            xScale: root.mirrorsInRtl && root.layoutMirrored ? -1 : 1
        }
    }
}
