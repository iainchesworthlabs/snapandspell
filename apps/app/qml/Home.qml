// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

// home: the primary way in is one large Play placard. A second, quieter card
// opens the grown-up area; every adult workflow — the real board, settings and
// progress — lives behind the gate there. The wordmark, the hero and the two
// cards say what the app is without asking a child to read a menu.
//
// The page is sized from the window it is given. At the 900 x 620 minimum the
// cards lose height rather than sliding under the bottom edge. Below 600 (a phone
// upright) nothing scrolls up and down: each card is a page of its own, as tall
// as the screen allows, with Play first and the grown-ups' card a swipe away (its
// edge shows beside Play, so a child can see there is more). In a window far
// larger than the design the column grows (to 1.5 times) and, when it is still
// shorter than the window, sits in the middle of it: the bar spans the width, so
// the content is never left top-left over an empty lower half. The two cards are
// one pair of components laid out by a grid, and each card gets its height from
// its own words (kit/ModeCard.qml), so a larger text size makes the card taller
// instead of drawing the words over its picture.
FocusScope {
    id: root

    required property var controller

    signal soloRequested
    signal parentRequested
    signal collectionRequested
    signal boardsRequested

    // Two cards side by side from a tablet upright (and a phone on its side) up;
    // one page each on a narrow window, a phone held upright.
    readonly property bool wide: !Theme.isNarrow(width)
    // A card to a page, with Play first and the grown-ups' card a swipe away: a phone upright (too
    // narrow for two cards) and a phone on its side (too low for the cards, the hero and the footer).
    readonly property bool paged: !wide || Theme.isShort(room)
    // Home has one primary Play picture and one adult picture; the name is kept
    // for the shell's checks and for a future first-run layout.
    readonly property int pictures: 2

    // The space the page has under the bar (which grows at a large text size).
    readonly property real room: Math.max(0, root.height - bar.height)
    readonly property bool small: room < 650
    readonly property real margin: small ? Theme.space4 : Theme.space8
    readonly property real gutter: small ? Theme.space4 : Theme.space6
    // How much bigger than the design the window is, to 1.5: the column and the
    // cards grow with it, the type does not (it follows the Text size setting).
    readonly property real zoom: Math.max(1, Math.min(root.width / Theme.pageWidth,
                                                      room / Theme.pageHeight, 1.5))

    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

    // The words of the two cards. They are here, not inside the cards' components, so that the
    // catalogues list them in the order they are written whichever version of lupdate reads them.
    readonly property string playSubtitle: qsTr("Shake the box. Find words. Collect jewels.")
    readonly property string playAction: qsTr("Play")
    readonly property string familySubtitle: qsTr("Use the camera, or type the board. A grown-up sets\u00a0this\u00a0up.")
    readonly property string familyAction: qsTr("Continue")

    // The two cards, once each in the wide page and once each in the phone's. Only one of the pages
    // is made, so a screen reader and a test find each card once.
    component PlayCard: ModeCard {
        primary: true
        eyebrow: HomeWording.soloEyebrow
        title: HomeWording.soloPicture
        subtitle: root.playSubtitle
        actionLabel: root.playAction
        onClicked: root.soloRequested()

        JewelBox {
            anchors.fill: parent
            closed: false
            tiles: ["S", "O", "S", "S", "A", "N", "N", "S", "L"]
        }
    }
    component FamilyCard: ModeCard {
        grownUp: true
        eyebrow: HomeWording.forGrownUps
        title: HomeWording.familyPicture
        subtitle: root.familySubtitle
        actionLabel: root.familyAction
        onClicked: root.parentRequested()

        TabletArt {
            anchors.fill: parent
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    AppBar {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        showBack: false
        transparent: true
        showWordmark: true
        showParent: true
        // Always there: a child who has just started can open it and see the
        // empty shelf with the way to play, rather than waiting for a first find
        // before the collection exists.
        showJewels: true
        // The boards played: the earlier boards are one press from home, not
        // buried in the play panel.
        showBoards: true
        onParentRequested: root.parentRequested()
        onJewelsRequested: root.collectionRequested()
        onBoardsRequested: root.boardsRequested()
    }

    Loader {
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        focus: true
        sourceComponent: root.paged ? phonePage : widePage
    }

    // --- a tablet, a laptop, a phone on its side ---------------------------------------
    Component {
        id: widePage

        Flickable {
            id: page

            // The cards take what the hero and the footer leave, up to their largest,
            // and never less than their own words and picture need.
            readonly property real cardHeight: Math.max(
                primaryCard.minimumHeight, familyCard.minimumHeight,
                Math.min(Theme.cardMax * root.zoom,
                         root.room - 2 * root.margin - hero.implicitHeight - footer.implicitHeight
                         - 2 * root.gutter - Theme.hair))

            contentHeight: Math.max(content.implicitHeight + 2 * root.margin, height)
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ColumnLayout {
                id: content
                width: Math.min(page.width - 2 * root.margin, Theme.pageWidth * root.zoom)
                x: (page.width - width) / 2
                // In the middle of a window taller than the column, otherwise below
                // the margin.
                y: Math.max(root.margin, (page.height - implicitHeight) / 2)
                spacing: root.gutter

                ColumnLayout {
                    id: hero
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: Theme.space2

                    Eyebrow {
                        Layout.fillWidth: true
                        text: qsTr("A word-finding game for young readers")
                        accented: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        Layout.fillWidth: true
                        role: root.small ? "title" : "display"
                        text: qsTr("Shake. Find. Collect.")
                        wrapMode: Text.WordWrap
                    }
                }

                GridLayout {
                    id: cards
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    columns: 2
                    columnSpacing: root.gutter
                    rowSpacing: root.gutter

                    PlayCard {
                        id: primaryCard
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: page.cardHeight
                        Layout.minimumHeight: minimumHeight
                        titleReserve: familyCard.titleNeeded
                        subtitleReserve: familyCard.subtitleNeeded
                        buttonReserve: familyCard.buttonNeeded
                    }

                    FamilyCard {
                        id: familyCard
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: page.cardHeight
                        Layout.minimumHeight: minimumHeight
                        titleReserve: primaryCard.titleNeeded
                        subtitleReserve: primaryCard.subtitleNeeded
                        buttonReserve: primaryCard.buttonNeeded
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.hair
                    color: Theme.line
                }

                // The two labels keep their own edges and elide only if the window is
                // truly too narrow for both.
                RowLayout {
                    id: footer
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: Theme.space4

                    Eyebrow {
                        text: qsTr("Snap and Spell")
                        elide: Text.ElideRight
                        Layout.minimumWidth: 0
                        Layout.maximumWidth: content.width * 0.42
                    }
                    Label {
                        role: "caption"
                        text: qsTr("Everything stays on this device.")
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                    }
                }
            }
        }
    }

    // --- a phone held upright ----------------------------------------------------------
    // The name of the game, the cards one to a page, a pair of dots, and the promise. All of it is
    // in view at once; the second card is a swipe to the side.
    Component {
        id: phonePage

        Item {
            id: phone

            // How much of the next card shows beside the one in hand.
            readonly property real peek: Theme.space8
            readonly property real cardWidth: Math.max(0, width - 2 * root.margin - peek)

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: root.margin
                anchors.rightMargin: root.margin
                anchors.topMargin: Theme.space2
                anchors.bottomMargin: Theme.space2
                spacing: Theme.space2

                // On its side a phone is too low for the name of the game as well: the cards take the room.
                Label {
                    visible: root.wide === false
                    Layout.fillWidth: true
                    role: "title"
                    text: qsTr("Shake. Find. Collect.")
                    wrapMode: Text.WordWrap
                }

                ListView {
                    id: pager
                    objectName: "homePager"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: -root.margin
                    Layout.rightMargin: -root.margin
                    orientation: ListView.Horizontal
                    layoutDirection: Theme.rightToLeft ? Qt.RightToLeft : Qt.LeftToRight
                    boundsBehavior: Flickable.StopAtBounds
                    spacing: root.gutter
                    clip: true
                    // The card in hand sits at the margin; the next peeks from the other side.
                    highlightRangeMode: ListView.StrictlyEnforceRange
                    preferredHighlightBegin: root.margin
                    preferredHighlightEnd: root.margin + phone.cardWidth
                    highlightMoveDuration: Theme.reducedMotion ? 0 : Theme.turn
                    snapMode: ListView.SnapOneItem
                    model: 2

                    delegate: Item {
                        id: slot
                        required property int index
                        width: phone.cardWidth
                        height: pager.height

                        // A card taller than the page (a large text size) scrolls up and down on its own;
                        // a sideways swipe still goes to the pager.
                        Flickable {
                            id: scroller
                            anchors.fill: parent
                            contentWidth: width
                            contentHeight: Math.max(height, cardLoader.cardMinimum + Theme.shadowLarge)
                            flickableDirection: Flickable.VerticalFlick
                            boundsBehavior: Flickable.StopAtBounds
                            clip: true

                            Loader {
                                id: cardLoader
                                // The card's own least height: its words and the least picture.
                                readonly property real cardMinimum: item ? (item as ModeCard).minimumHeight : 0
                                focus: slot.index === 0
                                width: parent.width
                                // The card's hard shadow hangs under it.
                                height: scroller.contentHeight - Theme.shadowLarge
                                sourceComponent: slot.index === 0 ? playPage : familyPage
                            }
                        }
                    }
                }

                // Which card is in hand: two dots, the one in hand filled.
                Row {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Theme.space3
                    Repeater {
                        model: 2
                        Rectangle {
                            id: dot
                            required property int index
                            width: Theme.space3
                            height: Theme.space3
                            radius: width / 2
                            color: dot.index === pager.currentIndex ? Theme.ink : "transparent"
                            border.width: Theme.outlineControl
                            border.color: Theme.line
                            Accessible.ignored: true
                        }
                    }
                }

                Label {
                    visible: root.wide === false
                    Layout.fillWidth: true
                    role: "caption"
                    text: qsTr("Everything stays on this device.")
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }
        }
    }

    Component {
        id: playPage
        PlayCard {
            compact: true
            sideways: root.wide
        }
    }
    Component {
        id: familyPage
        FamilyCard {
            compact: true
            sideways: root.wide
        }
    }
}
