// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

// scan.check (docs/design/game.md, "The check screen"): the replica of the real
// board, upright, with the reader's doubtful tiles ringed, a letter picker, the
// size the adult can change, the secondary action and "Start playing". Nothing is
// solved until "Start playing": ScanModel::confirm() is the only way from the
// draft to a Board, and that press is the grown-up saying they looked
// (docs/design/scanner.md, "Confirm and correct").
//
// The screen is laid out by the rule Solo Play and the assistant's board use
// (SoloLayout): the replica in the leading two thirds and a panel in the
// trailing third of a wide window, the replica above and the panel below in a
// tall one. So the board does not jump when "Start playing" turns this screen
// into the assistant's: it is the same tray, at the same size, in the same place.
// The tray, its gold rim and the cream dice are the kit's (BoardTray, TileFace).
//
// The panel holds the board size (SegmentedChoice, a check mark and not a gold
// border), Turn for a camera board, one sentence of guidance and the actions
// (ActionBar, one height ladder, 16 dp from the edges, aligned with the board's
// area). The sentence is whichever is true: a blank typed grid says what to do,
// an unfinished board says what it needs, a board with ringed tiles says what a
// ring means. "Start playing" is disabled until every tile has a letter, so it
// can never be pressed to be refused; with ringed tiles left it asks first
// (RingedQuestion, a Sheet of the scan screen).
//
// The secondary action says what it does: a typed board is not a reading, so it
// starts over as a fresh empty grid; a camera-read board goes back to the frame
// and scans again. Turn is a camera board's control: a typed board is already in
// the order its typist chose.
//
// The letters are the replica's, upper case, as the dice are; the words and every
// sentence are ScanWording's. Nothing here decides what a board is.
FocusScope {
    id: root

    required property ScanModel scan
    required property AssistModel assist

    signal playRequested

    // The tile the picker opened for, or -1. The picker is the scan screen's
    // Sheet; it reads and clears this.
    property int picked: -1
    // "noGame" after a press that the assistant could not start, or "".
    property string problem: ""
    // Whether the question about ringed tiles is open (the scan screen's Sheet).
    property bool askingRinged: false

    readonly property int boardRows: root.scan.rows
    readonly property int boardColumns: root.scan.columns
    readonly property bool ready: root.boardRows > 0 && root.boardColumns > 0
    readonly property bool blank: root.scan.letters.every(letters => letters === "")
    readonly property bool wide: rule.wide
    // For the RTL check in the stub shell: the confirm button is on the trailing
    // side, and the window's mirroring reaches this screen.
    readonly property real yesPlayX: panel.x + actions.x + actions.width / 2
    readonly property bool mirrored: LayoutMirroring.enabled
    readonly property real tile: rule.tile
    readonly property real gap: rule.gap
    readonly property real pitch: root.tile + root.gap

    // The sizes offered (3 to 6, the sizes the game plays), and the size the
    // reader counted if it is none of them (it counts 3 to 8): shown and chosen,
    // so the board never has a size the grown-up cannot see.
    readonly property var sizeOptions: {
        const sizes = root.scan.sizes.slice();
        if (root.scan.size > 0 && sizes.indexOf(root.scan.size) < 0) {
            sizes.push(root.scan.size);
        }
        sizes.sort((a, b) => a - b);
        return sizes.map(side => ({value: side, label: ScanWording.sizeLabel(side)}));
    }

    // The sentence under the board size: the one thing the grown-up most needs
    // to know now.
    readonly property string guidance: {
        if (root.problem === "noGame") {
            return ScanWording.noGame;
        }
        if (!root.scan.draftFromCamera && root.blank) {
            return ScanWording.typedHint;
        }
        // A tile the reader could not read is ringed too, so the ring's sentence
        // covers it: it is the one to say while any ring is left.
        if (root.scan.doubtful.length > 0) {
            return ScanWording.doubtfulHint;
        }
        if (!root.scan.complete) {
            return ScanWording.incomplete;
        }
        return root.scan.draftFromCamera ? "" : ScanWording.typedReady;
    }

    activeFocusOnTab: true

    // A correction is the grown-up's: it clears whatever the last press said.
    Connections {
        target: root.scan

        function onDraftChanged() {
            root.problem = "";
        }
    }

    SoloLayout {
        id: rule
        width: root.width
        height: root.height
        boardSize: Math.max(root.boardRows, root.boardColumns, 3)
        // The panel here is a size choice, a sentence and two buttons, and scrolls: the board, which is
        // tapped to correct, takes what a tall window can give it (a tile is at least 48 dp).
        minPanel: 200
        // The replica stays the family's board, left to right as it was read; the
        // split around it follows the window's direction.
        rightToLeft: root.mirrored
    }

    // "Start playing": the grown-up's confirmation, and the board the assistant
    // then plays. The board is handed over as plain values, as QML carries them;
    // the AssistGame makes its own Board from the same confirmed grid.
    function play() {
        root.problem = "";
        if (!root.scan.confirm()) {
            return;
        }
        if (!root.assist.begin(root.scan.rows, root.scan.columns, root.scan.letters)) {
            root.problem = "noGame";
            return;
        }
        root.playRequested();
    }

    // The press on "Start playing": with ringed tiles left, ask first.
    function requestPlay() {
        if (!root.scan.complete) {
            return;
        }
        if (root.scan.doubtful.length > 0) {
            root.askingRinged = true;
            return;
        }
        root.play();
    }

    // The secondary action does what its label says. A typed board was not read
    // by a camera, so "Start over" is a fresh empty grid to type again; a camera
    // board's "Scan again" is the reading's way back to the frame.
    function secondaryAction() {
        if (root.scan.draftFromCamera) {
            root.scan.readAgain();
        } else {
            root.scan.typeBoard();
        }
    }

    // --- the board -------------------------------------------------------------
    Rectangle {
        x: rule.boardArea.x
        y: rule.boardArea.y
        width: rule.boardArea.width
        height: rule.boardArea.height
        color: Theme.boardArea
    }

    Item {
        id: board
        readonly property real side: root.boardColumns * root.tile + (root.boardColumns - 1) * root.gap
        readonly property real tall: root.boardRows * root.tile + (root.boardRows - 1) * root.gap
        readonly property real outset: Theme.trayOutset(root.tile, rule.inset)

        x: rule.boardRect.x + (rule.boardRect.width - board.side) / 2
        y: rule.boardRect.y + (rule.boardRect.height - board.tall) / 2
        width: Math.max(board.side, 0)
        height: Math.max(board.tall, 0)
        visible: root.ready
        // The replica of the real board does not mirror: its tiles keep the
        // order the family's board has, in every language.
        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true

        BoardTray {
            x: -board.outset
            y: -board.outset
            width: board.width + 2 * board.outset
            height: board.height + 2 * board.outset
        }

        Repeater {
            model: root.scan.tiles

            Item {
                id: cell
                required property int index
                required property var modelData

                readonly property int row: Math.floor(cell.index / Math.max(root.boardColumns, 1))
                readonly property int column: cell.index % Math.max(root.boardColumns, 1)

                x: cell.column * root.pitch
                y: cell.row * root.pitch
                width: Math.max(root.tile, 1)
                height: width
                activeFocusOnTab: true

                // The reader doubted this tile and no one has settled it: a question mark in a
                // sunshine disc at the corner of the die (TileFace's `doubtful`).
                TileFace {
                    anchors.fill: parent
                    letters: cell.modelData.letters
                    lit: false
                    doubtful: cell.modelData.doubtful
                }

                // Tab reaches a tile (a press does not take focus), and the ring says which.
                FocusRing {
                    visible: cell.activeFocus
                    inner: Theme.radiusSmall
                }

                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                            || event.key === Qt.Key_Space) {
                        root.picked = cell.index;
                        event.accepted = true;
                    }
                }

                Accessible.role: Accessible.Button
                Accessible.name: cell.modelData.doubtful
                                 ? ScanWording.doubtfulTileName(cell.modelData.letters, cell.row + 1, cell.column + 1)
                                 : ScanWording.tileName(cell.modelData.letters, cell.row + 1, cell.column + 1)
                Accessible.focusable: true
                Accessible.onPressAction: root.picked = cell.index

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.picked = cell.index
                }
            }
        }
    }

    Label {
        visible: !root.ready
        x: rule.boardArea.x + Theme.space4
        y: rule.boardArea.y
        width: rule.boardArea.width - 2 * Theme.space4
        height: rule.boardArea.height
        role: "body"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        color: Theme.inkSecondary
        text: ScanWording.readAgainMessage
    }

    // --- the panel: size, turn, what to know, the actions ----------------------
    Item {
        id: panel
        x: rule.panelArea.x
        y: rule.panelArea.y
        width: rule.panelArea.width
        height: rule.panelArea.height

        Rectangle {
            anchors.fill: parent
            color: Theme.panel
        }
        // The panel's edge toward the board: its leading side beside the board,
        // its top under it.
        Rectangle {
            visible: root.wide
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Theme.hair
            color: Theme.line
        }
        Rectangle {
            visible: !root.wide
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Theme.hair
            color: Theme.line
        }

        // The panel scrolls as a whole when a short window (a phone on its side, big text) leaves it less
        // than its parts need; the actions are then at the foot of what scrolls.
        Flickable {
            id: panelScroll
            anchors.fill: parent
            contentWidth: width
            contentHeight: Math.max(height, panelColumn.implicitHeight + 2 * Theme.space4)
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentHeight > height + 0.5

            // As wide as a phone's panel at most: on a 2400 px window the buttons
            // would otherwise stretch across a third of the screen.
            ColumnLayout {
                id: panelColumn
                y: Theme.space4
                x: (panelScroll.width - width) / 2
                height: panelScroll.contentHeight - 2 * Theme.space4
                width: Math.min(panelScroll.width - 2 * Theme.space4, Theme.modalMax)
                spacing: Theme.space3

                // What can scroll is the middle: the actions below it never leave the
                // window, however short it is or however large the text.
                Flickable {
                    id: middle
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: middleColumn.implicitHeight
                    Layout.minimumHeight: Math.min(middleColumn.implicitHeight, Theme.targetMin)
                    contentWidth: width
                    contentHeight: middleColumn.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: contentHeight > height

                    ColumnLayout {
                        id: middleColumn
                        width: middle.width
                        spacing: Theme.space3

                        // Not a Card: its title and padding cost 70 dp that the panel
                        // does not have at 150% text on a 900 by 620 window.
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space2

                            Eyebrow {
                                text: ScanWording.boardSize
                            }

                            SegmentedChoice {
                                Layout.fillWidth: true
                                title: ScanWording.boardSize
                                size: "min"
                                compact: true
                                options: root.sizeOptions
                                current: root.scan.size
                                onChosen: value => root.scan.resize(value, value)
                            }

                            // Turn is the camera board's control: the real board is
                            // turned every which way, and a quarter turn of the
                            // replica matches how the child sees it.
                            AppButton {
                                id: turn
                                visible: root.scan.draftFromCamera
                                Layout.alignment: Qt.AlignLeft
                                size: "min"
                                variant: "ghost"
                                quiet: true
                                iconName: "again"
                                label: qsTr("Turn")
                                accessibleName: ScanWording.turn
                                onClicked: root.scan.turn()
                            }
                        }
                    }
                }

                // What the grown-up most needs to know now. It stays put, above the
                // actions, where the board size above it may scroll.
                Label {
                    Layout.fillWidth: true
                    visible: text !== ""
                    role: "body"
                    wrapMode: Text.WordWrap
                    color: Theme.ink
                    text: root.guidance
                }

                // One row (or, when it does not fit, one stack): the secondary action
                // beside the primary, in the same order in every language. Start
                // playing waits for every tile to have a letter.
                ActionBar {
                    id: actions
                    Layout.fillWidth: true
                    tier: "child"
                    primaryLabel: ScanWording.startPlaying
                    primaryEnabled: root.scan.complete
                    secondaryLabel: root.scan.draftFromCamera ? ScanWording.scanAgain : ScanWording.startOver
                    onPrimaryClicked: root.requestPlay()
                    onSecondaryClicked: root.secondaryAction()
                }
            }
        }
    }
}
