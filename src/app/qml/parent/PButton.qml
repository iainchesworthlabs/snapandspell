import QtQuick

// A button for an adult's screen: at least 48 dp each way, a name and a role for
// assistive technology, the keyboard (Space and Enter), and a ring where the
// focus is. It is drawn from plain items, not Qt Quick Controls, so that it
// looks and measures the same under every platform style.
Item {
    id: root

    property string text
    // The main choice of a screen, or a quiet one.
    property bool primary: false
    // A choice among several, shown pressed in when it is the one chosen.
    property bool checkable: false
    property bool checked: false
    property bool compact: false

    signal clicked

    implicitWidth: Math.max(ParentTheme.targetMin, label.implicitWidth + 2 * ParentTheme.space4)
    implicitHeight: Math.max(ParentTheme.targetMin, label.implicitHeight + 2 * ParentTheme.space2)
    activeFocusOnTab: true

    Accessible.role: root.checkable ? Accessible.RadioButton : Accessible.Button
    Accessible.name: root.text
    Accessible.checkable: root.checkable
    Accessible.checked: root.checked
    Accessible.focusable: true
    Accessible.onPressAction: root.activate()

    function activate() {
        if (root.enabled) {
            root.clicked();
        }
    }

    Keys.onSpacePressed: event => {
        if (!event.isAutoRepeat) {
            root.activate();
        }
        event.accepted = true;
    }
    Keys.onReturnPressed: event => {
        root.activate();
        event.accepted = true;
    }
    Keys.onEnterPressed: event => {
        root.activate();
        event.accepted = true;
    }

    Rectangle {
        id: face
        anchors.fill: parent
        radius: ParentTheme.radius
        // State is never colour alone: a chosen option has a heavier edge as well.
        color: {
            if (!root.enabled) {
                return ParentTheme.page;
            }
            if (root.primary || root.checked) {
                return ParentTheme.buttonFill;
            }
            return taps.pressed ? ParentTheme.pressWash : (hover.hovered ? ParentTheme.hoverWash : ParentTheme.card);
        }
        border.width: root.checked ? ParentTheme.rule * 2 : ParentTheme.hair * 2
        border.color: root.enabled ? (root.checked ? ParentTheme.ink : ParentTheme.outline) : ParentTheme.hoverWash
    }

    Rectangle {
        // The focus ring, outside the face.
        anchors.fill: parent
        anchors.margins: -ParentTheme.space1
        radius: ParentTheme.radius + ParentTheme.space1
        color: "transparent"
        border.width: ParentTheme.rule
        border.color: ParentTheme.focusRing
        visible: root.activeFocus
    }

    Text {
        id: label
        anchors.centerIn: parent
        width: parent.width - 2 * ParentTheme.space3
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: root.text
        font.pixelSize: root.compact ? ParentTheme.sizeSmall : ParentTheme.sizeBody
        font.bold: root.checked || root.primary
        color: !root.enabled ? ParentTheme.outline : ((root.primary || root.checked) ? ParentTheme.buttonInk : ParentTheme.ink)
    }

    HoverHandler {
        id: hover
        enabled: root.enabled
    }
    TapHandler {
        id: taps
        enabled: root.enabled
        // A tap that ends outside the button is cancelled: slide off to cancel.
        onTapped: {
            root.forceActiveFocus();
            root.activate();
        }
    }
}
