pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// The panel of solo.playing (docs/design/game.md, "The panel"; the squares of
// docs/design/layouts.svg): the words I found, the hint, and the history. It is
// one third of the window: the trailing third of a wide one, the bottom third of a
// tall one. There is no New board button and no count of words left.
//
//   wide      the tray, then the hint, then the history, stacked
//   tall      the tray across the top; the hint and the history side by side below
//   roomy     300 dp or more on both sides: a tray of three rows or more, the hint
//             with its display, three thumbnails of earlier boards inline
//   smaller   the tray in the space left, one 64 dp hint button, and one button
//             that opens the history as a sheet
//
// While a tapped path is waiting, the hint's place holds the Done jewel instead:
// the tap-in-order way of handing a word in.
Item {
    id: root

    required property var solo
    required property bool wide
    signal historyRequested

    readonly property bool roomy: width >= Theme.roomyPanel && height >= Theme.roomyPanel
    readonly property real margin: Theme.space2
    readonly property bool pending: solo.path.length > 0 && !solo.pointerDown
    readonly property bool hintShown: solo.helpAllowed || pending
    // Hebrew and Yiddish: the hint and the history swap sides in a tall panel.
    readonly property bool mirrored: Theme.rightToLeft

    // The hint and the history each have a card, at least as high as the 64 dp
    // button in it and its margins; the tray takes what is left.
    readonly property real cardMin: Theme.targetChild + 2 * Theme.space2
    readonly property real cardHeight: Math.max(cardMin, (height - 4 * margin) * (wide ? 0.215 : 0.38))
    readonly property int cards: wide ? (hintShown ? 2 : 1) : 1
    readonly property real trayHeight: height - (cards + 2) * margin - cards * cardHeight

    Rectangle {
        anchors.fill: parent
        color: Theme.panel
        border.width: Theme.hair
        border.color: Theme.outline
    }

    // --- the tray: words I found ----------------------------------------------------
    Rectangle {
        id: tray
        x: root.margin
        y: root.margin
        width: root.width - 2 * root.margin
        height: root.trayHeight
        color: Theme.card
        border.width: Theme.rule
        border.color: Theme.outline

        Accessible.role: Accessible.List
        Accessible.name: SoloWording.wordsFound

        ListView {
            id: words
            anchors.fill: parent
            anchors.margins: Theme.space2
            clip: true
            model: root.solo.found
            spacing: Theme.space1
            // Newest at the foot: the list follows the last jewel in.
            onCountChanged: positionViewAtEnd()

            delegate: Item {
                id: row
                required property int index
                required property var modelData
                width: words.width
                height: Theme.targetChild

                Accessible.role: Accessible.ListItem
                Accessible.name: SoloWording.found(modelData.word)
                Accessible.onPressAction: Feedback.cue("speak", modelData.word)

                Jewel {
                    id: jewel
                    anchors.verticalCenter: parent.verticalCenter
                    tier: row.modelData.tier
                    colorIndex: row.index
                    x: Theme.space1
                }
                Text {
                    anchors.left: jewel.right
                    anchors.leftMargin: Theme.space3
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.word
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.wordSize
                    color: Theme.ink
                }
                // Tap a word to hear it.
                MouseArea {
                    anchors.fill: parent
                    onClicked: Feedback.cue("speak", row.modelData.word)
                }
            }

            // A jewel drops into the tray; with reduced motion it appears.
            add: Transition {
                enabled: !Theme.reducedMotion
                NumberAnimation {
                    property: "y"
                    from: -Theme.space16
                    duration: Theme.turn
                    easing.type: Easing.OutBounce
                }
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: Theme.base
                }
            }
        }
    }

    // --- the hint: the start tile ----------------------------------------------------------
    Rectangle {
        id: hint
        visible: root.hintShown
        x: root.wide || !root.mirrored ? root.margin : root.width - width - root.margin
        y: tray.y + tray.height + root.margin
        width: root.wide ? root.width - 2 * root.margin : (root.width - 3 * root.margin) / 2
        height: root.cardHeight
        color: Theme.hintCard
        border.width: Theme.rule
        border.color: Theme.outline

        readonly property real button: Math.min(Theme.targetChild, height - 2 * Theme.space1)

        // The hint: one press lights the start tile of one word not yet found.
        AppButton {
            visible: !root.pending
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Theme.space2
            variant: "secondary"
            ink: Theme.hintCardInk
            edge: Theme.hintCardInk
            hoverFill: Qt.rgba(0.12, 0.16, 0.22, 0.12)
            pressFill: Qt.rgba(0.12, 0.16, 0.22, 0.24)
            iconName: "hint"
            accessibleName: SoloWording.showStart
            onClicked: root.solo.askHelp()
        }

        // The display: the board in small, with the start tile ringed.
        BoardThumb {
            visible: !root.pending && root.roomy
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: Theme.space2
            height: Math.min(parent.height - 2 * Theme.space2, Theme.hintDisplay)
            width: height
            rows: root.solo.rows
            columns: root.solo.columns
            tiles: root.solo.tiles
            showJewels: false
            Rectangle {
                visible: root.solo.helpTile >= 0
                readonly property real cell: parent.side / Math.max(parent.rows, parent.columns, 1)
                x: root.solo.helpTile >= 0 ? (root.solo.helpTile % parent.columns) * cell + (parent.width - parent.side) / 2 : 0
                y: root.solo.helpTile >= 0 ? Math.floor(root.solo.helpTile / parent.columns) * cell : 0
                width: cell
                height: cell
                color: "transparent"
                border.width: Theme.litEdge
                border.color: Theme.hintCardInk
            }
        }

        // Done: hands the tapped path in.
        AppButton {
            id: doneButton
            visible: root.pending
            anchors.centerIn: parent
            width: Math.max(Theme.targetChild, Math.min(parent.width - 2 * Theme.space2, Theme.doneWidth))
            height: Theme.targetChild
            variant: "primary"
            accessibleName: SoloWording.done
            onClicked: root.solo.done()

            Jewel {
                anchors.centerIn: parent
                width: 40
                height: 40
                tier: "power"
                color: Theme.dieLit
            }
            Icon {
                anchors.centerIn: parent
                width: 22
                height: 22
                name: "done"
                color: Theme.ink
                strokeWidth: 3
            }
        }
    }

    // --- history: earlier boards -------------------------------------------------------------
    Rectangle {
        id: history
        x: root.wide || !root.hintShown ? root.margin
           : (root.mirrored ? root.margin : hint.x + hint.width + root.margin)
        y: root.wide && root.hintShown ? hint.y + hint.height + root.margin
                                       : tray.y + tray.height + root.margin
        width: root.wide || !root.hintShown ? root.width - 2 * root.margin
                                            : (root.width - 3 * root.margin) / 2
        height: root.cardHeight
        color: Theme.card
        border.width: Theme.rule
        border.color: Theme.outline

        // Roomy: three thumbnails of earlier boards, inline. A tap opens the sheet.
        Row {
            visible: root.roomy
            anchors.centerIn: parent
            spacing: Theme.space2
            Repeater {
                model: Math.min(3, root.solo.history.length)
                BoardThumb {
                    required property int index
                    readonly property var entry: root.solo.history[index]
                    width: Math.min(Theme.thumbInline, (history.width - 4 * Theme.space2) / 3)
                    height: Math.min(history.height - 2 * Theme.space2, width + jewelRow)
                    rows: entry.rows
                    columns: entry.columns
                    tiles: entry.tiles
                    found: entry.found
                }
            }
        }
        MouseArea {
            visible: root.roomy
            anchors.fill: parent
            onClicked: root.historyRequested()
        }
        Accessible.role: Accessible.Button
        Accessible.name: SoloWording.earlierBoards
        Accessible.onPressAction: root.historyRequested()

        // Smaller: one button that opens the sheet.
        AppButton {
            visible: !root.roomy
            anchors.centerIn: parent
            variant: "secondary"
            iconName: "history"
            accessibleName: SoloWording.earlierBoards
            onClicked: root.historyRequested()
        }
    }
}
