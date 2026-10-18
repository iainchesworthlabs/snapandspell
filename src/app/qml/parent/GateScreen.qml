pragma ComponentBehavior: Bound
import QtQuick

// The parental gate (docs/design/dashboard.md, "The gate"): two fingers on two
// marked circles for two seconds. It keeps a child out of the settings and the
// data by accident; it is not security.
//
// The rules are the library's (wjs/parent_gate.hpp) and are reached through the
// controller; this screen only turns touches into those calls, draws the
// circles and the progress, and gives a person who cannot hold two circles the
// same screen through an accessibility action.
//
//   - A touch screen: two fingers.
//   - A mouse and a keyboard: the mouse on the left circle, and Space for the
//     right one. A child with a mouse is no more likely to hold both than a
//     child with two fingers.
//   - A screen reader: activating the left circle opens it, which is the one
//     route that needs no hold. A tap on a circle is not that action.
FocusScope {
    id: root

    required property var controller

    // The parent area's direction, from the shell. The two circles are symmetric,
    // so the gate reads the same either way; its words and the progress bar follow
    // the reading direction.
    LayoutMirroring.enabled: ParentTheme.rightToLeft
    LayoutMirroring.childrenInherit: true

    // The ids this screen gives the keyboard's stand-in for a second finger; the
    // system's own touch ids are not negative.
    readonly property int spaceTouch: -2

    focus: true

    // The time is the controller's: this screen reads no clock of its own.
    function now() {
        return root.controller.now();
    }

    function releaseKey() {
        root.controller.gateTouchUp(root.spaceTouch, root.now());
    }

    onActiveFocusChanged: {
        if (!activeFocus) {
            root.releaseKey();
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat && root.controller.gateSpots.length === 2) {
            const second = root.controller.gateSpots[1];
            root.controller.gateTouchDown(root.spaceTouch, second.x, second.y, root.now());
            event.accepted = true;
        }
    }
    Keys.onReleased: event => {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
            root.releaseKey();
            event.accepted = true;
        }
    }

    Column {
        id: words
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: ParentTheme.space6
        width: Math.min(parent.width - 2 * ParentTheme.space4, ParentTheme.contentWidth)
        spacing: ParentTheme.space2

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: ParentWording.gateTitle
            font.pixelSize: ParentTheme.sizeTitle
            font.bold: true
            color: ParentTheme.ink
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.Heading
            Accessible.name: text
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: ParentWording.gateInstruction
            font.pixelSize: ParentTheme.sizeLead
            color: ParentTheme.ink
            wrapMode: Text.WordWrap
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: ParentWording.gateKeys
            font.pixelSize: ParentTheme.sizeSmall
            color: ParentTheme.inkSecondary
            wrapMode: Text.WordWrap
        }
    }

    // Where the circles are drawn and where touches are read: the same item, so
    // that the library's coordinates are the screen's.
    Item {
        id: field
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: words.bottom
        anchors.topMargin: ParentTheme.space4

        onWidthChanged: root.layout()
        onHeightChanged: root.layout()
        Component.onCompleted: root.layout()

        MultiPointTouchArea {
            anchors.fill: parent
            minimumTouchPoints: 1
            maximumTouchPoints: 32

            onPressed: touchPoints => {
                for (const point of touchPoints) {
                    root.controller.gateTouchDown(point.pointId, point.x, point.y, root.now());
                }
            }
            onUpdated: touchPoints => {
                for (const point of touchPoints) {
                    root.controller.gateTouchMove(point.pointId, point.x, point.y, root.now());
                }
            }
            onReleased: touchPoints => {
                for (const point of touchPoints) {
                    root.controller.gateTouchUp(point.pointId, root.now());
                }
            }
            onCanceled: touchPoints => {
                for (const point of touchPoints) {
                    root.controller.gateTouchUp(point.pointId, root.now());
                }
            }
        }

        Repeater {
            model: root.controller.gateSpots

            Item {
                id: spot
                required property int index
                required property var modelData

                x: modelData.x - modelData.r
                y: modelData.y - modelData.r
                width: 2 * modelData.r
                height: 2 * modelData.r

                // The circle, and what has been held so far as a disc that grows
                // from its centre: the progress is a shape and not a colour.
                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: ParentTheme.card
                    border.width: ParentTheme.rule * 2
                    border.color: ParentTheme.ink
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - 2 * ParentTheme.space2
                    height: width
                    radius: width / 2
                    color: ParentTheme.buttonFill
                    scale: root.controller.gateProgress
                    visible: root.controller.gateProgress > 0
                }
                Rectangle {
                    // A small mark in the middle, so that the circle is a place to put a finger.
                    anchors.centerIn: parent
                    width: ParentTheme.space4
                    height: width
                    radius: width / 2
                    color: root.controller.gateProgress > 0.5 ? ParentTheme.buttonInk : ParentTheme.ink
                }

                // The route for a person who cannot hold two circles: only the
                // first circle carries it, and only the assistive action runs it.
                Accessible.role: Accessible.Button
                Accessible.name: spot.index === 0 ? ParentWording.gateSpot : ""
                Accessible.ignored: spot.index !== 0
                Accessible.onPressAction: {
                    if (spot.index === 0) {
                        root.controller.unlockForAssistiveTechnology();
                    }
                }
            }
        }
    }

    // How far the hold has got, as a bar as well: for a person watching the
    // circles from a distance.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: ParentTheme.space6
        width: Math.min(parent.width - 2 * ParentTheme.space4, 360)
        height: ParentTheme.space2
        radius: height / 2
        color: ParentTheme.hoverWash
        visible: root.controller.gateHolding

        Rectangle {
            width: parent.width * root.controller.gateProgress
            height: parent.height
            radius: height / 2
            color: ParentTheme.buttonFill
        }
    }

    // The fill is drawn from the clock while both circles are held; there is no
    // timer in the game, and this runs only for as long as a parent holds.
    FrameAnimation {
        running: root.controller.gateHolding
        onTriggered: root.controller.gateTick(root.now())
    }

    function layout() {
        if (field.width > 0 && field.height > 0) {
            root.controller.layoutGate(field.width, field.height, ParentTheme.targetMin);
        }
    }
}
