pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// scan.check (docs/design/game.md, "The check screen"): the replica of the real
// board, upright, with the reader's doubtful tiles ringed, a letter picker, the
// size the adult can change, "Read again" and "Yes, play". Nothing is solved
// until "Yes, play": ScanModel::confirm() is the only way from the draft to a
// Board, and that press is the grown-up saying they looked (docs/design/
// scanner.md, "Confirm and correct").
//
// The letters are the replica's, upper case, as the dice are; the words and every
// sentence are ScanWording's. Nothing here decides what a board is.
FocusScope {
    id: root

    required property ScanModel scan
    required property AssistModel assist

    signal backRequested
    signal playRequested

    // The tile the picker opened for, or -1.
    property int picked: -1
    // "incomplete" or "noGame" after a press that could not play, or "".
    property string problem: ""

    readonly property int boardRows: root.scan.rows
    readonly property int boardColumns: root.scan.columns
    readonly property bool ready: root.boardRows > 0 && root.boardColumns > 0
    // For the RTL check in the stub shell: the confirm button is on the trailing
    // side, and the window's mirroring reaches this screen.
    readonly property real yesPlayX: yesPlay.x
    readonly property bool mirrored: LayoutMirroring.enabled
    readonly property real tile: BoardGeometry.tile(gridArea.width, gridArea.height,
                                                    Math.max(root.boardRows, root.boardColumns))
    readonly property real gap: BoardGeometry.gap(gridArea.width, gridArea.height)
    readonly property var alphabet: "ABCDEFGHIJKLMNOPQRSTUVWXYZ".split("")

    activeFocusOnTab: true

    // A correction is the grown-up's: it clears whatever the last press said.
    Connections {
        target: root.scan

        function onDraftChanged() {
            root.problem = "";
        }
    }

    // "Yes, play": the grown-up's confirmation, and the board the assistant then
    // plays. The board is handed over as plain values, as QML carries them; the
    // AssistGame makes its own Board from the same confirmed grid.
    function play() {
        root.problem = "";
        if (!root.scan.confirm()) {
            root.problem = "incomplete";
            return;
        }
        if (!root.assist.begin(root.scan.rows, root.scan.columns, root.scan.letters)) {
            root.problem = "noGame";
            return;
        }
        root.playRequested();
    }

    function choose(letters) {
        if (root.picked < 0) {
            return;
        }
        // A letter, a pair or a word of up to three: the model says whether it
        // can be a tile; a picker that was refused stays open.
        if (root.scan.setTile(root.picked, letters)) {
            root.picked = -1;
        }
    }

    // --- the board -------------------------------------------------------------
    Item {
        id: gridArea
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: sizeRow.top
        anchors.margins: Theme.space4
        anchors.bottomMargin: Theme.space2
        // The replica of the real board does not mirror: its tiles keep the
        // order the family's board has, in every language.
        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        Grid {
            anchors.centerIn: parent
            columns: Math.max(root.boardColumns, 1)
            spacing: root.gap

            Repeater {
                model: root.scan.tiles

                Item {
                    id: cell
                    required property int index
                    required property var modelData

                    width: Math.max(root.tile, 1)
                    height: width

                    readonly property int row: Math.floor(cell.index / Math.max(root.boardColumns, 1))
                    readonly property int column: cell.index % Math.max(root.boardColumns, 1)

                    TileFace {
                        anchors.fill: parent
                        letters: cell.modelData.letters
                        lit: false
                    }

                    // The reader doubted this tile and no one has settled it:
                    // a ring and a "?", the same doubt the reading carried.
                    Rectangle {
                        visible: cell.modelData.doubtful
                        anchors.fill: parent
                        anchors.margins: -Theme.space1
                        color: "transparent"
                        border.width: Theme.heavy
                        border.color: Theme.ink
                    }
                    Text {
                        visible: cell.modelData.doubtful
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: Theme.space1
                        text: ScanWording.doubtfulMark
                        font.family: Theme.fontFamily
                        font.bold: true
                        font.pixelSize: Math.max(Theme.smallSize, cell.width * 0.22)
                        color: Theme.ink
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: ScanWording.tileName(cell.modelData.letters, cell.row + 1,
                                                          cell.column + 1)
                    Accessible.onPressAction: root.picked = cell.index

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.picked = cell.index
                    }
                }
            }
        }

        // The size is shown first (docs/design/game.md, "The check screen").
        Text {
            visible: !root.ready
            anchors.centerIn: parent
            text: ScanWording.readAgainMessage
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            color: Theme.inkSecondary
        }
    }

    // --- the turn, and the board size -----------------------------------------
    AppButton {
        id: turn
        anchors.right: parent.right
        anchors.rightMargin: Theme.space4
        anchors.bottom: sizeRow.top
        anchors.bottomMargin: Theme.space2
        size: "child"
        iconName: "again"
        accessibleName: ScanWording.turn
        onClicked: root.scan.turn()
    }

    Row {
        id: sizeRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: problemText.top
        anchors.bottomMargin: Theme.space2
        spacing: Theme.targetGap

        Repeater {
            model: root.scan.sizes

            AppButton {
                id: sizeChoice
                required property int modelData
                width: Theme.targetChild
                height: Theme.targetChild
                size: "child"
                checkable: true
                checked: root.scan.size === sizeChoice.modelData
                accessibleName: ScanWording.sizeChoice(sizeChoice.modelData)
                onClicked: root.scan.resize(sizeChoice.modelData, sizeChoice.modelData)

                // A picture of the board of that size: the only thing on the
                // button a child reads.
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.64
                    height: width
                    color: Theme.tray

                    Grid {
                        id: sizeGrid
                        anchors.fill: parent
                        anchors.margins: parent.width * 0.08
                        columns: sizeChoice.modelData
                        spacing: parent.width * 0.05

                        Repeater {
                            model: sizeChoice.modelData * sizeChoice.modelData

                            Rectangle {
                                width: (sizeGrid.width - (sizeChoice.modelData - 1) * sizeGrid.spacing)
                                       / sizeChoice.modelData
                                height: width
                                color: Theme.die
                            }
                        }
                    }
                }
            }
        }
    }

    // --- what the grown-up is told -------------------------------------------
    Text {
        id: problemText
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: bottomBar.top
        anchors.bottomMargin: Theme.space2
        width: Math.min(parent.width - 2 * Theme.space4, 720)
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        visible: text !== ""
        text: root.problem === "incomplete" ? ScanWording.incomplete
            : root.problem === "noGame" ? ScanWording.noGame
            : root.scan.doubtful.length > 0 ? ScanWording.doubtfulHint
            : ""
        font.family: Theme.fontFamily
        font.pixelSize: Theme.bodySize
        color: Theme.inkSecondary
    }

    // --- Read again, Yes, play -------------------------------------------------
    Item {
        id: bottomBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.space4
        height: Theme.targetPrimary

        AppButton {
            id: readAgain
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(parent.width / 4, Theme.targetChild)
            height: Theme.targetChild
            size: "child"
            variant: "secondary"
            accessibleName: ScanWording.readAgain
            onClicked: root.backRequested()

            Text {
                anchors.centerIn: parent
                text: ScanWording.readAgain
                font.family: Theme.fontFamily
                font.pixelSize: Theme.labelSize
                color: Theme.ink
                elide: Text.ElideRight
            }
        }

        AppButton {
            id: yesPlay
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(parent.width / 3, Theme.targetPrimary)
            height: Theme.targetPrimary
            size: "primary"
            variant: "primary"
            accessibleName: ScanWording.yesPlay
            onClicked: root.play()

            Text {
                anchors.centerIn: parent
                text: ScanWording.yesPlay
                font.family: Theme.fontFamily
                font.pixelSize: Theme.labelSize
                color: Theme.buttonInk
            }
        }
    }

    // --- the letter picker ------------------------------------------------------
    // A tap on a tile opens this; a suggestion the reader had comes first, then A
    // to Z, Qu, and a box for a tile of two or three letters.
    Rectangle {
        id: picker
        anchors.fill: parent
        visible: root.picked >= 0
        color: Theme.scrim
        z: 10

        MouseArea {
            anchors.fill: parent
            onClicked: root.picked = -1
        }

        Rectangle {
            id: card
            anchors.centerIn: parent
            width: Math.min(parent.width - 2 * Theme.space4, 640)
            height: Math.min(parent.height - 2 * Theme.space4, 620)
            color: Theme.card
            border.width: Theme.rule
            border.color: Theme.outline

            // The card takes the clicks that are not a letter or a close.
            MouseArea {
                anchors.fill: parent
            }

            readonly property real margin: Theme.space4
            readonly property int columns: Math.max(4, Math.floor(
                (card.width - 2 * card.margin) / (Theme.targetMin + Theme.targetGap)))
            readonly property real letter: Math.max(Theme.targetMin, Math.min(
                Theme.targetChild,
                (card.width - 2 * card.margin - (card.columns - 1) * Theme.targetGap)
                / card.columns))

            Flickable {
                anchors.fill: parent
                anchors.margins: card.margin
                contentHeight: pickerContent.implicitHeight
                clip: true

                Column {
                    id: pickerContent
                    width: parent.width
                    spacing: Theme.space2

                    Text {
                        width: parent.width
                        text: ScanWording.pickerTitle
                        font.family: Theme.fontFamily
                        font.bold: true
                        font.pixelSize: Theme.wordSize
                        color: Theme.ink
                        elide: Text.ElideRight
                    }

                    // The reader's other thoughts, best first.
                    Row {
                        spacing: Theme.targetGap

                        Repeater {
                            model: root.scan.alternatives(root.picked)

                            AppButton {
                                id: suggestion
                                required property string modelData
                                width: card.letter
                                height: card.letter
                                size: "min"
                                variant: "secondary"
                                accessibleName: suggestion.modelData
                                onClicked: root.choose(suggestion.modelData)

                                Text {
                                    anchors.centerIn: parent
                                    text: suggestion.modelData
                                    font.family: Theme.fontFamily
                                    font.bold: true
                                    font.pixelSize: parent.height * 0.42
                                    color: Theme.ink
                                }
                            }
                        }
                    }

                    Grid {
                        id: letters
                        width: parent.width
                        columns: card.columns
                        spacing: Theme.targetGap

                        Repeater {
                            model: root.alphabet

                            AppButton {
                                id: letterButton
                                required property string modelData
                                width: card.letter
                                height: card.letter
                                size: "min"
                                variant: "secondary"
                                accessibleName: letterButton.modelData
                                onClicked: root.choose(letterButton.modelData)

                                Text {
                                    anchors.centerIn: parent
                                    text: letterButton.modelData
                                    font.family: Theme.fontFamily
                                    font.bold: true
                                    font.pixelSize: parent.height * 0.42
                                    color: Theme.ink
                                }
                            }
                        }
                    }

                    Row {
                        spacing: Theme.space2

                        AppButton {
                            width: card.letter
                            height: Theme.targetChild
                            size: "child"
                            variant: "secondary"
                            accessibleName: ScanWording.pickerQu
                            onClicked: root.choose("QU")

                            Text {
                                anchors.centerIn: parent
                                text: ScanWording.pickerQu
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.labelSize
                                color: Theme.ink
                            }
                        }

                        TextInput {
                            id: otherLetters
                            width: Math.max(Theme.targetChild * 2, card.width * 0.3)
                            height: Theme.targetChild
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignHCenter
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.labelSize
                            font.capitalization: Font.AllUppercase
                            color: Theme.ink
                            maximumLength: 3
                            selectByMouse: true
                            Accessible.role: Accessible.EditableText
                            Accessible.name: ScanWording.pickerOther
                            onAccepted: root.choose(otherLetters.text)
                        }

                        AppButton {
                            width: Math.max(Theme.targetChild, card.width * 0.18)
                            height: Theme.targetChild
                            size: "child"
                            variant: "secondary"
                            accessibleName: ScanWording.useLetters
                            onClicked: root.choose(otherLetters.text)

                            Text {
                                anchors.centerIn: parent
                                text: ScanWording.useLetters
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.smallSize
                                color: Theme.ink
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }

            AppButton {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.space2
                size: "min"
                iconName: "close"
                accessibleName: ScanWording.closePicker
                onClicked: root.picked = -1
            }
        }
    }
}
