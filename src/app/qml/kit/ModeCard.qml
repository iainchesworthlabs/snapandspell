import QtQuick
import WordJewelsSolver

// A large choice on the home screen: a picture, a verb and a short line. The
// whole card is the target, so a child does not have to find a small button.
//
// The picture takes the space the words leave, not a fixed share of the card.
// At the minimum window height the card is shorter; the art shrinks and the
// label block keeps its place at the bottom instead of the two overlapping.
Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property string actionLabel: ""
    property string eyebrow: ""
    property string accessibleName: ""
    property bool primary: false
    property bool grownUp: false

    default property alias art: artArea.data
    signal clicked

    implicitHeight: 390
    radius: Theme.radiusLarge
    color: primary ? Theme.surface : Theme.card
    border.width: primary ? 2 : 1
    border.color: primary ? Theme.accent : Theme.line
    activeFocusOnTab: true
    focus: primary

    Accessible.role: Accessible.Button
    Accessible.name: root.accessibleName.length > 0 ? root.accessibleName : root.title
    Accessible.onPressAction: root.clicked()

    Keys.onSpacePressed: event => {
        root.clicked();
        event.accepted = true;
    }
    Keys.onReturnPressed: event => {
        root.clicked();
        event.accepted = true;
    }
    Keys.onEnterPressed: event => {
        root.clicked();
        event.accepted = true;
    }

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: taps.pressed ? Theme.pressWash : (hover.hovered ? Theme.hoverWash : "transparent")
    }

    Item {
        id: artArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: words.top
        anchors.margins: Theme.space4
        anchors.bottomMargin: Theme.space3
    }

    Column {
        id: words
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.space6
        spacing: Theme.space1

        Text {
            visible: root.eyebrow.length > 0
            text: root.eyebrow
            font.family: Theme.monoFont
            font.pixelSize: Theme.monoSize
            font.letterSpacing: 1.2
            color: root.grownUp ? Theme.inkSecondary : Theme.accent
            textFormat: Text.PlainText
        }
        Text {
            width: parent.width
            text: root.title
            font.family: Theme.uiFont
            font.pixelSize: root.primary ? Theme.titleSize : Theme.headingSize
            font.weight: Font.ExtraBold
            color: Theme.ink
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        Text {
            width: parent.width
            text: root.subtitle
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            color: Theme.inkSecondary
            wrapMode: Text.WordWrap
            maximumLineCount: root.primary ? 2 : 3
            elide: Text.ElideRight
        }
        Row {
            spacing: Theme.space2
            Text {
                text: root.actionLabel
                font.family: Theme.uiFont
                font.pixelSize: Theme.labelSize
                font.weight: Font.DemiBold
                color: Theme.accent
            }
            Text {
                text: "→"
                font.family: Theme.uiFont
                font.pixelSize: Theme.labelSize
                color: Theme.accent
            }
        }
    }

    HoverHandler {
        id: hover
    }
    TapHandler {
        id: taps
        onTapped: root.clicked()
    }
}
