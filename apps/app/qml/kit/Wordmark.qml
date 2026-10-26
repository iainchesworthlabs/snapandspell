// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import WordJewelsSolver

// The name is a brand, in Latin in every language, so it keeps the house faces (Pixelify Sans, Silkscreen)
// where the chrome around it is set in a language's own face.
// The lockup: a faceted jewel and the name. Word Jewels is the child-facing
// name; SOLVER is the product qualifier, in the house mono face. The same
// lockup is used in the home bar, the parent area's bar and the About page.
// Both words are Labels, so they follow the parent's Text size setting like the
// rest of the bar (they were 23/18 px and 9/7 px literals that ignored it).
Row {
    id: root

    property bool compact: false
    // Home: the name has a hard coral shadow under it, as the title of a game.
    property bool shadowed: false
    spacing: compact ? Theme.space2 : Theme.space3

    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("Word Jewels") + " " + qsTr("SOLVER")

    Jewel {
        width: root.compact ? Theme.jewelSm : Theme.jewelMd
        height: width
        anchors.verticalCenter: parent.verticalCenter
        tier: "power"
        color: root.shadowed ? Theme.coral : Theme.gold
    }

    Column {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        Item {
            implicitWidth: nameText.implicitWidth + Theme.shadowSmall
            implicitHeight: nameText.implicitHeight + Theme.shadowSmall
            width: implicitWidth
            height: implicitHeight

            Label {
                visible: root.shadowed
                x: Theme.shadowSmall
                y: Theme.shadowSmall
                role: "display"
                font.family: "Pixelify Sans"
                decorative: true
                text: nameText.text
                size: nameText.size
                lineRatio: nameText.lineRatio
                color: Theme.coral
            }
            Label {
                id: nameText
                role: "display"
                font.family: "Pixelify Sans"
                decorative: true
                text: qsTr("Word Jewels")
                size: root.compact ? Theme.headingSize : Theme.wordSize
                lineRatio: Theme.lineHeading
            }
        }
        Label {
            role: "eyebrow"
            font.family: "Silkscreen"
            decorative: true
            text: qsTr("SOLVER")
        }
    }
}
