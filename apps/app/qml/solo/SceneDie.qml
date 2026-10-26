// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell

// One die of the board scene: the kit's tile face (cream, a 3 px outline, a hard shadow, the Andika
// letter), or, for a dead tile, the same face with a pixel jewel on it, as the flat board draws it
// (docs/design/board.md, "The tile"). The scene only moves it and fades it; there is no shader and no
// effect layer.
Item {
    id: root

    property string letters: ""

    readonly property bool dead: letters === ""

    TileFace {
        anchors.fill: parent
        letters: root.letters
    }

    Jewel {
        visible: root.dead
        anchors.centerIn: parent
        width: parent.width * 0.58
        height: width
        tier: "power"
        color: Theme.jewelDead
    }
}
