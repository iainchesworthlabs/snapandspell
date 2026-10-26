// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell

// The name is a brand, in Latin in every language, so it keeps the house faces (Pixelify Sans, Silkscreen)
// where the chrome around it is set in a language's own face.
// The lockup: a faceted jewel and the name, Snap and Spell. The same lockup is used in the home bar,
// the parent area's bar and the About page. The name is a Label, so it follows the parent's Text size
// setting like the rest of the bar.
Row {
    id: root

    property bool compact: false
    // Home: the name has a hard coral shadow under it, as the title of a game.
    property bool shadowed: false
    spacing: compact ? Theme.space2 : Theme.space3

    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("Snap and Spell")

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
                text: qsTr("Snap and Spell")
                size: root.compact ? Theme.headingSize : Theme.wordSize
                lineRatio: Theme.lineHeading
            }
        }
    }
}
