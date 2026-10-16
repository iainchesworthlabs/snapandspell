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
    // or "" for a jewel. Set with fill(), which makes the cells of the atlas
    // from them in one pass.
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

    // Sets the size and the faces, and makes the cells of the atlas.
    function fill(rowCount, columnCount, tileFaces) {
        const letters = [];
        const jewels = [];
        for (let die = 0; die < tileFaces.length; ++die) {
            const sides = tileFaces[die];
            for (let side = 0; side < facesPerDie; ++side) {
                const text = sides && sides.length > 0 ? String(sides[side % sides.length]).toUpperCase() : "";
                const x = ((die % columnCount) * blockColumns + side % blockColumns) * cell;
                const y = (Math.floor(die / columnCount) * blockRows + Math.floor(side / blockColumns)) * cell;
                if (text === "") {
                    jewels.push({ x: x, y: y });
                } else {
                    // About 55% of the tile for one letter; a two-letter tile
                    // (the Qu face) at 80% of that, a three-letter tile smaller
                    // still.
                    letters.push({ x: x, y: y, letters: text,
                                   pixelSize: text.length > 2 ? 48 : (text.length === 2 ? 68 : 88) });
                }
            }
        }
        rows = rowCount;
        columns = columnCount;
        faces = tileFaces;
        letterCells = letters;
        jewelCells = jewels;
        letterPool = Math.max(letterPool, letters.length);
        jewelPool = Math.max(jewelPool, jewels.length);
    }

    // The cream face, dark edge and dark letter of every cell that has letters.
    Repeater {
        model: atlas.letterPool

        Rectangle {
            required property int index
            readonly property var place: atlas.letterCells[index]

            visible: place !== undefined
            x: place ? place.x : 0
            y: place ? place.y : 0
            width: atlas.cell
            height: atlas.cell
            color: atlas.faceColor
            border.color: atlas.edgeColor
            border.width: 10

            Text {
                anchors.centerIn: parent
                textFormat: Text.PlainText
                text: parent.place ? parent.place.letters : ""
                color: atlas.inkColor
                font.bold: true
                font.pixelSize: parent.place ? parent.place.pixelSize : 88
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
            color: "#2E9F92"
            border.color: atlas.edgeColor
            border.width: 10
        }

        // The four facets, from the middle of the cell to each edge.
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
