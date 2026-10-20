import QtQuick
import QtQuick.Shapes
import WordJewelsSolver

// An outline button that needs a deliberate press: hold it for a second while a
// ring fills (docs/design/game.md, "The resting screen"). It is a brake and not
// a gate: a child or a parent can do it. A screen reader, a switch or a key
// activates it by the usual action, with no hold. Letting go early starts the
// ring over; nothing is lost.
Item {
    id: root

    property string iconName: "again"
    property string label: ""
    property string accessibleName: ""
    property int holdMs: Theme.holdToPlay

    signal activated

    // 0 to 1, how far the ring has filled.
    property real progress: 0
    readonly property bool holding: hold.running

    implicitWidth: Theme.targetChild * 2
    implicitHeight: Theme.targetChild
    activeFocusOnTab: true

    Accessible.role: Accessible.Button
    Accessible.name: accessibleName
    Accessible.focusable: true
    Accessible.onPressAction: root.activated()

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: root.holding ? Theme.pressWash : Theme.card
        border.width: 1
        border.color: Theme.outline
    }

    Row {
        anchors.centerIn: parent
        spacing: Theme.space2
        Icon {
            name: root.iconName
            width: 26
            height: 26
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.ink
        }
        Text {
            visible: root.label.length > 0
            text: root.label
            anchors.verticalCenter: parent.verticalCenter
            font.family: Theme.uiFont
            font.pixelSize: Theme.labelSize
            font.weight: Font.DemiBold
            color: Theme.ink
        }
    }

    // The ring that fills, around the edge of the button.
    Shape {
        anchors.fill: parent
        visible: root.progress > 0
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.buttonFill
            strokeWidth: Theme.heavy * 2
            fillColor: "transparent"
            startX: 0
            startY: 0
            PathLine { x: root.width * Math.min(1, root.progress * 4); y: 0 }
        }
        ShapePath {
            strokeColor: Theme.buttonFill
            strokeWidth: Theme.heavy * 2
            fillColor: "transparent"
            startX: root.width
            startY: 0
            PathLine { x: root.width; y: root.height * Math.max(0, Math.min(1, root.progress * 4 - 1)) }
        }
        ShapePath {
            strokeColor: Theme.buttonFill
            strokeWidth: Theme.heavy * 2
            fillColor: "transparent"
            startX: root.width
            startY: root.height
            PathLine { x: root.width * (1 - Math.max(0, Math.min(1, root.progress * 4 - 2))); y: root.height }
        }
        ShapePath {
            strokeColor: Theme.buttonFill
            strokeWidth: Theme.heavy * 2
            fillColor: "transparent"
            startX: 0
            startY: root.height
            PathLine { x: 0; y: root.height * (1 - Math.max(0, Math.min(1, root.progress * 4 - 3))) }
        }
    }

    Rectangle {
        visible: root.activeFocus && keyboardFocus
        property bool keyboardFocus: false
        x: -4
        y: -4
        width: root.width + 8
        height: root.height + 8
        color: "transparent"
        border.width: Theme.rule
        border.color: Theme.ink
        id: focusRing
    }

    NumberAnimation {
        id: hold
        target: root
        property: "progress"
        from: 0
        to: 1
        duration: root.holdMs
        onFinished: {
            root.progress = 0;
            root.activated();
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: {
            focusRing.keyboardFocus = false;
            hold.restart();
        }
        onReleased: release()
        onCanceled: release()
        // Sliding off the button before the ring is full cancels.
        onContainsPressChanged: {
            if (!containsPress) {
                release();
            }
        }

        function release() {
            if (hold.running) {
                hold.stop();
                root.progress = 0;
            }
        }
    }

    Keys.onPressed: event => {
        focusRing.keyboardFocus = true;
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.activated();
            event.accepted = true;
        }
    }
}
