// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

// The panel beside the flat board: the words found, and under them one zone of
// actions - the word in hand, the hint, Finish board, and Clear and Done - that
// follow one height ladder (64 dp, with Done at 96) and never clip, wrapping their
// labels rather than cutting them. Every card has a title and an empty state; no
// card is a blank rectangle.
//
// The panel is one full-height column in a wide window and two columns in a tall
// one (the words beside the actions: a bottom third is too short for both stacked,
// which once left the words card a 4 px sliver). Nothing moves when a path starts:
// the line for the word in hand has its height reserved, and so does the hint's
// words, which share it. If a window and a text size leave less room than the
// actions need, the panel scrolls instead of hiding one under the edge. In the
// tall layout (a phone held upright) and in a short one (a phone on its side) Clear
// and Done are the foot of the panel and do not scroll with the rest: the way on is
// always in reach.
Item {
    id: root

    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

    required property var solo
    required property bool wide
    // The line over the actions: the word being made, or what just happened.
    property string statusText: ""
    property string statusDetail: ""
    property bool statusWord: false
    signal finishRequested

    readonly property bool helpShown: solo.helpAllowed
    readonly property bool pending: solo.path.length > 0
    readonly property bool mirrored: Theme.rightToLeft
    readonly property int hintLevel: solo.hintLevel
    // At the parent's middle level ("start") the first press lights the tile
    // and the next press hides it; only the fullest level offers the second
    // rung.
    readonly property string hintLabel: hintLevel === 0 ? SoloWording.showStart
                                      : (hintLevel === 1 && solo.helpLevel === "two")
                                        ? SoloWording.twoLetters
                                        : SoloWording.hideHint
    readonly property string hintHelp: hintLevel === 0 ? ""
                                     : hintLevel === 1 ? SoloWording.startHintShown
                                                       : SoloWording.twoLettersShown
    // The answer to the last question: a find made from the lit start puts the
    // rings away, and this says whether the same letters still begin more of the
    // board's words.
    readonly property string hintAnswer: !solo.hintResolved ? ""
                                         : solo.hintRemaining > 0
                                           ? SoloWording.moreWordsWithStart(solo.hintRemaining)
                                           : SoloWording.noMoreWordsWithStart
    readonly property string hintLine: hintAnswer.length > 0 ? hintAnswer : hintHelp

    // The clock the parent set for the board, shown at the top of the words.
    readonly property bool clockShown: solo.clockMode !== "off"
    readonly property bool actionsShown: solo.state === "playing" || solo.state === "word-found"
    readonly property string actionLine: actionsShown && solo.path.length > 0
                                         ? qsTr("Tap the last die again, or press Done")
                                         : ""
    // What the line over the actions says: the word in hand (or what just
    // happened) first, the hint's words when there is nothing in hand.
    readonly property bool inHand: statusText.length > 0 || solo.path.length > 0
    readonly property bool hintOnly: !inHand && helpShown && hintLine.length > 0

    readonly property string wordCount: solo.found.length === 1 ? qsTr("1 word")
                                                                 : qsTr("%1 words").arg(solo.found.length)
    readonly property string pointCount: qsTr("%1 points").arg(solo.boardPoints)

    // A tall panel (a phone upright) is tighter all round: it has to hold everything in view at once.
    // Two columns (the words beside the word in hand and the hint) in a window that is wide and short,
    // a phone on its side, where the board's square leaves the panel wide.
    readonly property bool twoColumns: wide && Theme.isShort(height) && width >= Theme.twoColumnsWidth
    // Tight: a tall panel or a short one, which hold everything in view at once, so every part of
    // them is a little smaller.
    readonly property bool tight: !wide || twoColumns
    readonly property real margin: tight ? Theme.space3 : Theme.space4
    readonly property real gap: tight ? Theme.space2 : Theme.space3
    // Above the first row and under the foot: a tight panel gives the board every dp it can.
    readonly property real vMargin: tight ? Theme.space2 : Theme.space4
    // Whole rows of words the card always has room for: two beside the board, one under it (a taller
    // panel gives it more).
    readonly property int leastRows: tight ? 1 : 2
    // One row of word chips and the gap under it.
    readonly property real chipPitch: Theme.rowHeight + Theme.space1
    // Clear and Done are the foot of the panel, not in the zone's column, where the panel is tall (a
    // phone upright) or short (a phone on its side): there the rest scrolls above them.
    readonly property bool footed: !wide || Theme.isShort(height)
    // The tall layout's second column.
    readonly property real zoneWidth: Math.min(width * 0.45, Theme.modalMax - Theme.space16)
    // The line's reserved height: a word and its small line, or two lines of the hint.
    readonly property real stripHeight: Math.max(
        Theme.wordSize * Theme.lineWord + Theme.captionSize * Theme.lineCaption + Theme.space1,
        tight ? 2 * captionRuler.implicitHeight + 2 * Theme.space2
              : 2 * Theme.bodySize * Theme.lineBody + 2 * Theme.space2)

    // The line heights the tall panel is counted from: a text's own height, not the design's ratio, since
    // the faces' metrics make a line taller than size times ratio.
    Label { id: wordRuler; visible: false; decorative: true; role: "word"; text: "Ag" }
    Label { id: captionRuler; visible: false; decorative: true; role: "caption"; learning: true; text: "Ag" }
    Label { id: headingRuler; visible: false; decorative: true; role: "heading"; text: "Ag" }

    // The least height a tall panel (a phone upright) needs for everything in it to be in view at once:
    // the word in hand, the words found in one whole row, the hint and Finish board, and the foot with
    // Clear and Done. The screen gives the board the rest of the window; the board never takes it. It
    // is written from the kit alone, not from `wide`, which the screen's layout rule decides from this.
    readonly property real tallNeed: 2 * Theme.space2
                                     + Math.max(wordRuler.implicitHeight + captionRuler.implicitHeight + Theme.space1,
                                                2 * captionRuler.implicitHeight + 2 * Theme.space2)
                                     + 2 * Theme.space3
                                     + Math.max(headingRuler.implicitHeight, 2 * captionRuler.implicitHeight)
                                     + 3 * Theme.space1 + Theme.outlineControl + Theme.rowHeight
                                     + Theme.targetMin + Theme.targetPrimary + 3 * Theme.space2
                                     // What the layout adds beyond the sum of its parts.
                                     + Theme.space2

    function currentWord() {
        var letters = [];
        for (var i = 0; i < solo.path.length; ++i) {
            const tile = solo.tiles[solo.path[i]];
            letters.push(tile === "QU" ? "Qu" : tile);
        }
        return letters.join(" · ");
    }

    // Toybox: the panel is the page with cards on it, not a strip of its own (the cards carry the outlines).

    // Clear and Done: one pair, in the zone's column of a wide panel and at the foot of a tall one.
    Component {
        id: actionBar
        ActionBar {
            tier: "child"
            secondaryLabel: qsTr("Clear")
            secondaryEnabled: root.solo.path.length > 0 || root.statusText.length > 0
            primaryLabel: qsTr("Done")
            primaryIcon: "done"
            primaryEnabled: root.solo.path.length > 0
            onSecondaryClicked: root.solo.clearPath()
            onPrimaryClicked: root.solo.done()
        }
    }

    // The foot is painted over the scrolling content, on the page's own colour, so nothing that
    // scrolls shows through between or under its buttons.
    Rectangle {
        z: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: foot.height + root.vMargin + root.gap
        visible: foot.visible
        color: Theme.page
    }

    // The tall layout's foot.
    Item {
        id: foot
        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.margin
        anchors.rightMargin: root.margin
        anchors.bottomMargin: root.vMargin
        visible: root.footed && root.actionsShown
        height: visible && footLoader.item ? (footLoader.item as Item).implicitHeight : 0

        Loader {
            id: footLoader
            anchors.left: parent.left
            anchors.right: parent.right
            active: root.footed
            sourceComponent: actionBar
        }
    }

    Flickable {
        id: viewport
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: foot.visible ? foot.top : parent.bottom
        anchors.bottomMargin: foot.visible && !root.tight ? Theme.space1 : 0
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        // Over the foot the content stops at the cards' own shadow; otherwise a margin's worth.
        readonly property real bottomPad: foot.visible ? root.gap : root.vMargin
        contentHeight: Math.max(layout.implicitHeight + root.vMargin + bottomPad, height)

        GridLayout {
            id: layout
            x: root.margin
            y: root.vMargin
            width: viewport.width - 2 * root.margin
            // The room the viewport gives it, or what it needs if that is more.
            height: Math.max(implicitHeight, viewport.height - root.vMargin - viewport.bottomPad)
            columns: root.twoColumns ? 2 : 1
            rowSpacing: root.gap
            columnSpacing: root.gap

            // --- words I found -------------------------------------------------
            Rectangle {
                id: wordsCard
                objectName: "wordsCard"
                // Under the word in hand in a tall panel, over it in a wide one, beside it in two columns.
                Layout.row: root.wide ? 0 : 1
                Layout.rowSpan: root.twoColumns ? 2 : 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Never a sliver: the heading, its counts and two whole rows.
                Layout.minimumHeight: wordsColumn.implicitHeight + 2 * Theme.space3
                Layout.preferredHeight: wordsColumn.implicitHeight + 2 * Theme.space3
                radius: Theme.radiusPanel
                color: Theme.card
                border.width: Theme.outlinePanel
                border.color: Theme.line

                Rectangle {
                    z: -1
                    y: Theme.shadowLarge
                    width: parent.width
                    height: parent.height
                    radius: parent.radius
                    color: Theme.shadow
                }

                Accessible.role: Accessible.Grouping
                Accessible.name: SoloWording.wordsFound

                ColumnLayout {
                    id: wordsColumn
                    anchors.fill: parent
                    anchors.margins: Theme.space3
                    spacing: Theme.space1

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space2

                        Label {
                            Layout.fillWidth: true
                            role: "heading"
                            text: SoloWording.wordsFound
                            wrapMode: Text.WordWrap
                        }
                        // A tall panel has no height to spare for a line of its own: the counts stand
                        // beside the heading, one over the other.
                        Column {
                            visible: root.tight
                            Layout.alignment: Qt.AlignVCenter
                            Label {
                                role: "caption"
                                text: root.wordCount
                            }
                            Label {
                                role: "caption"
                                color: Theme.goldInk
                                text: root.pointCount
                            }
                        }
                        Label {
                            visible: root.clockShown
                            role: "numeral"
                            text: root.solo.clockText
                            color: root.solo.clockMode === "down" && root.solo.clockSeconds <= 30
                                   ? Theme.goldInk : Theme.ink
                            Accessible.role: Accessible.StaticText
                            Accessible.name: SoloWording.clockName(root.solo.clockMode, root.solo.clockText)
                        }
                    }
                    // Side by side, or one under the other when the card is too narrow for both (big text).
                    Flow {
                        visible: !root.tight
                        Layout.fillWidth: true
                        Layout.preferredHeight: childrenRect.height
                        spacing: Theme.space3

                        Label {
                            role: "caption"
                            text: root.wordCount
                        }
                        Label {
                            role: "caption"
                            color: Theme.goldInk
                            text: root.pointCount
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: Theme.space1
                        Layout.preferredHeight: Theme.outlineControl
                        color: Theme.line
                    }

                    // The words, as chips that wrap: each is a jewel and its word, as wide as it needs,
                    // so a narrow panel shows two or three at once where a row to a word showed one. The
                    // list is a whole number of chip rows tall, so a row is never cut under the heading,
                    // and a thin handle says there are more when there are.
                    Item {
                        id: listArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: root.leastRows * root.chipPitch - Theme.space1
                        Layout.preferredHeight: root.leastRows * root.chipPitch - Theme.space1

                        WordChips {
                            id: words
                            objectName: "wordsList"
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.rightMargin: Theme.space3
                            // Whole rows of chips, at least one: n rows and the gaps between them.
                            height: Math.max(1, Math.floor((listArea.height + Theme.space1) / root.chipPitch))
                                    * root.chipPitch - Theme.space1
                            model: root.solo.found
                            chipName: word => SoloWording.found(word)
                            // The points are said in the wide card, where there is room; a tall panel has
                            // them in its count.
                            showPoints: !root.tight
                            Accessible.name: SoloWording.wordsFound
                        }

                        ScrollCue {
                            view: words
                            anchors.top: words.top
                            anchors.bottom: words.bottom
                            anchors.right: parent.right
                        }

                        // Nothing found yet: where the words will go, in the list's
                        // own place (so the word in hand never lands on it).
                        Column {
                            visible: root.solo.found.length === 0
                            anchors.centerIn: parent
                            spacing: Theme.space2
                            Jewel {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: Theme.jewelMega
                                height: Theme.jewelMega
                                tier: "power"
                                color: Theme.jewelDead
                                Accessible.ignored: true
                            }
                            Label {
                                width: Math.min(implicitWidth, listArea.width - 2 * Theme.space2)
                                role: "caption"
                                learning: true
                                text: qsTr("Your words will land here.")
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }

            // --- the actions: the word in hand, the hint, Finish, Clear and Done ----
            Item {
                id: strip
                // Over the words in a tall panel, under them in a wide one, beside them in two columns.
                Layout.row: root.wide && !root.twoColumns ? 1 : 0
                Layout.column: root.twoColumns ? 1 : 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(root.stripHeight, handColumn.implicitHeight,
                                                 hintText.implicitHeight + 2 * Theme.space2)

                Rectangle {
                    visible: root.hintOnly
                    anchors.fill: parent
                    radius: Theme.radius
                    color: Theme.hintCard
                    border.width: Theme.hair
                    border.color: Theme.gold
                }

                Column {
                    id: handColumn
                    visible: root.inHand
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Label {
                        width: parent.width
                        role: "word"
                        font.weight: Font.DemiBold
                        text: root.statusText.length > 0 ? root.statusText : root.currentWord()
                        color: root.statusText.length > 0 && !root.statusWord
                               ? Theme.inkSecondary : Theme.ink
                        elide: Text.ElideRight
                        wrapMode: Text.NoWrap
                    }
                    Label {
                        width: parent.width
                        visible: root.statusDetail.length > 0 || root.solo.path.length > 0
                        role: "caption"
                        learning: true
                        text: root.statusDetail.length > 0 ? root.statusDetail : root.actionLine
                        wrapMode: Text.WordWrap
                        // One line where the panel is tight: its height is counted on that.
                        maximumLineCount: root.tight ? 1 : 2
                        elide: Text.ElideRight
                    }
                }

                Label {
                    id: hintText
                    visible: root.hintOnly
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.space3
                    anchors.verticalCenter: parent.verticalCenter
                    role: root.tight ? "caption" : "body"
                    learning: true
                    color: Theme.hintCardInk
                    text: root.hintLine
                    wrapMode: Text.WordWrap
                }
            }

            // The hint and Finish board: one over the other in a wide panel, side by side (and the
            // smaller 48 dp size) in a tall one, which has no height for two 64 dp rows.
            GridLayout {
                id: helpers
                Layout.row: root.twoColumns ? 1 : 2
                Layout.column: root.twoColumns ? 1 : 0
                Layout.fillWidth: true
                // A tall panel keeps the row's height whatever is shown, so nothing under it moves.
                Layout.minimumHeight: root.tight && !root.twoColumns ? Theme.targetMin : 0
                Layout.preferredHeight: root.tight && !root.twoColumns ? Theme.targetMin : implicitHeight
                visible: root.tight ? !root.twoColumns || root.helpShown || root.actionsShown
                                    : (root.helpShown || root.actionsShown)
                columns: root.tight && !root.twoColumns ? 2 : 1
                columnSpacing: root.gap
                rowSpacing: root.gap

                // A start for a word, a second letter, or the hint put away.
                AppButton {
                    visible: root.helpShown
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    size: root.tight ? "min" : "child"
                    variant: "secondary"
                    wrap: true
                    // The icon gives its room to the words in a tall panel's half-width button.
                    iconName: root.tight ? "" : "hint"
                    compact: root.tight
                    label: root.hintLabel
                    accessibleName: root.hintLabel
                    onClicked: root.solo.askHelp()
                }

                // The standard board holds tens to hundreds of dictionary words, so
                // its "all words found" ending is not a real ending for a child.
                // This is: the board is safely saved, and the summary shows what
                // was found.
                AppButton {
                    visible: root.actionsShown
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    size: root.tight ? "min" : "child"
                    variant: "secondary"
                    wrap: true
                    compact: root.tight
                    label: SoloWording.finishBoard
                    accessibleName: SoloWording.finishBoard
                    onClicked: root.finishRequested()
                }
            }

            // Clear clears a path; Done hands one in. A traced word hands itself
            // in when the finger lifts, so Done is for a word made by taps (or
            // the keyboard and a screen reader). In a tall panel they are the
            // foot of it instead (`foot`).
            Loader {
                visible: root.actionsShown && !root.footed
                Layout.row: 3
                Layout.fillWidth: true
                active: !root.footed
                sourceComponent: actionBar
            }
        }
    }
}
