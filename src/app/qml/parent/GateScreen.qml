pragma ComponentBehavior: Bound
import QtQuick

// The parental gate: one large press-and-hold target, not two far-apart
// circles. A parent should not have to discover that a mouse on one circle and
// Space on the keyboard have to coincide; the whole card explains the one
// gesture, and a keyboard user holds Space or Enter.
//
// The gate's own logic is unchanged: two touches, one in each hidden spot, for
// two seconds. The button presses both spots for the person, so the hand and
// the keyboard do the same thing.
FocusScope {
    id: root

    required property var controller

    LayoutMirroring.enabled: ParentTheme.rightToLeft
    LayoutMirroring.childrenInherit: true

    // The two virtual touches the one button puts down. Ids outside the range a
    // real touch would use, so they cannot collide with a palm.
    readonly property int firstTouch: 9001
    readonly property int secondTouch: 9002
    readonly property bool holding: root.controller.gateHolding

    focus: true

    function now() {
        return root.controller.now();
    }

    function beginHold() {
        if (root.controller.gateSpots.length !== 2 || root.holding) {
            return;
        }
        const first = root.controller.gateSpots[0];
        const second = root.controller.gateSpots[1];
        root.controller.gateTouchDown(root.firstTouch, first.x, first.y, root.now());
        root.controller.gateTouchDown(root.secondTouch, second.x, second.y, root.now());
    }

    function endHold() {
        root.controller.gateTouchUp(root.firstTouch, root.now());
        root.controller.gateTouchUp(root.secondTouch, root.now());
    }

    onActiveFocusChanged: {
        if (!activeFocus) {
            root.endHold();
        }
    }

    Keys.onPressed: event => {
        if ((event.key === Qt.Key_Space || event.key === Qt.Key_Return
             || event.key === Qt.Key_Enter) && !event.isAutoRepeat) {
            root.beginHold();
            event.accepted = true;
        }
    }
    Keys.onReleased: event => {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return
                || event.key === Qt.Key_Enter) {
            root.endHold();
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: ParentTheme.page
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * ParentTheme.space8, 560)
        height: Math.min(parent.height - 2 * ParentTheme.space8, 440)
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
                text: ParentWording.gateWhy
                font.family: ParentTheme.fontFamily
                font.pixelSize: ParentTheme.sizeSmall
                color: ParentTheme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        // The one target. It is deliberately large: 96 dp high, the whole width
        // of the card less its margins.
        Rectangle {
            id: holdTarget
            objectName: "gateHoldTarget"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: words.bottom
            anchors.topMargin: ParentTheme.space6
            anchors.leftMargin: ParentTheme.space6
            anchors.rightMargin: ParentTheme.space6
            height: 96
            radius: ParentTheme.radius
            color: root.holding ? ParentTheme.accentSoft : ParentTheme.card
            border.width: root.holding ? 2 : 1
            border.color: root.holding ? ParentTheme.gold : ParentTheme.outline
            activeFocusOnTab: true

            Accessible.role: Accessible.Button
            Accessible.name: ParentWording.gateHoldButton
            Accessible.focusable: true
            Accessible.onPressAction: root.controller.unlockForAssistiveTechnology()

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    holdTarget.forceActiveFocus();
                    root.beginHold();
                }
                onReleased: root.endHold()
                onCanceled: root.endHold()
                onContainsPressChanged: {
                    if (!containsPress) {
                        root.endHold();
                    }
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: ParentTheme.space1
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: ParentWording.gateHoldButton
                    font.family: ParentTheme.uiFont
                    font.pixelSize: ParentTheme.sizeLead
                    font.weight: Font.DemiBold
                    color: ParentTheme.ink
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: ParentWording.gateHoldHint
                    font.family: ParentTheme.fontFamily
                    font.pixelSize: ParentTheme.sizeSmall
                    color: ParentTheme.inkSecondary
                }
            }
        }

        // The progress is a bar, not a ring around two hidden spots: it says
        // exactly how much longer the press has to last.
        Rectangle {
            id: progress
            anchors.left: holdTarget.left
            anchors.right: holdTarget.right
            anchors.top: holdTarget.bottom
            anchors.topMargin: ParentTheme.space3
            height: 8
            radius: 4
            color: ParentTheme.hoverWash

            Rectangle {
                width: parent.width * Math.min(1, root.controller.gateProgress)
                height: parent.height
                radius: parent.radius
                color: ParentTheme.buttonFill
            }
        }

        Text {
            anchors.left: holdTarget.left
            anchors.right: holdTarget.right
            anchors.top: progress.bottom
            anchors.topMargin: ParentTheme.space2
            horizontalAlignment: Text.AlignHCenter
            text: ParentWording.gateKeys
            font.family: ParentTheme.fontFamily
            font.pixelSize: ParentTheme.sizeSmall
            color: ParentTheme.inkSecondary
            wrapMode: Text.WordWrap
        }
    }

    // The gate's two spots are still laid out, because the same Gate answers
    // the virtual touches. The button covers the field, so the spots themselves
    // are never the thing a parent has to aim at.
    Item {
        id: field
        anchors.fill: parent
        visible: false

        onWidthChanged: root.layout()
        onHeightChanged: root.layout()
        Component.onCompleted: root.layout()
    }

    function layout() {
        if (field.width > 0 && field.height > 0) {
            root.controller.layoutGate(field.width, field.height, ParentTheme.targetMin);
        }
    }

    FrameAnimation {
        running: root.controller.gateHolding
        onTriggered: root.controller.gateTick(root.now())
    }

    Component.onDestruction: root.endHold()
}
