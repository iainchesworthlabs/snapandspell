import QtQuick
import QtQuick.Shapes

// The faces of one board as one texture: for each die, a block of 3 x 2 cells,
// one cell for each of its six faces, laid out as the board is, so the die on
// the tile at row r and column c is the block at column c and row r. Side k of
// the cube is the cell at column k % 3 and row floor(k / 3) of its block (the
// mesh and ShakeScript.js's `sides` say the same). Cream face, dark letter,
// dark edge (docs/design/board.md, "The tile"). Every die samples its own
// block, and the atlas is drawn once for a board, not once for a die.
//
// A dead jewel is a cell with no letters, painted as four facets of a cut
// stone seen from above, so it reads as a jewel and not as an empty tile; a
// dead tile is a die of six of them.
Item {
    id: atlas

    // For each tile, row-major, the six strings of its die: "A", "QU", "SH",
    // or "" for a jewel.
    property var faces: []
    property int rows: 1
    property int columns: 1

    readonly property int cell: 128
    readonly property int blockColumns: 3
    readonly property int blockRows: 2
    readonly property int facesPerDie: blockColumns * blockRows
    readonly property color faceColor: "#F4E9CC"
    readonly property color edgeColor: "#2B2622"
    readonly property color inkColor: "#1F1B18"

    width: columns * blockColumns * cell
    height: rows * blockRows * cell

    Repeater {
        model: atlas.faces.length * atlas.facesPerDie

        Rectangle {
            id: face

            required property int index
            readonly property int die: Math.floor(index / atlas.facesPerDie)
            readonly property int side: index % atlas.facesPerDie
            readonly property string letters: {
                const sides = atlas.faces[die];
                return sides && sides.length > 0 ? String(sides[side % sides.length]).toUpperCase() : "";
            }

            x: ((die % atlas.columns) * atlas.blockColumns + side % atlas.blockColumns) * atlas.cell
            y: (Math.floor(die / atlas.columns) * atlas.blockRows + Math.floor(side / atlas.blockColumns)) * atlas.cell
            width: atlas.cell
            height: atlas.cell
            color: face.letters === "" ? "#2E9F92" : atlas.faceColor
            border.color: atlas.edgeColor
            border.width: 10

            // The four facets, from the middle of the cell to each edge. Made
            // only for a jewel: a board of letters has none.
            Loader {
                anchors.fill: parent
                active: face.letters === ""
                sourceComponent: facets
            }

            Text {
                anchors.centerIn: parent
                text: face.letters
                color: atlas.inkColor
                font.bold: true
                // About 55% of the tile for one letter; a two-letter tile (the
                // Qu face) at 80% of that, a three-letter tile smaller still.
                font.pixelSize: face.letters.length > 2 ? 48 : (face.letters.length === 2 ? 68 : 88)
            }
        }
    }

    Component {
        id: facets

        Shape {
            ShapePath {
                fillColor: "#63D6C7"
                strokeColor: "transparent"
                startX: 64; startY: 64
                PathLine { x: 10; y: 10 }
                PathLine { x: 118; y: 10 }
                PathLine { x: 64; y: 64 }
            }
            ShapePath {
                fillColor: "#2E9F92"
                strokeColor: "transparent"
                startX: 64; startY: 64
                PathLine { x: 118; y: 10 }
                PathLine { x: 118; y: 118 }
                PathLine { x: 64; y: 64 }
            }
            ShapePath {
                fillColor: "#1B6A61"
                strokeColor: "transparent"
                startX: 64; startY: 64
                PathLine { x: 118; y: 118 }
                PathLine { x: 10; y: 118 }
                PathLine { x: 64; y: 64 }
            }
            ShapePath {
                fillColor: "#46B8A9"
                strokeColor: "transparent"
                startX: 64; startY: 64
                PathLine { x: 10; y: 118 }
                PathLine { x: 10; y: 10 }
                PathLine { x: 64; y: 64 }
            }
        }
    }
}
