// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell

// The face of a die, Toybox: a cream tile with a 3 px ink outline, a hard 4 px shadow and an Andika
// letter in ink (DESIGN.md, section 7, "Tile states"). The states are shapes as well as colours:
//
//   on the path   coral, sunk 3 px onto its shadow (which it loses), and its place on the path in a
//                 disc on the corner
//   start / next  a help: sunshine, with a play mark (the start tile) or a square (the next letter)
//   valid         a word that was found: leaf, sunk, with a check
//   invalid       not a word: a 4 px pink outline and a cross
//   doubtful      the scanner's unsure letter: a question mark
//
// The path never crosses the letter, and the letter is ink on every fill.
Item {
    id: root

    property string letters: ""
    property bool lit: false
    // The die's place on the path, 1 for the first: shown in a disc when it is lit.
    property int order: 0
    // 0 for none, 1 for the start tile of a help, 2 for the next letter.
    property int help: 0
    property bool valid: false
    property bool invalid: false
    property bool doubtful: false

    // Sunk onto its shadow: on the path, or a word that was found.
    readonly property bool sunk: lit || valid

    readonly property string shown: letters === "QU" ? "Qu" : letters

    // The hard shadow, under the face; a die on the path has sunk onto it.
    Rectangle {
        visible: !root.sunk
        y: Theme.shadowMedium
        width: parent.width
        height: parent.height
        radius: Theme.radiusControl
        color: Theme.shadow
    }

    Rectangle {
        objectName: "face"
        y: root.sunk ? Theme.shadowSmall : 0
        width: parent.width
        height: parent.height
        radius: Theme.radiusControl
        color: root.valid ? Theme.leaf
             : root.lit ? Theme.dieLit
             : root.help > 0 ? Theme.sunshine : Theme.die
        border.width: root.invalid ? Theme.outlinePanel : Theme.outlineControl
        border.color: root.invalid ? Theme.danger : Theme.dieEdge
    }

    Label {
        objectName: "letter"
        anchors.centerIn: parent
        anchors.verticalCenterOffset: root.sunk ? Theme.shadowSmall : 0
        decorative: true
        text: root.shown
        font.family: Theme.fontFamily
        font.weight: Font.Bold
        size: root.height * Theme.letterShare * (root.letters === "QU" ? Theme.quShare : 1)
        lineRatio: 1.0
        color: Theme.dieInk
    }

    // What this die is, in a disc on a corner: its place on the path, or the help, or the result.
    TileMark {
        y: root.sunk ? Theme.shadowSmall - size * 0.25 : -size * 0.25
        kind: root.valid ? "valid"
            : root.invalid ? "invalid"
            : root.doubtful ? "doubtful"
            : root.help === 1 ? "start"
            : root.help === 2 ? "second"
            : (root.lit && root.order > 0) ? "order" : ""
        number: root.order
    }
}
