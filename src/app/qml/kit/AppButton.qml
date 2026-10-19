import QtQuick
import WordJewelsSolver

// The one button of the kit. A child's button carries a picture and a word, and
// speaks the word when pressed; a parent's button is the same control at 48 dp.
// The target sizes are tokens:
//
//   size "min"      48 dp   anything an adult touches
//   size "child"    64 dp   anything a child taps that is not primary
//   size "primary"  96 dp   Shake, Play, Done and the like
//
// `label` is the visible word. `accessibleName` falls back to the label, and
// `speaks` falls back to it too, so a labelled button is never silent.
Item {
    id: root

    property string variant: "secondary"   // primary | secondary | ghost | picture
    property string size: "child"          // min | child | primary
    property string iconName: ""
    property string label: ""
    property string accessibleName: ""
    property string speaks: ""
    property bool showFocus: activeFocus && focusByKey
    property bool focusByKey: false
    property real minimumWidth: 0
    property real radius: Theme.radius
    property color ink: Theme.ink
    property color edge: Theme.outline
    property color hoverFill: Theme.hoverWash
    property color pressFill: Theme.pressWash
    property bool checkable: false
    property bool checked: false

    default property alias content: contentArea.data

    signal clicked

    function activate() {
        Feedback.cue("tap", "");
        root.clicked();
    }

    readonly property real targetHeight: size === "primary" ? Theme.targetPrimary
                                       : size === "min" ? Theme.targetMin
                                       : Theme.targetChild
    readonly property bool pressed: mouse.pressed && mouse.containsMouse
    readonly property bool hovered: mouse.containsMouse
    readonly property bool isPrimary: variant === "primary"
    readonly property bool isGhost: variant === "ghost" || variant === "picture"
    readonly property bool hasLabel: label.length > 0
    readonly property real iconSize: size === "primary" ? 32 : size === "min" ? 20 : 28
    property real labelPointSize: size === "primary" ? Theme.childLabelSize
                                          : size === "min" ? Theme.smallSize
                                          : Theme.childLabelSize
    readonly property color contentColor: isPrimary ? Theme.buttonInk : root.ink

    implicitHeight: targetHeight
    implicitWidth: Math.max(targetHeight, minimumWidth,
                            hasLabel ? labelText.implicitWidth + (iconName !== "" ? iconSize + Theme.space2 : 0)
                                       + 2 * Theme.space3 : 0)
    activeFocusOnTab: true
    opacity: enabled ? 1.0 : 0.45

    Accessible.role: Accessible.Button
    Accessible.name: root.accessibleName.length > 0 ? root.accessibleName : root.label
    Accessible.focusable: true
    Accessible.checkable: checkable
    Accessible.checked: checked
    Accessible.onPressAction: root.activate()

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: {
            if (root.isPrimary) {
                return root.pressed ? Qt.darker(Theme.buttonFill, 1.15) : Theme.buttonFill;
            }
            if (root.isGhost) {
                return root.pressed ? root.pressFill : (root.hovered ? root.hoverFill : "transparent");
            }
            return root.pressed ? root.pressFill : (root.hovered ? root.hoverFill : Theme.card);
        }
        border.width: root.checked ? Theme.litEdge
                      : (root.variant === "secondary" ? Theme.rule : 0)
        border.color: root.checked ? Theme.gold : root.edge
    }

    Item {
        id: contentArea
        anchors.fill: parent
    }

    Row {
        visible: root.size !== "child" && (root.iconName !== "" || root.hasLabel)
        anchors.centerIn: parent
        spacing: Theme.space2

        Icon {
            visible: root.iconName !== ""
            name: root.iconName
            width: root.iconSize
            height: width
            anchors.verticalCenter: parent.verticalCenter
            color: root.contentColor
            strokeWidth: root.size === "primary" ? 2.4 : 2
        }
        Text {
            id: labelText
            visible: root.hasLabel
            text: root.label
            anchors.verticalCenter: parent.verticalCenter
            font.family: Theme.uiFont
            font.pixelSize: root.labelPointSize
            font.bold: root.isPrimary || root.checked
            color: root.contentColor
            elide: Text.ElideRight
        }
    }

    Column {
        visible: root.size === "child" && (root.iconName !== "" || root.hasLabel)
        anchors.centerIn: parent
        spacing: 2

        Icon {
            visible: root.iconName !== ""
            name: root.iconName
            width: root.iconSize
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            color: root.contentColor
            strokeWidth: 2
        }
        Text {
            visible: root.hasLabel
            text: root.label
            width: Math.min(root.width - 2 * Theme.space2, implicitWidth)
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            font.family: Theme.uiFont
            font.pixelSize: root.labelPointSize
            font.bold: true
            color: root.contentColor
            elide: Text.ElideRight
        }
    }

    Rectangle {
        visible: root.showFocus
        x: -4
        y: -4
        width: root.width + 8
        height: root.height + 8
        radius: root.radius + 4
        color: "transparent"
        border.width: Theme.rule
        border.color: Theme.focusRing
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onPressed: {
            root.focusByKey = false;
            const spoken = root.speaks.length > 0 ? root.speaks : root.label;
            if (spoken.length > 0) {
                Feedback.cue("speak", spoken);
            }
        }
        onClicked: root.activate()
    }

    Keys.onPressed: event => {
        root.focusByKey = true;
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.activate();
            event.accepted = true;
        }
    }
}
