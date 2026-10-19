import QtQuick
import QtQuick.Layouts

// What a parent sees (docs/design/dashboard.md, "What the dashboard shows"):
// this week, the sounds, the vault and the best, computed on the device when
// this screen is opened. Observations and never assessments: no time on screen,
// no streak, no comparison, no grade.
Flickable {
    id: root

    required property var controller

    signal scanRequested
    signal playRequested

    // The result of the parent's one retry of a failed save, if they pressed it.
    property bool retryTried: false
    property bool retrySucceeded: false

    // The parent area's direction, from the shell: the rows and their counts
    // follow it (the numbers themselves do not).
    LayoutMirroring.enabled: ParentTheme.rightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property var report: controller.dashboard
    readonly property var week: report.week
    readonly property bool anything: week.played || report.sounds.length > 0 || report.vault.length > 0 || report.best.length > 0

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * ParentTheme.space6
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    ColumnLayout {
        id: column
        x: Math.max(ParentTheme.space4, (root.width - width) / 2)
        y: ParentTheme.space6
        width: Math.min(root.width - 2 * ParentTheme.space4, ParentTheme.contentWidth)
        spacing: ParentTheme.space8

        Text {
            Layout.fillWidth: true
            text: ParentWording.privacyLine
            font.family: ParentTheme.monoFont
            font.pixelSize: ParentTheme.sizeSmall
            color: ParentTheme.inkSecondary
        }

        // The adult way into the family game. The scanner is a parent's
        // workflow, so it lives behind this gate rather than on the child's
        // home.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: welcome.implicitHeight + 2 * ParentTheme.space4
            radius: ParentTheme.radius
            color: ParentTheme.card
            border.width: 1
            border.color: ParentTheme.gold

            ColumnLayout {
                id: welcome
                anchors.fill: parent
                anchors.margins: ParentTheme.space4
                spacing: ParentTheme.space2

                Text {
                    Layout.fillWidth: true
                    text: ParentWording.welcomeTitle
                    font.family: ParentTheme.uiFont
                    font.pixelSize: ParentTheme.sizeLead
                    font.weight: Font.ExtraBold
                    color: ParentTheme.ink
                }
                Text {
                    Layout.fillWidth: true
                    text: ParentWording.welcomeBody
                    font.family: ParentTheme.uiFont
                    font.pixelSize: ParentTheme.sizeBody
                    color: ParentTheme.inkSecondary
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: ParentTheme.space2
                    PButton {
                        text: ParentWording.setUpBoard
                        primary: true
                        onClicked: root.scanRequested()
                    }
                    PButton {
                        text: ParentWording.backToPlay
                        onClicked: root.playRequested()
                    }
                }
            }
        }

        // A note about the saved progress, for the parent only.
        Rectangle {
            Layout.fillWidth: true
            visible: root.controller.storeNote.length > 0 || root.retryTried
            implicitHeight: noteColumn.implicitHeight + 2 * ParentTheme.space3
            radius: ParentTheme.radius
            color: ParentTheme.note
            border.width: ParentTheme.hair * 2
            border.color: ParentTheme.outline

            ColumnLayout {
                id: noteColumn
                anchors.fill: parent
                anchors.margins: ParentTheme.space3
                spacing: ParentTheme.space2

                Text {
                    id: noteText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pixelSize: ParentTheme.sizeBody
                    color: ParentTheme.noteInk
                    text: {
                        switch (root.controller.storeNote) {
                        case "unreadable":
                            return ParentWording.noteUnreadable;
                        case "newer":
                            return ParentWording.noteNewer;
                        case "unsaved":
                            return ParentWording.noteUnsaved;
                        default:
                            return "";
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.retryTried
                    wrapMode: Text.WordWrap
                    font.pixelSize: ParentTheme.sizeSmall
                    color: ParentTheme.noteInk
                    text: root.retrySucceeded ? ParentWording.saved : ParentWording.saveStillNotSaved
                }
                PButton {
                    visible: root.controller.storeNote === "unsaved"
                    text: ParentWording.retry
                    onClicked: {
                        root.retrySucceeded = root.controller.retrySave();
                        root.retryTried = true;
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.anything
            text: ParentWording.nothingYet
            font.pixelSize: ParentTheme.sizeLead
            color: ParentTheme.inkSecondary
            wrapMode: Text.WordWrap
        }

        // --- this week --------------------------------------------------------
        // The sentence begins "This week:", so the block has no heading of its own.
        Section {
            Layout.fillWidth: true
            visible: root.week.played

            Text {
                Layout.fillWidth: true
                text: ParentWording.weekSentence(root.week.boardsCleared, root.week.stars, root.week.newWords)
                font.pixelSize: ParentTheme.sizeBody
                color: ParentTheme.ink
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: root.week.longestWord.length > 0
                text: ParentWording.longestWord(root.week.longestWord)
                font.pixelSize: ParentTheme.sizeBody
                color: ParentTheme.ink
                wrapMode: Text.WordWrap
            }
        }

        // --- the sounds ---------------------------------------------------------
        Section {
            Layout.fillWidth: true
            visible: root.report.sounds.length > 0
            title: ParentWording.soundsHeading

            Repeater {
                model: root.report.sounds

                Rectangle {
                    id: row
                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: Math.max(ParentTheme.targetMin + 2 * ParentTheme.space2, rowText.implicitHeight + 2 * ParentTheme.space3)
                    radius: ParentTheme.radius
                    color: ParentTheme.card
                    border.width: ParentTheme.hair * 2
                    border.color: ParentTheme.hoverWash

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: ParentTheme.space3
                        spacing: ParentTheme.space4

                        Text {
                            Layout.preferredWidth: ParentTheme.space12
                            text: row.modelData.letters
                            font.pixelSize: ParentTheme.sizeSound
                            font.bold: true
                            color: ParentTheme.ink
                            horizontalAlignment: Text.AlignHCenter
                            Accessible.ignored: true
                        }

                        ColumnLayout {
                            id: rowText
                            Layout.fillWidth: true
                            spacing: ParentTheme.space1

                            Text {
                                Layout.fillWidth: true
                                textFormat: Text.StyledText
                                wrapMode: Text.WordWrap
                                font.pixelSize: ParentTheme.sizeBody
                                color: ParentTheme.ink
                                text: row.modelData.boardsMet > 0
                                    ? ParentWording.soundSentence(row.modelData.letters, row.modelData.boardsMet, row.modelData.boardsFound)
                                    : ParentWording.notMetYet
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: row.modelData.wordsUnassisted > 0
                                wrapMode: Text.WordWrap
                                font.pixelSize: ParentTheme.sizeSmall
                                color: ParentTheme.inkSecondary
                                text: ParentWording.foundWithoutStartTile(row.modelData.wordsUnassisted)
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: row.modelData.oftenFound
                                wrapMode: Text.WordWrap
                                font.pixelSize: ParentTheme.sizeSmall
                                font.bold: true
                                color: ParentTheme.ink
                                text: ParentWording.oftenFound
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: row.modelData.fewWords
                                wrapMode: Text.WordWrap
                                font.pixelSize: ParentTheme.sizeSmall
                                color: ParentTheme.inkSecondary
                                text: ParentWording.fewWords
                            }
                        }
                    }
                }
            }
        }

        // --- the vault ------------------------------------------------------------
        Section {
            Layout.fillWidth: true
            visible: root.report.vault.length > 0
            title: ParentWording.vaultHeading

            Flow {
                Layout.fillWidth: true
                spacing: ParentTheme.space2

                Repeater {
                    model: root.report.vault

                    Rectangle {
                        id: sticker
                        required property var modelData

                        width: stickerText.implicitWidth + 2 * ParentTheme.space3
                        height: ParentTheme.space8 + ParentTheme.space1
                        radius: height / 2
                        color: ParentTheme.panel

                        Text {
                            id: stickerText
                            anchors.centerIn: parent
                            text: sticker.modelData.word
                            font.pixelSize: ParentTheme.sizeBody
                            color: ParentTheme.ink
                        }
                    }
                }
            }
        }

        // --- the best -----------------------------------------------------------------
        Section {
            Layout.fillWidth: true
            visible: root.report.best.length > 0
            title: ParentWording.bestHeading

            Repeater {
                model: root.report.best

                Text {
                    required property var modelData
                    Layout.fillWidth: true
                    text: ParentWording.bestLine(modelData.size, modelData.stars)
                    font.pixelSize: ParentTheme.sizeBody
                    color: ParentTheme.ink
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
