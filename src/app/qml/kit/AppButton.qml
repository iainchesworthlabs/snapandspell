import QtQuick
import WordJewelsSolver

// The one button of the kit. Flat fills and 2 px rules, square corners, no
// shadow. Its size is a target token, never a number written at the place of use:
//
//   size "min"      48 dp   anything an adult touches
//   size "child"    64 dp   anything a child taps that is not primary
//   size "primary"  96 dp   Shake and the like, and at least a third of the width
//
// A child's button says what it does with a picture (`iconName`, or the content
// placed inside), not with words; `accessibleName` is for the screen reader and
// the person who cannot see the picture. It acts on release inside it, so a slide
// off the button cancels, it speaks its name when pressed, and its press plays
// the quiet tap of a control (Feedback.cue, carried out by Main.qml's engine).
Item {
    id: root

    property string variant: "secondary"   // primary | secondary | ghost | picture
    property string size: "child"          // min | child | primary
    property string iconName: ""
    property string accessibleName: ""
    property string speaks: ""             // the name the press cue carries
    property bool showFocus: activeFocus && focusByKey
    property bool focusByKey: false
    property real minimumWidth: 0
    // For a button on a card of its own colour (the hint's amber): its ink and edge.
    property color ink: Theme.ink
    property color edge: Theme.outline
    property color hoverFill: Theme.hoverWash
    property color pressFill: Theme.pressWash
    // A choice among several (the board size): chosen is a heavier edge as well as
    // a state the screen reader hears, never colour alone.
    property bool checkable: false
    property bool checked: false

    default property alias content: contentArea.data

    signal clicked

    // Every way of activating the button (a pointer, Space or Enter, a screen
    // reader's action) is the same click and the same quiet tap.
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

    implicitHeight: targetHeight
    implicitWidth: Math.max(targetHeight, minimumWidth)
    activeFocusOnTab: true

    opacity: enabled ? 1.0 : 0.45

    Accessible.role: Accessible.Button
    Accessible.name: accessibleName
    Accessible.focusable: true
    Accessible.checkable: checkable
    Accessible.checked: checked
    Accessible.onPressAction: root.activate()

    Rectangle {
        anchors.fill: parent
        color: root.isPrimary ? (root.pressed ? Qt.darker(Theme.buttonFill, 1.2) : Theme.buttonFill)
             : root.pressed ? root.pressFill
             : root.hovered ? root.hoverFill
             : "transparent"
        border.width: root.checked ? Theme.litEdge : (root.variant === "secondary" ? Theme.rule : 0)
        border.color: root.checked ? root.ink : root.edge
    }

    Item {
        id: contentArea
        anchors.fill: parent
    }

    Icon {
        visible: root.iconName !== ""
        name: root.iconName
        width: Math.min(root.height * 0.5, Theme.iconMax)
        height: width
        anchors.centerIn: parent
        color: root.isPrimary ? Theme.buttonInk : root.ink
        strokeWidth: root.size === "primary" ? 2.4 : 2
    }

    // Keyboard focus: a 2 px ring in ink, 2 px outside the control with the page
    // showing between. Keyboard only, never for a tap or a click.
    Rectangle {
        visible: root.showFocus
        x: -4
        y: -4
        width: root.width + 8
        height: root.height + 8
        color: "transparent"
        border.width: Theme.rule
        border.color: Theme.ink
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onPressed: {
            root.focusByKey = false;
            if (root.speaks !== "") {
                Feedback.cue("speak", root.speaks);
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
