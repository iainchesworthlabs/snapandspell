// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import SnapAndSpell

// The words found, as chips that wrap: each is a jewel and its word, as wide as it needs, so a narrow
// panel shows two or three at once where a row to a word showed one. A chip is a 48 dp stop for Tab
// and a press says the word. The list scrolls in whole rows, rests on one after a flick, keeps the
// newest find in view and brings a chip the keyboard reaches into view whole.
//
// The model is the game's list of finds: {word, tier, points}. The owner sets the height; give it a
// whole number of rows (n * chipPitch - the gap) and no row is cut.
Flickable {
    id: root

    property var model: []
    // What a screen reader says of a chip, from its word.
    property var chipName: word => word
    // The "+3" after a word: said where there is room, left out where the owner counts the points.
    property bool showPoints: false

    readonly property int count: root.model.length
    // One row of chips and the gap under it.
    readonly property real chipPitch: Theme.rowHeight + Theme.space1

    clip: true
    contentWidth: width
    contentHeight: chips.height
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick
    interactive: contentHeight > height + 0.5

    Accessible.role: Accessible.List

    // The latest find is in view, on the row it is on.
    function showEnd() {
        root.contentY = Math.max(0, root.contentHeight - root.height);
    }
    // A chip the keyboard reaches is scrolled into view, whole.
    function reveal(item) {
        if (item.y < root.contentY) {
            root.contentY = item.y;
        } else if (item.y + item.height > root.contentY + root.height) {
            root.contentY = item.y + item.height - root.height;
        }
    }
    // Rest on a whole row after a flick or a drag.
    function settle() {
        const row = Math.round(root.contentY / root.chipPitch);
        root.contentY = Math.max(0, Math.min(row * root.chipPitch, root.contentHeight - root.height));
    }
    onMovementEnded: root.settle()
    onCountChanged: Qt.callLater(root.showEnd)
    Component.onCompleted: Qt.callLater(root.showEnd)

    Flow {
        id: chips
        width: root.width
        spacing: Theme.space1

        Repeater {
            model: root.model

            delegate: Item {
                id: chip
                required property int index
                required property var modelData
                width: Math.min(chipRow.implicitWidth + 2 * Theme.space2, chips.width)
                height: Theme.rowHeight
                // A chip scrolled out of the list is not drawn: its jewel is a Shape, which the
                // list's clip does not always hold.
                visible: y + height > root.contentY && y < root.contentY + root.height

                activeFocusOnTab: true
                Accessible.role: Accessible.ListItem
                Accessible.name: root.chipName(chip.modelData.word)
                Accessible.focusable: true
                Accessible.onPressAction: chip.speak()

                function speak() {
                    Feedback.cue("speak", chip.modelData.word);
                }

                Keys.onPressed: event => {
                    const acts = event.key === Qt.Key_Space || event.key === Qt.Key_Return
                                 || event.key === Qt.Key_Enter;
                    if (acts) {
                        chip.speak();
                    }
                    event.accepted = acts;
                }
                onActiveFocusChanged: {
                    if (chip.activeFocus) {
                        root.reveal(chip);
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radiusSmall
                    color: mouse.pressed ? Theme.pressWash : (mouse.containsMouse ? Theme.hoverWash : Theme.surfaceDim)
                    Accessible.ignored: true
                }
                Row {
                    id: chipRow
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.space2
                    spacing: Theme.space2
                    Jewel {
                        id: jewel
                        anchors.verticalCenter: parent.verticalCenter
                        width: Theme.jewelMd
                        height: Theme.jewelMd
                        tier: chip.modelData.tier
                        colorIndex: chip.index
                        Accessible.ignored: true
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth,
                                        chips.width - 2 * Theme.space2 - jewel.width - chipRow.spacing
                                        - (points.visible ? points.implicitWidth + chipRow.spacing : 0))
                        role: "word"
                        decorative: true
                        text: chip.modelData.word
                        elide: Text.ElideRight
                    }
                    Label {
                        id: points
                        visible: root.showPoints
                        anchors.verticalCenter: parent.verticalCenter
                        role: "eyebrow"
                        decorative: true
                        font.capitalization: Font.MixedCase
                        color: Theme.goldInk
                        text: qsTr("+%1").arg(chip.modelData.points)
                    }
                }
                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: chip.speak()
                }
                FocusRing {
                    visible: chip.activeFocus
                    inner: Theme.radiusSmall
                }
            }
        }
    }
}
