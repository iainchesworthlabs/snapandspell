// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

// The child's own collection, reached from home. It is where the weekly stars,
// the words and the earlier boards live, so the play panel can be about the
// board in front of the child. Nothing here is a rank against anyone else: the
// records are this device's own, and there is nothing to lose.
//
// Every block is a kit Card (a heading and, when there is nothing in it, an
// empty state); the best boards are a grid of three columns, not a line padded
// with spaces. Gold is the reward and appears only where one is shown: the big
// jewel and the points beside it, and each found word's jewel.
FocusScope {
    id: root

    required property var controller

    signal closed
    signal playRequested

    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property var week: controller.dashboard.week
    readonly property var vault: controller.dashboard.vault
    readonly property var recent: controller.recentBoards
    readonly property var words: vault.length > 40 ? vault.slice(vault.length - 40) : vault
    readonly property bool empty: root.controller.lifetimeWords === 0
                                        && root.controller.lifetimeStars === 0
    // The child's own best board at each size, from the boards still kept. This
    // is a personal wall, not a rank against another child: no name, no order
    // beyond the size, and no one else's score exists to compare with.
    readonly property var bestSizes: [3, 4, 5, 6]
    readonly property bool anyBest: root.bestSizes.some(size => root.bestStars(size) > 0)
    // The cells of the best-boards grid, three to a row: the size, the stars and
    // the words of that board. A size with no board kept has no row.
    readonly property var bestCells: {
        const cells = [];
        for (const size of root.bestSizes) {
            const stars = root.bestStars(size);
            if (stars > 0) {
                cells.push(JewelsWording.sizeLabel(size), JewelsWording.stars(stars),
                           JewelsWording.words(root.bestWords(size)));
            }
        }
        return cells;
    }

    function bestStars(size) {
        let best = 0;
        for (let i = 0; i < root.recent.length; ++i) {
            if (root.recent[i].columns === size) {
                best = Math.max(best, root.recent[i].stars);
            }
        }
        return best;
    }
    function bestWords(size) {
        let best = 0;
        for (let i = 0; i < root.recent.length; ++i) {
            if (root.recent[i].columns === size && root.recent[i].stars === root.bestStars(size)) {
                best = Math.max(best, root.recent[i].found);
            }
        }
        return best;
    }

    // A found word: its jewel and the word, pressed to hear it. A sticker on the
    // card (the page's own paper and its edge), reachable by keyboard.
    component WordChip: Rectangle {
        id: chip

        required property int index
        required property var modelData
        readonly property bool showFocus: activeFocus && !pointerFocus
        property bool pointerFocus: false

        implicitWidth: chipRow.implicitWidth + 2 * Theme.space3
        implicitHeight: Theme.targetMin
        radius: height / 2
        color: chipTap.pressed ? Theme.pressWash : (chipHover.hovered ? Theme.hoverWash : Theme.panel)
        border.width: Theme.hair
        border.color: Theme.line
        activeFocusOnTab: true

        Accessible.role: Accessible.Button
        Accessible.name: chip.modelData.word
        Accessible.focusable: true
        Accessible.onPressAction: Feedback.cue("speak", chip.modelData.word)

        Row {
            id: chipRow
            anchors.centerIn: parent
            spacing: Theme.space2

            Jewel {
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.jewelSm
                height: width
                tier: chip.modelData.word.length <= 3 ? "spark"
                      : (chip.modelData.word.length === 4 ? "power" : "mega")
                colorIndex: chip.index
                Accessible.ignored: true
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                role: "word"
                text: chip.modelData.word
                decorative: true
            }
        }

        FocusRing {
            visible: chip.showFocus
            inner: chip.radius
        }
        HoverHandler {
            id: chipHover
        }
        TapHandler {
            id: chipTap
            onPressedChanged: {
                if (chipTap.pressed) {
                    chip.pointerFocus = true;
                }
            }
            onTapped: Feedback.cue("speak", chip.modelData.word)
        }
        Keys.onPressed: event => {
            chip.pointerFocus = false;
            const acts = event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
            if (acts) {
                Feedback.cue("speak", chip.modelData.word);
            }
            event.accepted = acts;
        }
        onActiveFocusChanged: {
            if (!chip.activeFocus) {
                chip.pointerFocus = false;
            }
        }
    }

    // One of the page's big numbers: a small label, the figure in six digits, and what it counts.
    component StatPanel: Rectangle {
        id: stat

        property string label: ""
        property int value: 0
        property string caption: ""
        property bool gem: false

        Layout.fillWidth: true
        implicitHeight: statColumn.implicitHeight + 2 * Theme.space4
        radius: Theme.radiusPanel
        color: Theme.card
        border.width: Theme.outlinePanel
        border.color: Theme.line
        Accessible.role: Accessible.StaticText
        Accessible.name: stat.label + ": " + stat.value + ", " + stat.caption

        Rectangle {
            z: -1
            y: Theme.shadowLarge
            width: parent.width
            height: parent.height
            radius: parent.radius
            color: Theme.shadow
        }

        ColumnLayout {
            id: statColumn
            anchors.fill: parent
            anchors.margins: Theme.space4
            spacing: Theme.space1

            RowLayout {
                Layout.fillWidth: true
                // The same height whether or not the panel carries a jewel, so the three line up.
                Layout.preferredHeight: Theme.jewelMd
                Eyebrow {
                    Layout.fillWidth: true
                    text: stat.label
                    decorative: true
                }
                Jewel {
                    visible: stat.gem
                    Layout.preferredWidth: Theme.jewelMd
                    Layout.preferredHeight: Theme.jewelMd
                    tier: "mega"
                    color: Theme.gold
                    Accessible.ignored: true
                }
            }
            Label {
                Layout.fillWidth: true
                role: "numeral"
                learning: true
                size: Theme.titleSize
                text: JewelsWording.padded(stat.value)
                decorative: true
            }
            Label {
                Layout.fillWidth: true
                role: "body"
                learning: true
                text: stat.caption
                decorative: true
                color: Theme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }
    }

    // The week as seven bars, one a day, the first day of the week first.
    component WeekPanel: Card {
        id: weekPanel

        title: JewelsWording.weekTitle
        Accessible.name: JewelsWording.weekBars(root.week.starsByDay)

        readonly property int most: Math.max(1, ...root.week.starsByDay)
        readonly property real barArea: Theme.space16 * 2

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: weekPanel.barArea + Theme.space6
            spacing: Theme.space2

            Repeater {
                model: root.week.starsByDay

                ColumnLayout {
                    id: bar
                    required property int index
                    required property int modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Theme.space1

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            // A day with nothing is a sliver, so the week still reads as seven days.
                            height: Math.max(Theme.outlineControl, parent.height * bar.modelData / weekPanel.most)
                            color: Theme.coral
                            border.width: Theme.outlineControl
                            border.color: Theme.line
                            Accessible.ignored: true
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        role: "caption"
                        horizontalAlignment: Text.AlignHCenter
                        decorative: true
                        text: JewelsWording.dayInitial((root.week.firstWeekday - 1 + bar.index) % 7 + 1)
                    }
                }
            }
        }
    }

    component WordsCard: Card {
        Layout.fillWidth: true
        visible: !root.empty
        title: JewelsWording.shelfTitle
        empty: root.words.length === 0
        emptyText: JewelsWording.shelfEmpty

        Flow {
            Layout.fillWidth: true
            spacing: Theme.space2

            Repeater {
                model: root.words

                WordChip {
                }
            }
        }
    }

    component BoardsCard: Card {
        Layout.fillWidth: true
        visible: !root.empty
        title: JewelsWording.boardsTitle
        empty: root.recent.length === 0
        emptyText: JewelsWording.boardsEmpty

        Flow {
            Layout.fillWidth: true
            spacing: Theme.space4

            Repeater {
                model: root.recent.slice(0, 8)

                Column {
                    id: boardCard
                    required property var modelData
                    width: Theme.thumbSheetCell
                    spacing: Theme.space1

                    BoardThumb {
                        width: parent.width
                        height: width
                        rows: boardCard.modelData.rows
                        columns: boardCard.modelData.columns
                        tiles: boardCard.modelData.tiles
                        found: boardCard.modelData.words
                    }
                    Label {
                        width: parent.width
                        role: "caption"
                        text: JewelsWording.boardLine(boardCard.modelData.stars,
                                                      boardCard.modelData.found)
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        root.controller.refresh();
        root.forceActiveFocus();
    }

    Keys.onEscapePressed: event => {
        root.closed();
        event.accepted = true;
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    AppBar {
        id: appBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        showBack: true
        showWordmark: false
        showParent: false
        title: JewelsWording.title
        onBack: root.closed()
    }

    Flickable {
        id: page
        anchors.top: appBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentWidth: width
        contentHeight: column.implicitHeight + 2 * Theme.space6
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: column
            width: Math.min(page.width - 2 * Theme.space6, Theme.pageWidth)
            x: (page.width - width) / 2
            y: Theme.space6
            spacing: Theme.space6

            // The page is two columns where the window has the width for them (the week's bars
            // stand beside the rest) and one where it has not.
            GridLayout {
                id: main
                Layout.fillWidth: true
                readonly property bool sideBySide: page.width >= Theme.expandedWidth
                columns: main.sideBySide ? 2 : 1
                columnSpacing: Theme.space6
                rowSpacing: Theme.space6

                ColumnLayout {
                    id: leftColumn
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.space6

                    // --- the three big numbers --------------------------------------
                    GridLayout {
                        Layout.fillWidth: true
                        visible: !root.empty
                        columns: leftColumn.width >= Theme.modalMax ? 3 : 1
                        columnSpacing: Theme.space4
                        rowSpacing: Theme.space4

                        StatPanel {
                            label: JewelsWording.statStars
                            value: root.controller.lifetimeStars
                            caption: JewelsWording.statAllBoards
                            gem: true
                        }
                        StatPanel {
                            label: JewelsWording.statPoints
                            value: root.controller.lifetimePoints
                            caption: JewelsWording.statAllBoards
                        }
                        StatPanel {
                            label: JewelsWording.statWords
                            value: root.controller.lifetimeWords
                            caption: JewelsWording.statFoundSoFar
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: !root.empty && root.controller.longestWord.length > 0
                        role: "body"
                        learning: true
                        text: JewelsWording.longest(root.controller.longestWord)
                        color: Theme.inkSecondary
                        wrapMode: Text.WordWrap
                    }

                    // The week's bars sit here where there is one column.
                    WeekPanel {
                        Layout.fillWidth: true
                        visible: !main.sideBySide && root.week.played
                    }

                    // --- my best boards ---------------------------------------------------
                    Card {
                        Layout.fillWidth: true
                        visible: root.anyBest
                        title: JewelsWording.bestTitle

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 3
                            columnSpacing: Theme.space6
                            rowSpacing: Theme.space2

                            Repeater {
                                model: root.bestCells

                                Label {
                                    required property int index
                                    required property string modelData
                                    // The size leads its row; the stars and the words
                                    // follow in the voice of the page.
                                    role: index % 3 === 0 ? "label" : "body"
                                    lineRatio: Theme.lineBody
                                    learning: index % 3 !== 0
                                    text: modelData
                                    Layout.fillWidth: index % 3 === 2
                                    Layout.alignment: Qt.AlignBaseline
                                }
                            }
                        }
                    }

                    // The words found, and the boards played.
                    WordsCard {
                        Layout.fillWidth: true
                    }
                    BoardsCard {
                        Layout.fillWidth: true
                    }
                }

                // The week's bars stand beside the rest where there are two columns.
                WeekPanel {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: Theme.modalMax / 2
                    Layout.maximumWidth: Theme.modalMax / 2
                    visible: main.sideBySide && root.week.played
                }
            }

            // --- a first visit ------------------------------------------------------
            Card {
                Layout.fillWidth: true
                visible: root.empty
                empty: true
                emptyTitle: JewelsWording.emptyTitle
                emptyText: JewelsWording.emptyBody
            }
            AppButton {
                Layout.alignment: Qt.AlignHCenter
                visible: root.empty
                size: "primary"
                variant: "primary"
                label: JewelsWording.play
                onClicked: root.playRequested()
            }
        }
    }
}
