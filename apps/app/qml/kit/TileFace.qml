// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import WordJewelsSolver

// The face of a die, Toybox: a cream tile with a 3 px ink outline, a hard 4 px shadow and an Andika
// letter in ink. On the path it turns coral, sinks 3 px onto its shadow and loses it, so lit is a shape
// as well as a colour, and the path never crosses the letter.
Item {
    id: root

    property string letters: ""
    property bool lit: false
    // Kept for callers that still pass it, but the numbered badge is gone: the
    // path itself shows the order, and a number on a five-year-old's die was
    // harder to read than the line it replaced.
    property int order: 0

    readonly property string shown: letters === "QU" ? "Qu" : letters

    // The hard shadow, under the face; a die on the path has sunk onto it.
    Rectangle {
        visible: !root.lit
        y: Theme.shadowMedium
        width: parent.width
        height: parent.height
        radius: Theme.radiusControl
        color: Theme.shadow
    }

    Rectangle {
        objectName: "face"
        y: root.lit ? Theme.shadowSmall : 0
        width: parent.width
        height: parent.height
        radius: Theme.radiusControl
        color: root.lit ? Theme.dieLit : Theme.die
        border.width: Theme.outlineControl
        border.color: Theme.dieEdge
    }

    Label {
        objectName: "letter"
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.lit ? Theme.shadowSmall : 0
        decorative: true
        text: root.shown
        font.family: Theme.fontFamily
        font.weight: Font.Bold
        size: root.height * Theme.letterShare * (root.letters === "QU" ? Theme.quShare : 1)
        lineRatio: 1.0
        color: Theme.dieInk
    }

}
