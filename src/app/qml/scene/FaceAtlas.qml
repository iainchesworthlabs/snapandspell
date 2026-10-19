pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import "ShakeScript.js" as Script

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
    // or "" for a jewel. Set with fill(), which makes the cells of the atlas
    // from them in one pass.
    property var faces: []
    property int rows: 1
    property int columns: 1

    readonly property int cell: 128
    readonly property int blockColumns: 3
    readonly property int blockRows: 2
    readonly property int facesPerDie: blockColumns * blockRows
    // The same cream, dark-teal edge and ink as TileFace.qml. The face is inset
    // and rounded here so a lit cube reads as the same die the child picks up on
    // the flat board, not as a square photo pasted on a cube.
    readonly property color faceColor: "#FFF7E3"
    readonly property color edgeColor: "#20323A"
    readonly property color inkColor: "#1F2A30"
    readonly property color jewelColor: "#8FA1B8"

    width: columns * blockColumns * cell
    height: rows * blockRows * cell

    // Where each cell is and what is on it, made by fill(): the cells with
    // letters ({ x, y, letters, pixelSize }) and the jewels ({ x, y }). The
    // items below show them by their place in these lists.
    property var letterCells: []
    property var jewelCells: []
    // How many of each item there are. They are made as a board needs them and
    // then kept: a smaller board hides the ones it does not use, and a board
    // of a size that has been shown makes none. Making them was most of what a
    // board that changed the atlas's size cost the GUI thread.
    property int letterPool: 0
    property int jewelPool: 0

    // Sets the size and the faces, and makes the cells of the atlas. The cells
    // themselves (their place and their letters) are the scene script's
    // (Script.atlasCells), so what the work budget counts is what is drawn.
    function fill(rowCount, columnCount, tileFaces) {
        const cells = Script.atlasCells(columnCount, tileFaces, cell, blockColumns, blockRows);
        rows = rowCount;
        columns = columnCount;
        faces = tileFaces;
        letterCells = cells.letters;
        jewelCells = cells.jewels;
        letterPool = Math.max(letterPool, letterCells.length);
        jewelPool = Math.max(jewelPool, jewelCells.length);
    }

    // The cream face, dark edge and dark letter of every cell that has letters.
    Repeater {
        model: atlas.letterPool

        Rectangle {
            id: cellFace
            required property int index
            readonly property var place: atlas.letterCells[index]

            visible: place !== undefined
            x: place ? place.x : 0
            y: place ? place.y : 0
            width: atlas.cell
            height: atlas.cell
            radius: 12
            color: atlas.edgeColor

            Rectangle {
                anchors.fill: parent
                anchors.margins: 4
                radius: 9
                color: atlas.faceColor

                // The top catches the light and the foot sits in its shadow, as
                // on the flat tile.
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 7
                    height: 3
                    radius: 2
                    color: "#FFFFFF"
                    opacity: 0.55
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 7
                    height: 4
                    radius: 2
                    color: "#000000"
                    opacity: 0.12
                }

                Text {
                    anchors.centerIn: parent
                    textFormat: Text.PlainText
                    text: cellFace.place ? cellFace.place.letters : ""
                    color: atlas.inkColor
                    font.family: "Andika"
                    font.weight: Font.Bold
                    font.pixelSize: cellFace.place ? cellFace.place.pixelSize : 88
                }
            }
        }
    }

    // The jewel, shown from the one texture it is drawn into below.
    Repeater {
        model: atlas.jewelPool

        ShaderEffect {
            required property int index
            readonly property var place: atlas.jewelCells[index]
            // The effect's own shaders show its `source`.
            property variant source: jewelTexture

            visible: place !== undefined
            x: place ? place.x : 0
            y: place ? place.y : 0
            width: atlas.cell
            height: atlas.cell
            blending: false
        }
    }

    // The jewel is drawn once, into a texture of one cell, and every jewel cell
    // shows that texture: a Shape of four paths for each of six cells of each
    // dead die cost about 0.6 ms a cell (docs/design/shake-prototype.md). The
    // texture is not live: it is made on the first frame and kept. It sits one
    // cell outside the atlas, so that the atlas does not show it.
    Item {
        id: jewel

        width: atlas.cell
        height: atlas.cell

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: atlas.edgeColor
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 4
            radius: 9
            color: atlas.faceColor
        }

        // The dead jewel is the same cut stone the tray shows: slate facets, a
        // gold outline and a small table, on a cream die.
        Item {
            id: gem
            anchors.centerIn: parent
            width: 74
            height: 74

            Shape {
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    strokeColor: "#C98A1E"
                    strokeWidth: 4
                    fillColor: atlas.jewelColor
                    joinStyle: ShapePath.RoundJoin
                    startX: gem.width * 0.5
                    startY: gem.height * 0.04
                    PathLine { x: gem.width * 0.94; y: gem.height * 0.36 }
                    PathLine { x: gem.width * 0.74; y: gem.height * 0.94 }
                    PathLine { x: gem.width * 0.26; y: gem.height * 0.94 }
                    PathLine { x: gem.width * 0.06; y: gem.height * 0.36 }
                    PathLine { x: gem.width * 0.5; y: gem.height * 0.04 }
                }
                ShapePath {
                    strokeColor: "#20323A"
                    strokeWidth: 1
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    startX: gem.width * 0.06
                    startY: gem.height * 0.36
                    PathLine { x: gem.width * 0.94; y: gem.height * 0.36 }
                    PathMove { x: gem.width * 0.34; y: gem.height * 0.36 }
                    PathLine { x: gem.width * 0.5; y: gem.height * 0.94 }
                    PathLine { x: gem.width * 0.66; y: gem.height * 0.36 }
                }
            }
        }
    }

    ShaderEffectSource {
        id: jewelTexture

        x: -atlas.cell
        width: atlas.cell
        height: atlas.cell
        sourceItem: jewel
        hideSource: true
        live: false
    }
}
