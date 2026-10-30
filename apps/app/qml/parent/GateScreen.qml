// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

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

    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

    // The two virtual touches the one button puts down. Ids outside the range a
    // real touch would use, so they cannot collide with a palm.
    readonly property int firstTouch: 9001
    readonly property int secondTouch: 9002
    readonly property bool holding: root.controller.gateHolding
    // A window too low for the whole card (a phone on its side): it keeps the title, the instruction, the
    // button and the steps, and the two sentences that explain them go.
    readonly property bool short: Theme.isShort(root.height)

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
        color: Theme.page
    }

    // The card sits in the middle of the page and takes its height from what is
    // on it (it was a fixed 560 x 440 with a dead third below its last line); a
    // window too short for it, or a large text size, scrolls it.
    Flickable {
        id: view
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, card.height + 2 * Theme.space8)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick
        interactive: contentHeight > height

        // A Toybox panel: the cream card, a 4 px ink outline and a hard drop.
        PixelBox {
            id: card
            width: Math.min(view.width - 2 * Theme.space8, Theme.modalMax)
            height: words.implicitHeight + 2 * (root.short ? Theme.space4 : Theme.space6)
            x: (view.width - width) / 2
            y: Math.max(Theme.space8, (view.contentHeight - height) / 2)
            fill: Theme.card
            radius: Theme.radiusPanel
            shadowOffset: Theme.shadowLarge

            ColumnLayout {
                id: words
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: root.short ? Theme.space4 : Theme.space6
                spacing: Theme.space2

                Label {
                    Layout.fillWidth: true
                    role: "title"
                    text: ParentWording.gateTitle
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    role: "body"
                    size: Theme.headingSize
                    text: ParentWording.gateInstruction
                    wrapMode: Text.WordWrap
                }
                Label {
                    Layout.fillWidth: true
                    visible: !root.short
                    role: "caption"
                    text: ParentWording.gateWhy
                    wrapMode: Text.WordWrap
                }

                // The one target. It is deliberately large: 96 dp high, the whole width of the card less its
                // margins, the primary button's ink with its hard drop; it sinks onto the drop while it is held.
                PixelBox {
                    id: holdTarget
                    objectName: "gateHoldTarget"
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.space4
                    Layout.bottomMargin: Theme.shadowLarge
                    Layout.preferredHeight: Math.max(Theme.targetPrimary, holdWords.implicitHeight + 2 * Theme.space3)
                    fill: Theme.buttonFill
                    edge: Theme.buttonFill
                    edgeWidth: Theme.outlineControl
                    radius: Theme.radiusPanel
                    shadowOffset: Theme.shadowLarge
                    sunk: root.holding
                    activeFocusOnTab: true

                    readonly property bool showFocus: activeFocus && !pointerFocus
                    property bool pointerFocus: false

                    Accessible.role: Accessible.Button
                    Accessible.name: ParentWording.gateHoldButton
                    Accessible.focusable: true
                    Accessible.onPressAction: root.controller.unlockForAssistiveTechnology()

                    onActiveFocusChanged: {
                        if (!holdTarget.activeFocus) {
                            holdTarget.pointerFocus = false;
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onPressed: {
                            holdTarget.pointerFocus = true;
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

                    ColumnLayout {
                        id: holdWords
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: holdTarget.sunk ? holdTarget.shadowOffset : 0
                        width: parent.width - 2 * Theme.space3
                        spacing: Theme.space1
                        Label {
                            Layout.fillWidth: true
                            role: "heading"
                            color: Theme.buttonInk
                            text: ParentWording.gateHoldButton
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            decorative: true
                        }
                        Label {
                            Layout.fillWidth: true
                            role: "caption"
                            color: Theme.buttonInk
                            text: ParentWording.gateHoldHint
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            decorative: true
                        }
                    }

                    FocusRing {
                        visible: holdTarget.showFocus
                        inner: holdTarget.radius
                    }
                }

                // The progress is a row of twenty steps, not a ring around two hidden spots: it says
                // exactly how much longer the press has to last, a step to each tenth of a second. An
                // empty step is outlined in the token that holds 3:1 against the card.
                RowLayout {
                    id: progress
                    objectName: "gateSteps"
                    readonly property int steps: 20
                    readonly property int filled: Math.floor(Math.min(1, root.controller.gateProgress) * steps)
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.space4
                    // Closer together on a narrow card, so a step is still a step.
                    spacing: width > 320 ? Theme.space1 : Theme.heavy

                    Repeater {
                        model: progress.steps
                        Rectangle {
                            required property int index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumWidth: 0
                            color: index < progress.filled ? Theme.accent : Theme.hoverWash
                            border.width: Theme.heavy
                            border.color: Theme.outline
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: !root.short
                    role: "caption"
                    horizontalAlignment: Text.AlignHCenter
                    text: ParentWording.gateKeys
                    wrapMode: Text.WordWrap
                }
            }
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
            root.controller.layoutGate(field.width, field.height, Theme.targetMin);
        }
    }

    FrameAnimation {
        running: root.controller.gateHolding
        onTriggered: root.controller.gateTick(root.now())
    }

    Component.onDestruction: root.endHold()
}
