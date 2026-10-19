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
    property int order: 0

    readonly property string shown: letters === "QU" ? "Qu" : letters

    scale: lit ? Theme.litScale : 1
    Behavior on scale {
        NumberAnimation {
            duration: Theme.quick
            easing.type: Easing.OutCubic
        }
    }

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

    Rectangle {
        visible: root.lit && root.order > 0
        width: Math.max(18, root.height * 0.15)
        height: width
        radius: width / 2
        x: Theme.space1
        y: Theme.space1
        color: Theme.gold
        border.width: 1
        border.color: Theme.tray
        Text {
            anchors.centerIn: parent
            text: root.order
            font.family: Theme.monoFont
            font.pixelSize: Math.max(9, parent.height * 0.55)
            font.weight: Font.DemiBold
            color: Theme.tray
        }
    }
}
