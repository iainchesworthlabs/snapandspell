pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// The panel beside the flat board: the words found, one quiet hint and the way
// to earlier boards. Every card has a title and an empty state; no card is a
// blank rectangle.
Item {
    id: root

    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

    required property var solo
    required property bool wide
    signal historyRequested

    readonly property bool helpShown: solo.helpAllowed
    readonly property bool pending: solo.path.length > 0
    // The hint's place no longer changes when a path waits; the action bar owns
    // Done now. `hintShown` is kept as the panel's own statement of whether the
    // hint card is present, for the screen's tests.
    readonly property bool hintShown: helpShown
    readonly property bool mirrored: Theme.rightToLeft
    readonly property string hintLetter: solo.helpTile >= 0 && solo.helpTile < solo.tiles.length
                                          ? solo.tiles[solo.helpTile] : ""

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
                    height: 26
                    Text {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Words I found")
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.headingSize
                        font.weight: Font.DemiBold
                        color: Theme.ink
                    }
                    Text {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.solo.found.length === 1 ? qsTr("1 word")
                                                          : qsTr("%1 words").arg(root.solo.found.length)
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.monoSize
                        color: Theme.inkSecondary
                    }
                }
                Rectangle {
                    width: parent.width
                    height: Theme.hair
                    color: Theme.line
                }
            }

            ListView {
                id: words
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: trayHeader.bottom
                anchors.bottom: parent.bottom
                anchors.margins: Theme.space2
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
                        anchors.verticalCenter: parent.verticalCenter
                        text: row.modelData.word
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.wordSize
                        color: Theme.ink
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
                    label: SoloWording.showStart
                    accessibleName: SoloWording.showStart
                    onClicked: root.solo.askHelp()
                }
                Text {
                    visible: root.hintLetter.length > 0
                    text: qsTr("Start here: %1").arg(root.hintLetter)
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    color: Theme.inkSecondary
                }
            }
        }

        // --- earlier boards ---------------------------------------------------
        Rectangle {
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
                iconName: "history"
                label: SoloWording.earlierBoards
                accessibleName: SoloWording.earlierBoards
                onClicked: root.historyRequested()
            }
        }
    }
}
