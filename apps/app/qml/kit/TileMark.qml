// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import WordJewelsSolver

// The little disc on the corner of a die that says what the die is (DESIGN.md, section 7, "Tile
// states"): the order of a die on the path, the start tile and the next letter of a help, a word
// that was found, a string that is not a word, a letter the scanner is unsure of. A mark is a
// shape and a picture, never colour alone: a number, a play triangle, a square, a check, a cross
// or a question mark, in an ink-outlined disc.
//
//   order     a number, on the leading top corner (ink disc, cream numeral)
//   start     a play mark (sunshine disc), the leading top corner
//   second    a square mark (sunshine disc), the leading top corner
//   valid     a check (leaf disc), the trailing top corner
//   invalid   a cross (pink disc), the trailing top corner
//   doubtful  a question mark (sunshine disc), the trailing top corner
Item {
    id: root

    property string kind: ""
    property int number: 0

    readonly property bool trailing: kind === "valid" || kind === "invalid" || kind === "doubtful"
    // About a third of the die, never smaller than a finger-sized mark can be read at.
    readonly property real size: Math.max(Theme.space6, Math.min(Theme.space8, (parent ? parent.width : 0) * 0.3))

    visible: kind !== ""
    width: size
    height: size
    x: trailing ? (parent ? parent.width - width + size * 0.25 : 0) : -size * 0.25
    y: -size * 0.25
    // The die's own name says what it is; the mark is a picture of it.
    Accessible.ignored: true

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: root.kind === "order" ? Theme.ink
             : root.kind === "valid" ? Theme.leaf
             : root.kind === "invalid" ? Theme.danger
             : Theme.sunshine
        border.width: Theme.outlineControl
        border.color: Theme.dieEdge
    }

    Label {
        visible: root.kind === "order"
        anchors.centerIn: parent
        decorative: true
        role: "numeral"
        text: root.number
        size: root.size * 0.5
        lineRatio: 1.0
        color: Theme.buttonInk
    }

    Icon {
        visible: root.kind !== "order" && root.kind !== ""
        anchors.centerIn: parent
        width: root.size * 0.62
        height: width
        name: root.kind === "valid" ? "done"
            : root.kind === "invalid" ? "close"
            : root.kind === "start" ? "play"
            : root.kind === "second" ? "square" : "question"
        color: Theme.dieEdge
    }
}
