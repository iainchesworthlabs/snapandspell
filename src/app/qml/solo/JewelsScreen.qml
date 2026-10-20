pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// The child's own collection, reached from home. It is where the weekly stars,
// the words and the earlier boards live, so the play panel can be about the
// board in front of the child. Nothing here is a rank against anyone else: the
// records are this device's own, and there is nothing to lose.
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

        Column {
            id: column
            width: Math.min(page.width - 2 * Theme.space6, 980)
            x: (page.width - width) / 2
            y: Theme.space6
            spacing: Theme.space6

            // --- the big picture ------------------------------------------------
            Rectangle {
                width: parent.width
                implicitHeight: hero.implicitHeight + 2 * Theme.space6
                radius: Theme.radiusLarge
                color: Theme.card
                border.width: 1
                border.color: Theme.gold

                RowLayout {
                    id: hero
                    anchors.fill: parent
                    anchors.margins: Theme.space6
                    spacing: Theme.space6

                    Jewel {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 72
                        Layout.alignment: Qt.AlignVCenter
                        tier: "mega"
                        color: Theme.gold
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space1
                        Text {
                            Layout.fillWidth: true
                            text: JewelsWording.jewels(root.controller.lifetimeStars)
                            font.family: Theme.uiFont
                            font.pixelSize: Theme.titleSize
                            font.weight: Font.ExtraBold
                            color: Theme.ink
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: JewelsWording.points(root.controller.lifetimePoints)
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.labelSize
                            color: Theme.goldInk
                        }
                        Text {
                            Layout.fillWidth: true
                            text: JewelsWording.words(root.controller.lifetimeWords)
                                   + (root.controller.longestWord.length > 0
                                      ? "  ·  " + JewelsWording.longest(root.controller.longestWord) : "")
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.bodySize
                            color: Theme.inkSecondary
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            // --- my best boards ---------------------------------------------------
            Rectangle {
                width: parent.width
                visible: root.anyBest
                implicitHeight: bestColumn.implicitHeight + 2 * Theme.space4
                radius: Theme.radius
                color: Theme.card
                border.width: 1
                border.color: Theme.line

                Column {
                    id: bestColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space4
                    spacing: Theme.space2

                    Text {
                        text: JewelsWording.bestTitle
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.headingSize
                        font.weight: Font.DemiBold
                        color: Theme.ink
                    }
                    Repeater {
                        model: root.bestSizes

                        Text {
                            required property int modelData
                            visible: root.bestStars(modelData) > 0
                            text: JewelsWording.bestLine(modelData, root.bestStars(modelData),
                                                         root.bestWords(modelData))
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.labelSize
                            color: Theme.ink
                        }
                    }
                }
            }

            // --- this week -------------------------------------------------------
            Rectangle {
                width: parent.width
                visible: root.week.played
                implicitHeight: weekColumn.implicitHeight + 2 * Theme.space4
                radius: Theme.radius
                color: Theme.card
                border.width: 1
                border.color: Theme.line

                ColumnLayout {
                    id: weekColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space4
                    spacing: Theme.space1
                    Text {
                        text: JewelsWording.weekTitle
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.headingSize
                        font.weight: Font.DemiBold
                        color: Theme.ink
                    }
                    Text {
                        text: JewelsWording.weekLine(root.week.stars, root.week.newWords)
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                    }
                    Text {
                        visible: root.week.longestWord.length > 0
                        text: JewelsWording.longest(root.week.longestWord)
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                    }
                }
            }

            // --- the words -------------------------------------------------------
            Rectangle {
                width: parent.width
                visible: !root.empty
                implicitHeight: shelfColumn.implicitHeight + 2 * Theme.space4
                radius: Theme.radius
                color: Theme.card
                border.width: 1
                border.color: Theme.line

                Column {
                    id: shelfColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space4
                    spacing: Theme.space3

                    Text {
                        text: JewelsWording.shelfTitle
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.headingSize
                        font.weight: Font.DemiBold
                        color: Theme.ink
                    }
                    Text {
                        visible: root.words.length === 0
                        text: JewelsWording.shelfEmpty
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                    }
                    Flow {
                        width: parent.width
                        spacing: Theme.space2

                        Repeater {
                            model: root.words

                            Rectangle {
                                id: chip
                                required property int index
                                required property var modelData
                                width: chipRow.implicitWidth + 2 * Theme.space3
                                height: 48
                                radius: 24
                                color: Theme.accentSoft
                                border.width: 1
                                border.color: Theme.accent

                                Row {
                                    id: chipRow
                                    anchors.centerIn: parent
                                    spacing: Theme.space2
                                    Jewel {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 22
                                        height: 22
                                        tier: chip.modelData.word.length <= 3 ? "spark"
                                              : (chip.modelData.word.length === 4 ? "power" : "mega")
                                        colorIndex: chip.index
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: chip.modelData.word
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.labelSize
                                        color: Theme.ink
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: Feedback.cue("speak", chip.modelData.word)
                                }
                                Accessible.role: Accessible.Button
                                Accessible.name: chip.modelData.word
                                Accessible.onPressAction: Feedback.cue("speak", chip.modelData.word)
                            }
                        }
                    }
                }
            }

            // --- earlier boards ---------------------------------------------------
            Rectangle {
                width: parent.width
                visible: !root.empty
                implicitHeight: boardsColumn.implicitHeight + 2 * Theme.space4
                radius: Theme.radius
                color: Theme.card
                border.width: 1
                border.color: Theme.line

                Column {
                    id: boardsColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space4
                    spacing: Theme.space3

                    Text {
                        text: JewelsWording.boardsTitle
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.headingSize
                        font.weight: Font.DemiBold
                        color: Theme.ink
                    }
                    Text {
                        visible: root.recent.length === 0
                        text: JewelsWording.boardsEmpty
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                    }
                    Flow {
                        width: parent.width
                        spacing: Theme.space4

                        Repeater {
                            model: root.recent.slice(0, 8)

                            Column {
                                id: boardCard
                                required property var modelData
                                width: 142
                                spacing: Theme.space1

                                BoardThumb {
                                    width: 132
                                    height: 132
                                    rows: boardCard.modelData.rows
                                    columns: boardCard.modelData.columns
                                    tiles: boardCard.modelData.tiles
                                    found: boardCard.modelData.words
                                }
                                Text {
                                    width: parent.width
                                    text: JewelsWording.boardLine(boardCard.modelData.stars,
                                                                  boardCard.modelData.found)
                                    font.family: Theme.monoFont
                                    font.pixelSize: Theme.monoSize
                                    color: Theme.inkSecondary
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            // --- a first visit ------------------------------------------------------
            Column {
                width: parent.width
                visible: root.empty
                spacing: Theme.space2
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: JewelsWording.emptyTitle
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.headingSize
                    font.weight: Font.DemiBold
                    color: Theme.ink
                }
                Text {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: JewelsWording.emptyBody
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                    wrapMode: Text.WordWrap
                }
                AppButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    size: "primary"
                    variant: "primary"
                    label: JewelsWording.play
                    onClicked: root.playRequested()
                }
            }
        }
    }
}
