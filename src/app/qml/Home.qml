pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// home: two large pictures, the box (Solo Play) and the tablet on a stand (the
// family game), and a small corner button for the parent. There is no text to read
// and no first-run carousel: the pictures are the instruction
// (docs/design/application.md, "Screens and navigation").
//
// A picture speaks its name when pressed and acts on release inside it; a slide
// off it cancels. A picture is shown only if pressing it does something, so the
// family game's waits for its screen (Navigation.scanAvailable) and so does the
// parent's button (Navigation.parentAvailable).
FocusScope {
    id: root

    signal soloRequested
    signal scanRequested
    signal parentRequested

    readonly property bool wide: width >= height
    readonly property int pictures: Navigation.scanAvailable ? 2 : 1
    readonly property real gap: Theme.space8
    readonly property real margin: Theme.space6
    readonly property real barOffset: Theme.barHeight
    // The side of each picture: as large as the window allows, two in a row when
    // wide and in a column when tall.
    readonly property real side: Math.max(Theme.targetPrimary, Math.min(
        wide ? (width - 2 * margin - (pictures - 1) * gap) / pictures : width - 2 * margin,
        wide ? height - barOffset - 2 * margin : (height - barOffset - 2 * margin - (pictures - 1) * gap) / pictures,
        Theme.pictureMax))

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    Grid {
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.barOffset + (root.height - root.barOffset - height) / 2
        columns: root.wide ? root.pictures : 1
        spacing: root.gap

        // The box: a dark tray with its dice, lid beside it.
        AppButton {
            id: soloPicture
            width: root.side
            height: root.side
            variant: "secondary"
            accessibleName: qsTr("Play on my own")
            speaks: "solo"
            focus: true
            onClicked: root.soloRequested()

            Rectangle {
                id: tray
                anchors.centerIn: parent
                width: parent.width * 0.74
                height: width
                color: Theme.tray
                border.width: Theme.rule
                border.color: Theme.trayEdge

                Grid {
                    anchors.fill: parent
                    anchors.margins: tray.width * 0.07
                    columns: 3
                    spacing: tray.width * 0.05
                    Repeater {
                        model: 9
                        Rectangle {
                            width: (tray.width * 0.86 - 2 * tray.width * 0.05) / 3
                            height: width
                            color: Theme.die
                        }
                    }
                }
            }
        }

        // The tablet on a stand: the family game.
        AppButton {
            id: scanPicture
            visible: Navigation.scanAvailable
            width: root.side
            height: root.side
            variant: "secondary"
            accessibleName: qsTr("Play with the family")
            speaks: "family"
            onClicked: root.scanRequested()

            // The stand.
            Rectangle {
                width: parent.width * 0.06
                height: parent.height * 0.18
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: parent.height * 0.1
                color: Theme.ink
            }
            Rectangle {
                width: parent.width * 0.4
                height: parent.height * 0.05
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: parent.height * 0.1
                color: Theme.ink
            }
            // The tablet.
            Rectangle {
                id: tablet
                width: parent.width * 0.74
                height: parent.height * 0.5
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.height * 0.14
                color: Theme.ink

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: tablet.width * 0.05
                    color: Theme.boardArea

                    Grid {
                        anchors.centerIn: parent
                        columns: 3
                        spacing: tablet.height * 0.03
                        Repeater {
                            model: 9
                            Rectangle {
                                width: tablet.height * 0.2
                                height: width
                                color: Theme.die
                                border.width: Theme.hair
                                border.color: Theme.dieEdge
                            }
                        }
                    }
                }
            }
        }
    }

    // The way to the grown-ups' area is a small button in the corner.
    AppButton {
        visible: Navigation.parentAvailable
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.space2
        iconName: "parent"
        accessibleName: qsTr("For grown-ups")
        onClicked: root.parentRequested()
    }
}
