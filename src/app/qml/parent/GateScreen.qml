pragma ComponentBehavior: Bound
import QtQuick

// The parental gate: a compact card, two marked circles and a deliberate hold.
// It keeps a child out of the adult area by accident; it is not security. Touch
// and mouse+Space are joined by a keyboard-only held Enter, so a parent who
// cannot use a pointer can still open it.
FocusScope {
    id: root

    required property var controller

    LayoutMirroring.enabled: ParentTheme.rightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property int spaceTouch: -2
    property bool enterHeld: false

    focus: true

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
        if (event.key === Qt.Key_Space && !event.isAutoRepeat
                && root.controller.gateSpots.length === 2) {
            const second = root.controller.gateSpots[1];
            root.controller.gateTouchDown(root.spaceTouch, second.x, second.y, root.now());
            event.accepted = true;
        } else if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                   && !event.isAutoRepeat) {
            root.enterHeld = true;
            enterHold.restart();
            event.accepted = true;
        }
    }
    Keys.onReleased: event => {
        if (event.key === Qt.Key_Space && !event.isAutoRepeat) {
            root.releaseKey();
            event.accepted = true;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.enterHeld = false;
            enterHold.stop();
            event.accepted = true;
        }
    }

    Timer {
        id: enterHold
        interval: 2000
        onTriggered: {
            if (root.enterHeld) {
                root.controller.unlockForAssistiveTechnology();
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: ParentTheme.page
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * ParentTheme.space8, 600)
        height: Math.min(parent.height - 2 * ParentTheme.space8, 480)
        radius: ParentTheme.radius
        color: ParentTheme.surface
        border.width: 1
        border.color: ParentTheme.line

        Column {
            id: words
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: ParentTheme.space6
            spacing: ParentTheme.space2

            Text {
                width: parent.width
                text: ParentWording.gateTitle
                font.family: ParentTheme.uiFont
                font.pixelSize: ParentTheme.sizeTitle
                font.weight: Font.ExtraBold
                color: ParentTheme.ink
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.Heading
                Accessible.name: text
            }
            Text {
                width: parent.width
                text: ParentWording.gateInstruction
                font.family: ParentTheme.uiFont
                font.pixelSize: ParentTheme.sizeLead
                color: ParentTheme.ink
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                text: ParentWording.gateKeys
                font.family: ParentTheme.fontFamily
                font.pixelSize: ParentTheme.sizeSmall
                color: ParentTheme.inkSecondary
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                text: ParentWording.gateEnterHold
                font.family: ParentTheme.fontFamily
                font.pixelSize: ParentTheme.sizeSmall
                color: ParentTheme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        Item {
            id: field
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: words.bottom
            anchors.bottom: progress.top
            anchors.margins: ParentTheme.space6

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

                    Rectangle {
                        anchors.fill: parent
                        radius: width / 2
                        color: root.controller.gateProgress > 0 ? ParentTheme.accentSoft : ParentTheme.card
                        border.width: 2
                        border.color: root.controller.gateProgress > 0 ? ParentTheme.gold : ParentTheme.outline
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: (parent.width - 2 * ParentTheme.space2) * Math.min(1, root.controller.gateProgress)
                        height: width
                        radius: width / 2
                        color: ParentTheme.buttonFill
                        visible: root.controller.gateProgress > 0
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: 12
                        height: 12
                        radius: 6
                        color: ParentTheme.gold
                        visible: root.controller.gateProgress <= 0
                    }

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

        Rectangle {
            id: progress
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: ParentTheme.space6
            height: 6
            radius: 3
            color: ParentTheme.hoverWash
            visible: root.controller.gateHolding

            Rectangle {
                width: parent.width * root.controller.gateProgress
                height: parent.height
                radius: parent.radius
                color: ParentTheme.buttonFill
            }
        }
    }

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
