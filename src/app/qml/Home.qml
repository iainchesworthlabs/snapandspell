pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// home: the primary way in is one large Play placard. A second, quieter card
// opens the grown-up area; every adult workflow — the real board, settings and
// progress — lives behind the gate there. The wordmark, the hero and the two
// cards say what the app is without asking a child to read a menu.
FocusScope {
    id: root

    signal soloRequested
    signal parentRequested

    readonly property bool wide: width >= 900
    // Home has one primary Play picture and one adult picture; the name is kept
    // for the shell's checks and for a future first-run layout.
    readonly property int pictures: 2

    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

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
        showWordmark: true
        showParent: true
        onParentRequested: root.parentRequested()
    }

    Flickable {
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight + 2 * Theme.space8
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: content
            width: Math.min(parent.width - 2 * Theme.space8, 1180)
            x: (parent.width - width) / 2
            y: Theme.space8
            spacing: Theme.space8

            Column {
                width: parent.width
                spacing: Theme.space2

                Text {
                    text: qsTr("A WORD-FINDING GAME FOR YOUNG READERS")
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.monoSize
                    font.letterSpacing: 1.4
                    color: Theme.accent
                }
                Text {
                    width: parent.width
                    text: qsTr("Shake. Find. Collect.")
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.displaySize
                    font.weight: Font.ExtraBold
                    font.letterSpacing: -1.8
                    color: Theme.ink
                    wrapMode: Text.WordWrap
                }
                Text {
                    width: Math.min(parent.width, 720)
                    text: qsTr("A calm word game for little readers. Shake the jewel box, find words, and collect the jewels.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.headingSize
                    color: Theme.inkSecondary
                    wrapMode: Text.WordWrap
                }
            }

            Grid {
                width: parent.width
                columns: root.wide ? 2 : 1
                spacing: Theme.space6

                ModeCard {
                    width: root.wide ? (parent.width - Theme.space6) * 0.58 : parent.width
                    height: root.wide ? 420 : 400
                    primary: true
                    accessibleName: HomeWording.soloPicture
                    title: qsTr("Play on my own")
                    subtitle: qsTr("Shake the box. Find words. Collect jewels.")
                    actionLabel: qsTr("Play")
                    onClicked: root.soloRequested()

                    JewelBox {
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.86, 430)
                        height: parent.height * 0.96
                        closed: false
                        tiles: ["S", "O", "S", "S", "A", "N", "N", "S", "L"]
                    }
                }

                ModeCard {
                    width: root.wide ? parent.width - (parent.width - Theme.space6) * 0.58 - Theme.space6 : parent.width
                    height: root.wide ? 420 : 330
                    grownUp: true
                    accessibleName: HomeWording.familyPicture
                    eyebrow: qsTr("FOR GROWN-UPS")
                    title: qsTr("Play with a real board")
                    subtitle: qsTr("Use the camera, or type the board. A grown-up sets this up.")
                    actionLabel: qsTr("Continue")
                    onClicked: root.parentRequested()

                    Item {
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.70, 310)
                        height: parent.height * 0.78

                        // The stand.
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            width: parent.width * 0.46
                            height: 8
                            radius: 4
                            color: Theme.ink
                        }
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 6
                            width: 10
                            height: parent.height * 0.18
                            color: Theme.ink
                        }

                        // The tablet.
                        Rectangle {
                            id: tablet
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            width: parent.width * 0.78
                            height: width * 0.66
                            radius: 12
                            color: Theme.ink
                            border.width: 2
                            border.color: Theme.line

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 8
                                radius: 8
                                color: Theme.boardArea
                                border.width: 1
                                border.color: Theme.dieEdge

                                Grid {
                                    anchors.centerIn: parent
                                    columns: 4
                                    spacing: 3
                                    Repeater {
                                        model: 16
                                        Rectangle {
                                            width: (tablet.width * 0.44 - 9) / 4
                                            height: width
                                            radius: 2
                                            color: Theme.die
                                            border.width: 1
                                            border.color: Theme.dieEdge
                                        }
                                    }
                                }

                                // The camera looking at the board.
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 8
                                    width: 13
                                    height: 13
                                    radius: 7
                                    color: Theme.accent
                                    border.width: 3
                                    border.color: Theme.ink
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: Theme.hair
                color: Theme.line
            }

            Row {
                width: parent.width
                spacing: Theme.space4

                Text {
                    text: qsTr("Word Jewels Solver")
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.smallSize
                    color: Theme.inkSecondary
                }
                Item { width: Math.max(0, parent.width - 320); height: 1 }
                Text {
                    text: qsTr("Everything stays on this device.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallSize
                    color: Theme.inkSecondary
                }
            }
        }
    }

}
