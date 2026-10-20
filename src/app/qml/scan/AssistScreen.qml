pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// assist.playing (docs/design/game.md, "Playing"): the confirmed board in the
// leading two-thirds and the panel in the trailing third, the same split rule as
// Solo Play (SoloLayout, board in the top two-thirds of a tall window). The app
// is not the game: there is no scoreboard against the other players, no timer,
// no word count, and no "New board" that a camera noticed.
//
// The panel holds the words found (as the same jewels and stars as Solo Play),
// the one help (the start tile), and New board. The camera is not here: a new
// round is a new scan, and this screen's New board asks for one.
FocusScope {
    id: root

    required property AssistModel assist

    // Back: to the check screen the person came from (the board is recorded).
    signal closed
    // A new round: the scan screen, ready, from the start.
    signal newBoardRequested

    readonly property real barOffset: Theme.barHeight
    readonly property bool wide: rule.wide
    // The interface's direction, set by the shell: the split mirrors, the replica
    // of the board does not. For the RTL check in the stub shell.
    property bool rightToLeft: Theme.rightToLeft
    readonly property real panelX: panel.x
    readonly property real boardX: board.x
    readonly property bool mirrored: LayoutMirroring.enabled

    LayoutMirroring.enabled: root.rightToLeft
    LayoutMirroring.childrenInherit: true

    activeFocusOnTab: true

    SoloLayout {
        id: rule
        width: root.width
        height: root.height - root.barOffset
        boardSize: Math.max(root.assist.rows, 3)
        // The replica stays the family's board, left to right as it was confirmed;
        // the split around it follows the window's direction.
        rightToLeft: root.mirrored
    }

    readonly property rect boardArea: Qt.rect(rule.boardArea.x, rule.boardArea.y + root.barOffset,
                                              rule.boardArea.width, rule.boardArea.height)
    readonly property rect panelArea: Qt.rect(rule.panelArea.x, rule.panelArea.y + root.barOffset,
                                              rule.panelArea.width, rule.panelArea.height)

    // The board is laid out by the same rule as Solo Play's; the touch geometry
    // is in the board's own pixels, which is what PathTracer takes.
    function relayout() {
        if (root.assist.rows > 0) {
            root.assist.setGeometry(rule.tile, rule.gap);
        }
    }

    Component.onCompleted: root.relayout()
    Component.onDestruction: root.assist.leave()

    Connections {
        target: rule

        function onChanged() {
            root.relayout();
        }
    }

    // The visible way back, Esc and the system back: the history closes first if
    // it is open, then the board is saved to the store and the person goes where
    // they came from.
    function back() {
        if (history.open) {
            history.close();
            return;
        }
        root.assist.leave();
        root.closed();
    }

    function newBoard() {
        root.assist.leave();
        root.newBoardRequested();
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    // --- the bar ------------------------------------------------------------------
    AppBar {
        id: bar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        showBack: true
        showWordmark: false
        showParent: false
        title: qsTr("Find words")
        onBack: root.back()
    }

    // --- the replica, leading two-thirds --------------------------------------------
    Rectangle {
        x: root.boardArea.x
        y: root.boardArea.y
        width: root.boardArea.width
        height: root.boardArea.height
        color: Theme.boardArea
    }

    AssistBoard {
        id: board
        x: root.boardArea.x + rule.boardRect.x
        y: root.boardArea.y + rule.boardRect.y
        width: rule.boardRect.width
        height: rule.boardRect.height
        tile: rule.tile
        gap: rule.gap
        assist: root.assist
        // The replica is the board the family confirmed, read left to right as
        // the real one was: it does not mirror.
        LayoutMirroring.enabled: false
        LayoutMirroring.childrenInherit: true
    }

    // --- the panel, trailing third ---------------------------------------------------
    Item {
        id: panel
        x: root.panelArea.x
        y: root.panelArea.y
        width: root.panelArea.width
        height: root.panelArea.height

        readonly property real margin: Theme.space2
        readonly property real actions: root.assist.helpAllowed
                                        ? Theme.targetPrimary : Theme.targetChild

        Rectangle {
            anchors.fill: parent
            color: Theme.surface
        }
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Theme.hair
            color: Theme.line
        }

        // Words I found.
        Rectangle {
            id: tray
            x: panel.margin
            y: panel.margin
            width: parent.width - 2 * panel.margin
            height: parent.height - 2 * panel.margin - panel.actions - Theme.space2
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.line
            clip: true

            Accessible.role: Accessible.List
            Accessible.name: ScanWording.wordsFound

            Text {
                id: trayTitle
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.space4
                text: ScanWording.wordsFound
                font.family: Theme.uiFont
                font.weight: Font.DemiBold
                font.pixelSize: Theme.headingSize
                color: Theme.ink
                elide: Text.ElideRight
            }

            ListView {
                id: words
                anchors.top: trayTitle.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: Theme.space4
                anchors.rightMargin: Theme.space4
                anchors.bottomMargin: Theme.space4
                clip: true
                model: root.assist.found
                spacing: Theme.space1
                onCountChanged: positionViewAtEnd()

                // A jewel drops in; with reduced motion it appears.
                add: Transition {
                    enabled: !Theme.reducedMotion
                    NumberAnimation {
                        property: "y"
                        from: -Theme.space8
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

                delegate: Item {
                    id: row
                    required property int index
                    required property var modelData

                    width: words.width
                    height: Math.min(Theme.targetChild, tray.height * 0.3)

                    Accessible.role: Accessible.ListItem
                    Accessible.name: ScanWording.foundName(row.modelData.word)
                    Accessible.onPressAction: Feedback.cue("speak", row.modelData.word)

                    Jewel {
                        id: jewel
                        anchors.verticalCenter: parent.verticalCenter
                        x: Theme.space1
                        tier: row.modelData.tier
                        colorIndex: row.index
                    }

                    Text {
                        anchors.left: jewel.right
                        anchors.leftMargin: Theme.space2
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.word
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.wordSize
                        color: Theme.ink
                        elide: Text.ElideRight
                    }
                }
            }

            Column {
                visible: root.assist.found.length === 0
                anchors.centerIn: parent
                spacing: Theme.space2
                Jewel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Theme.jewelPower
                    height: width
                    tier: "power"
                }
                Text {
                    text: ScanWording.noWordsYet
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                }
            }
        }

        // The one help: the start tile, asked for and never offered. Absent when
        // the parent has turned help off.
        AppButton {
            id: showStart
            visible: root.assist.helpAllowed
            anchors.left: parent.left
            anchors.leftMargin: panel.margin
            anchors.bottom: parent.bottom
            anchors.bottomMargin: panel.margin
            width: Math.max(panel.width - 2 * panel.margin - newBoard.width
                            - historyButton.width - 2 * Theme.space2,
                            Theme.targetChild)
            height: Theme.targetPrimary
            size: "primary"
            variant: "primary"
            label: ScanWording.showStart
            labelPointSize: Theme.smallSize
            accessibleName: ScanWording.showStart
            onClicked: root.assist.askHelp()
        }

        // Earlier boards, the same history as Solo Play's: the child opens it on
        // purpose, and it is never offered after a board.
        AppButton {
            id: historyButton
            anchors.right: newBoard.left
            anchors.rightMargin: Theme.space2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: panel.margin
            width: Theme.targetChild
            height: Theme.targetChild
            size: "child"
            variant: "secondary"
            iconName: "history"
            accessibleName: ScanWording.earlierBoards
            onClicked: history.open = true
        }

        AppButton {
            id: newBoard
            anchors.right: parent.right
            anchors.rightMargin: panel.margin
            anchors.bottom: parent.bottom
            anchors.bottomMargin: panel.margin
            width: Math.max(Theme.targetChild, Math.min(parent.width / 3, Theme.doneWidth))
            height: Theme.targetChild
            size: "child"
            variant: "secondary"
            label: ScanWording.newBoard
            labelPointSize: Theme.smallSize
            accessibleName: ScanWording.newBoard
            onClicked: root.newBoard()
        }
    }

    // Earlier boards, over the screen: newest first, the store's last twenty.
    AssistHistory {
        id: history
        assist: root.assist
    }
}
