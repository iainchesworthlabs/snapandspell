pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// home: the primary way in is one large Play placard. A second, quieter card
// opens the grown-up area; every adult workflow — the real board, settings and
// progress — lives behind the gate there. The wordmark, the hero and the two
// cards say what the app is without asking a child to read a menu.
//
// The page is sized from the window it is given: at the 900 x 620 minimum the
// hero loses its lede and the cards lose height rather than sliding under the
// bottom edge. A short window therefore still shows both ways in and the
// privacy line; it never hides the primary action.
FocusScope {
    id: root

    required property var controller

    signal soloRequested
    signal parentRequested
    signal collectionRequested

    readonly property bool wide: width >= 900
    // Home has one primary Play picture and one adult picture; the name is kept
    // for the shell's checks and for a future first-run layout.
    readonly property int pictures: 2

    // The space below the app bar. Everything on the page is laid out in it.
    readonly property real room: Math.max(0, height - Theme.appBarHeight)
    readonly property bool short: room < 650
    readonly property real margin: short ? Theme.space4 : Theme.space8
    readonly property real gutter: short ? Theme.space4 : Theme.space6
    // The cards take what the hero and footer leave, rather than a fixed 420
    // that pushes the footer past the window at the minimum height.
    readonly property real cardHeight: Math.max(300, Math.min(420, room
                                                              - 2 * margin
                                                              - hero.implicitHeight
                                                              - 2 * gutter
                                                              - footer.implicitHeight - Theme.space2))

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
        // Always there: a child who has just started can open it and see the
        // empty shelf with the way to play, rather than waiting for a first find
        // before the collection exists.
        showJewels: true
        onParentRequested: root.parentRequested()
        onJewelsRequested: root.collectionRequested()
    }

    Flickable {
        id: page
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: Math.max(content.implicitHeight + 2 * root.margin, height)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        Column {
            id: content
            width: Math.min(page.width - 2 * root.margin, 1180)
            x: (page.width - width) / 2
            y: root.margin
            spacing: root.gutter

            Column {
                id: hero
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
                    font.pixelSize: root.short ? Theme.titleSize : Theme.displaySize
                    font.weight: Font.ExtraBold
                    font.letterSpacing: -1.8
                    color: Theme.ink
                    wrapMode: Text.WordWrap
                }
                Text {
                    // At the minimum height the two ways in matter more than a
                    // second sentence; it comes back as soon as there is room.
                    visible: !root.short
                    width: Math.min(parent.width, 720)
                    text: qsTr("A calm word game for little readers. Shake the jewel box, find words, and collect the jewels.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.headingSize
                    color: Theme.inkSecondary
                    wrapMode: Text.WordWrap
                }
            }

            Row {
                width: parent.width
                spacing: root.gutter
                visible: root.wide

                ModeCard {
                    width: (parent.width - root.gutter) * 0.56
                    height: root.cardHeight
                    primary: true
                    accessibleName: HomeWording.soloPicture
                    title: qsTr("Play on my own")
                    subtitle: qsTr("Shake the box. Find words. Collect jewels.")
                    actionLabel: qsTr("Play")
                    onClicked: root.soloRequested()

                    JewelBox {
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.9, 460)
                        height: Math.max(120, parent.height)
                        closed: false
                        tiles: ["S", "O", "S", "S", "A", "N", "N", "S", "L"]
                    }
                }

                ModeCard {
                    width: parent.width - (parent.width - root.gutter) * 0.56 - root.gutter
                    height: root.cardHeight
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
                        height: Math.min(parent.height * 0.92, 250)

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

            // Below 900 the two ways in stack; the page scrolls rather than
            // squeezing a card until its label is cut.
            Column {
                width: parent.width
                spacing: root.gutter
                visible: !root.wide

                ModeCard {
                    width: parent.width
                    height: 360
                    primary: true
                    accessibleName: HomeWording.soloPicture
                    title: qsTr("Play on my own")
                    subtitle: qsTr("Shake the box. Find words. Collect jewels.")
                    actionLabel: qsTr("Play")
                    onClicked: root.soloRequested()

                    JewelBox {
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.9, 460)
                        height: Math.max(120, parent.height)
                        closed: false
                        tiles: ["S", "O", "S", "S", "A", "N", "N", "S", "L"]
                    }
                }

                ModeCard {
                    width: parent.width
                    height: 320
                    grownUp: true
                    accessibleName: HomeWording.familyPicture
                    eyebrow: qsTr("FOR GROWN-UPS")
                    title: qsTr("Play with a real board")
                    subtitle: qsTr("Use the camera, or type the board. A grown-up sets this up.")
                    actionLabel: qsTr("Continue")
                    onClicked: root.parentRequested()

                    Item {
                        anchors.centerIn: parent
                        width: Math.min(parent.width * 0.55, 250)
                        height: Math.min(parent.height * 0.86, 210)

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
                        Rectangle {
                            id: smallTablet
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
                                            width: (smallTablet.width * 0.44 - 9) / 4
                                            height: width
                                            radius: 2
                                            color: Theme.die
                                            border.width: 1
                                            border.color: Theme.dieEdge
                                        }
                                    }
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

            // The footer used a fixed 320 px spacer, which pushed the privacy
            // line past the right edge on anything narrower than 1180. The
            // layout now keeps the two labels on their own edges and elides
            // only if the window is truly too narrow for both.
            RowLayout {
                id: footer
                width: parent.width
                spacing: Theme.space4

                Text {
                    text: qsTr("Word Jewels Solver")
                    font.family: Theme.monoFont
                    font.pixelSize: Theme.smallSize
                    color: Theme.inkSecondary
                    elide: Text.ElideRight
                    Layout.maximumWidth: parent.width * 0.42
                }
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                }
                Text {
                    text: qsTr("Everything stays on this device.")
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallSize
                    color: Theme.inkSecondary
                    horizontalAlignment: Text.AlignRight
                    elide: Text.ElideRight
                    Layout.maximumWidth: parent.width * 0.55
                }
            }
        }
    }
}
