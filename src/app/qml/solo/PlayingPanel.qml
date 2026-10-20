pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// The panel beside the flat board: the words found (with the word being made),
// one quiet hint, and the Clear and Done actions at the foot of the column.
// Every card has a title and an empty state; no card is a blank rectangle. The
// panel is one full-height column, so the actions at its foot sit at the bottom
// of the window and the column never moves when a path starts.
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
    // The hint's place is steady now that Done owns the actions' row: `hintShown`
    // is kept as the panel's own statement of whether the hint card is present,
    // for the screen's tests.
    readonly property bool hintShown: helpShown
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

    // The clock the parent set for the board, shown with the actions.
    readonly property bool clockShown: solo.clockMode !== "off"
    readonly property bool actionsShown: solo.state === "playing" || solo.state === "word-found"
    readonly property string actionLine: actionsShown && solo.path.length > 0
                                         ? qsTr("Tap the last die again, or press Done")
                                         : ""

    function currentWord() {
        var letters = [];
        for (var i = 0; i < solo.path.length; ++i) {
            const tile = solo.tiles[solo.path[i]];
            letters.push(tile === "QU" ? "Qu" : tile);
        }
        return letters.join(" · ");
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.panel
    }
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Theme.hair
        color: Theme.line
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.space4
        spacing: Theme.space3

        // --- words I found ---------------------------------------------------
        // The tray, and the word being made (or what just happened) under its
        // header: the found words and the word in hand share one card.
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.line
            clip: true

            Column {
                id: trayHeader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.space3
                spacing: 0

                Item {
                    width: parent.width
                    height: 40
                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        text: qsTr("Words I found")
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.headingSize
                        font.weight: Font.DemiBold
                        color: Theme.ink
                    }
                    Column {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        spacing: 1
                        Text {
                            anchors.right: parent.right
                            text: root.solo.found.length === 1 ? qsTr("1 word")
                                                              : qsTr("%1 words").arg(root.solo.found.length)
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.monoSize
                            color: Theme.inkSecondary
                        }
                        Text {
                            anchors.right: parent.right
                            text: qsTr("%1 points").arg(root.solo.boardPoints)
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.monoSize
                            color: Theme.goldInk
                        }
                    }
                }
                Rectangle {
                    width: parent.width
                    height: Theme.hair
                    color: Theme.line
                }
            }

            // The word in hand: a found word is the big line and the praise is
            // the small one; a path being made shows its letters and the way to
            // hand them in. When there is neither, the tray is all words.
            Column {
                id: status
                visible: root.statusText.length > 0 || root.solo.path.length > 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: trayHeader.bottom
                anchors.margins: Theme.space3
                anchors.topMargin: Theme.space2
                spacing: 1

                Text {
                    width: parent.width
                    text: root.statusText.length > 0 ? root.statusText
                                                     : root.currentWord()
                    font.family: root.statusWord || root.solo.path.length > 0
                                 ? Theme.fontFamily : Theme.uiFont
                    font.pixelSize: root.statusWord || root.solo.path.length > 0
                                    ? Theme.wordSize : Theme.headingSize
                    font.weight: Font.DemiBold
                    color: root.statusText.length > 0 && !root.statusWord
                           ? Theme.inkSecondary : Theme.ink
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                }
                Text {
                    width: parent.width
                    visible: root.statusDetail.length > 0 || root.solo.path.length > 0
                    text: root.statusDetail.length > 0 ? root.statusDetail
                                                       : root.actionLine
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallSize
                    color: Theme.inkSecondary
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                }
            }

            ListView {
                id: words
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: status.visible ? status.bottom : trayHeader.bottom
                anchors.bottom: parent.bottom
                anchors.margins: Theme.space2
                anchors.topMargin: Theme.space2
                clip: true
                model: root.solo.found
                spacing: 2
                onCountChanged: positionViewAtEnd()

                delegate: Item {
                    id: row
                    required property int index
                    required property var modelData
                    width: words.width
                    height: 42

                    Accessible.role: Accessible.ListItem
                    Accessible.name: SoloWording.found(row.modelData.word)
                    Accessible.onPressAction: Feedback.cue("speak", row.modelData.word)

                    Jewel {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 26
                        height: 26
                        tier: row.modelData.tier
                        colorIndex: row.index
                    }
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 38
                        anchors.right: points.left
                        anchors.rightMargin: Theme.space2
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.word
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.wordSize
                        color: Theme.ink
                        elide: Text.ElideRight
                    }
                    Text {
                        id: points
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("+%1").arg(row.modelData.points)
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.monoSize
                        color: Theme.goldInk
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: Feedback.cue("speak", row.modelData.word)
                    }
                }
            }

            Column {
                visible: root.solo.found.length === 0
                anchors.centerIn: parent
                spacing: Theme.space2
                Jewel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 38
                    height: 38
                    tier: "power"
                    color: Theme.jewelDead
                }
                Text {
                    text: qsTr("Your words will land here.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                }
            }
        }

        // --- need a start? ----------------------------------------------------
        Rectangle {
            visible: root.helpShown
            Layout.fillWidth: true
            implicitHeight: hintColumn.implicitHeight + 2 * Theme.space3
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.gold

            Column {
                id: hintColumn
                anchors.fill: parent
                anchors.margins: Theme.space3
                spacing: Theme.space2

                Text {
                    text: qsTr("Need a start?")
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.labelSize
                    font.weight: Font.DemiBold
                    color: Theme.ink
                }
                AppButton {
                    width: parent.width
                    size: "child"
                    variant: "secondary"
                    iconName: "hint"
                    label: root.hintLabel
                    accessibleName: root.hintLabel
                    onClicked: root.solo.askHelp()
                }
                Text {
                    width: parent.width
                    visible: root.hintLevel > 0
                    text: root.hintHelp
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                    wrapMode: Text.WordWrap
                }
                // The answer to the last question: a find made from the lit
                // start puts the rings away, and this says whether the same
                // letters still begin more of the board's words.
                Text {
                    width: parent.width
                    visible: root.solo.hintResolved
                    text: root.solo.hintRemaining > 0
                          ? SoloWording.moreWordsWithStart(root.solo.hintRemaining)
                          : SoloWording.noMoreWordsWithStart
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        // --- finish this board ------------------------------------------------
        // The standard board holds tens to hundreds of dictionary words, so its
        // "all words found" ending is not a real ending for a child. This is:
        // the board is safely saved, and the summary shows what was found.
        Rectangle {
            visible: root.actionsShown
            Layout.fillWidth: true
            implicitHeight: 62
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.line

            AppButton {
                anchors.fill: parent
                anchors.margins: Theme.space1
                variant: "ghost"
                size: "min"
                iconName: "done"
                label: SoloWording.finishBoard
                accessibleName: SoloWording.finishBoard
                onClicked: root.finishRequested()
            }
        }

        // --- the actions: the clock, Clear, Done ---------------------------------
        // Clear clears a path; Done hands one in. A traced word hands itself in
        // when the finger lifts, so Done is for a word made by taps (or the
        // keyboard and a screen reader) - and the board's clock counts beside
        // them, at the bottom of the window.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 120
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.line

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.space3
                spacing: Theme.space3

                Text {
                    visible: root.clockShown
                    text: root.solo.clockText
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.headingSize
                    font.weight: Font.DemiBold
                    color: root.solo.clockMode === "down" && root.solo.clockSeconds <= 30
                           ? Theme.goldInk : Theme.ink
                    Accessible.name: SoloWording.clockName(root.solo.clockMode, root.solo.clockText)
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                }

                AppButton {
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 64
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.actionsShown
                    size: "child"
                    variant: "secondary"
                    label: qsTr("Clear")
                    accessibleName: qsTr("Clear the word")
                    enabled: root.solo.path.length > 0 || root.statusText.length > 0
                    onClicked: root.solo.clearPath()
                }
                AppButton {
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: Theme.targetPrimary
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.actionsShown
                    size: "primary"
                    variant: "primary"
                    iconName: "done"
                    label: qsTr("Done")
                    accessibleName: qsTr("Done")
                    enabled: root.solo.path.length > 0
                    onClicked: root.solo.done()
                }
            }
        }
    }
}
