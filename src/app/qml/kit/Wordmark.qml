import QtQuick
import WordJewelsSolver

// The lockup: a faceted jewel and the name. Word Jewels is the child-facing
// name; SOLVER is the product qualifier, in the house mono face. The same
// lockup is used in the home bar and the About page.
Row {
    id: root

    property bool compact: false
    spacing: compact ? Theme.space2 : Theme.space3

    Jewel {
        width: root.compact ? 22 : 30
        height: width
        anchors.verticalCenter: parent.verticalCenter
        tier: "power"
        color: Theme.gold
    }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        Text {
            text: qsTr("Word Jewels")
            font.family: Theme.uiFont
            font.pixelSize: root.compact ? 18 : 23
            font.weight: Font.ExtraBold
            font.letterSpacing: -0.6
            color: Theme.ink
        }
        Text {
            text: qsTr("SOLVER")
            font.family: Theme.monoFont
            font.pixelSize: root.compact ? 7 : 9
            font.letterSpacing: 1.4
            color: Theme.inkSecondary
        }
    }
}
