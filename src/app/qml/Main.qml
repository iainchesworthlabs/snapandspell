import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import WordJewelsSolver

ApplicationWindow {
    id: window

    // The board is size x size. A tile holds one to three letters; "Q" on its
    // own stands for the Qu face.
    property int boardSize: 4
    property var tiles: []

    width: 480
    height: 800
    visible: true
    title: qsTr("Word Jewels Solver")

    function resetTiles() {
        const fresh = [];
        for (let i = 0; i < boardSize * boardSize; ++i) {
            fresh.push("");
        }
        tiles = fresh;
    }

    // The board in the library's syntax, or "" while a tile is still empty.
    function boardText() {
        const rows = [];
        for (let row = 0; row < boardSize; ++row) {
            const cells = [];
            for (let column = 0; column < boardSize; ++column) {
                const tile = tiles[row * boardSize + column].trim().toUpperCase();
                if (tile.length === 0) {
                    return "";
                }
                cells.push(tile === "Q" ? "QU" : tile);
            }
            rows.push(cells.join(" "));
        }
        return rows.join("/");
    }

    function solveBoard() {
        const text = boardText();
        if (text.length === 0) {
            solver.solve("", 3);
            return;
        }
        solver.solve(text, 3);
    }

    Component.onCompleted: resetTiles()

    SolverModel {
        id: solver
        Component.onCompleted: loadDefaultDictionary()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("Board size")
            }
            SpinBox {
                from: 2
                to: 8
                value: window.boardSize
                onValueModified: {
                    window.boardSize = value;
                    window.resetTiles();
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Clear")
                onClicked: window.resetTiles()
            }
        }

        GridLayout {
            id: grid
            Layout.alignment: Qt.AlignHCenter
            columns: window.boardSize
            rowSpacing: 6
            columnSpacing: 6

            Repeater {
                model: window.boardSize * window.boardSize

                TextField {
                    required property int index

                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    horizontalAlignment: Text.AlignHCenter
                    maximumLength: 3
                    font.pixelSize: 24
                    font.capitalization: Font.AllUppercase
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                    text: window.tiles[index] === undefined ? "" : window.tiles[index]
                    onTextEdited: {
                        const updated = window.tiles.slice();
                        updated[index] = text;
                        window.tiles = updated;
                    }
                }
            }
        }

        Button {
            Layout.fillWidth: true
            text: qsTr("Solve")
            enabled: solver.ready
            onClicked: window.solveBoard()
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: solver.status
        }

        Label {
            visible: solver.results.length > 0
            text: qsTr("%1 points").arg(solver.totalScore)
            font.bold: true
        }

        ListView {
            id: results
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: solver.results

            delegate: ItemDelegate {
                required property var modelData

                width: results.width
                text: modelData.word
                contentItem: RowLayout {
                    Label {
                        Layout.fillWidth: true
                        text: modelData.word
                        font.pixelSize: 18
                    }
                    Label {
                        text: modelData.score
                        font.bold: true
                    }
                }
            }
        }
    }
}
