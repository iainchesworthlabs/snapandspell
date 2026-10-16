import QtQuick
import WordJewelsSolver

// The face of a tile (docs/design/board.md, "The tile"): a square with square
// corners, flat, no shadow and no gradient. At rest a cream face with a dark
// letter and a dark edge; lit, on the path, the face turns amber, the edge thickens
// to 4 dp and the tile scales to 106%. Lit is a shape as well as a colour: a lit
// die is not told from a resting one by colour alone (their contrast is 1.33:1).
Item {
    id: root

    property string letters: ""
    property bool lit: false

    // "QU" is the Qu face: one tile of two letters, drawn smaller so it fits.
    readonly property string shown: letters === "QU" ? "Qu" : letters

    scale: lit ? Theme.litScale : 1
    Behavior on scale {
        NumberAnimation {
            duration: Theme.quick
        }
    }

    Rectangle {
        anchors.fill: parent
        color: root.lit ? Theme.dieLit : Theme.die
        border.width: root.lit ? Theme.litEdge : Theme.rule
        border.color: Theme.dieEdge
    }

    Text {
        anchors.centerIn: parent
        text: root.shown
        font.family: Theme.fontFamily
        font.bold: true
        font.pixelSize: root.height * Theme.letterShare * (root.letters === "QU" ? Theme.quShare : 1)
        color: root.lit ? Theme.dieLitInk : Theme.dieInk
    }
}
