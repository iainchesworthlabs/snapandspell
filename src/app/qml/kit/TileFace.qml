import QtQuick
import WordJewelsSolver

// The face of a die: a cream tile with an Andika letter, a soft bevel and a dark
// edge. On the path it keeps its face and letter; a gold rim, a slight lift and
// a small order badge show that it is selected. Lit is a shape as well as a
// colour, and the path never crosses the letter.
Item {
    id: root

    property string letters: ""
    property bool lit: false
    // Kept for callers that still pass it, but the numbered badge is gone: the
    // path itself shows the order, and a number on a five-year-old's die was
    // harder to read than the line it replaced.
    property int order: 0

    readonly property string shown: letters === "QU" ? "Qu" : letters

    Rectangle {
        objectName: "face"
        anchors.fill: parent
        radius: Theme.radiusSmall
        color: Theme.die
        border.width: root.lit ? Theme.litEdge : Theme.rule
        border.color: root.lit ? Theme.gold : Theme.dieEdge
    }

    // The top of the die catches the light; the foot sits in its shadow.
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 2
        height: 2
        radius: 1
        color: "#FFFFFF"
        opacity: 0.55
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2
        height: 3
        radius: 1
        color: "#000000"
        opacity: 0.12
    }

    Text {
        objectName: "letter"
        anchors.centerIn: parent
        text: root.shown
        font.family: Theme.fontFamily
        font.weight: Font.Bold
        font.pixelSize: root.height * Theme.letterShare * (root.letters === "QU" ? Theme.quShare : 1)
        color: Theme.dieInk
    }

}
