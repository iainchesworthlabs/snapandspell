// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell
import "PixelArt.js" as PixelArt

// A found word's jewel, in pixels: an ink outline, a fill, a highlight at the top left and a shade at
// the bottom right. Its tier is its shape and its size (a diamond, a cut gem, a star); its colour is
// decoration and says nothing (docs/design/application.md, "Colour and contrast"). No text is drawn
// into it.
Item {
    id: root

    property string tier: "spark"          // spark | power | mega
    property int colorIndex: 0
    property color color: Theme.jewelColors[colorIndex % Theme.jewelColors.length]

    readonly property real diameter: tier === "mega" ? Theme.jewelMega
                                   : tier === "power" ? Theme.jewelPower
                                   : Theme.jewelSpark

    implicitWidth: diameter
    implicitHeight: diameter

    PixelPicture {
        anchors.fill: parent
        rows: PixelArt.gems[root.tier] === undefined ? PixelArt.gems.spark : PixelArt.gems[root.tier]
        colors: ({
            "o": Theme.deepPurple,
            "f": root.color,
            "h": Qt.lighter(root.color, 1.45),
            "s": Qt.darker(root.color, 1.3)
        })
    }
}
