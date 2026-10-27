// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell
import SnapAndSpell.Parent

// The size matrix in other languages, on the shapes where words have the least room: a phone upright, small
// and on its side, and big text on one. German for the long translations, Hebrew for the right-to-left
// layout (the bar, the panel and the tabs mirror; the board's tiles do not), Japanese for the wide scripts in
// their own face. A screen a person meets first or most: Home, My jewels, the parent area with its gate open,
// and Solo Play's box and board. Every control that takes Tab focus is at least 48 dp each way, wholly inside
// the window, with no truncated label and on no other control (apps/app/ui-tests/SizeMatrix.js).
SizeMatrixCase {
    name: "SizeMatrixLanguages"
    settle: 700
    sizes: [
        {w: 390, h: 844, scale: 1.0},
        {w: 360, h: 640, scale: 1.0},
        {w: 844, h: 390, scale: 1.0},
        {w: 390, h: 844, scale: 1.5}
    ]
    languages: [
        {code: "de", rtl: false},
        {code: "he", rtl: true},
        {code: "ja", rtl: false}
    ]
    subjects: [homeComponent, jewelsComponent, areaComponent, boxComponent, playComponent]

    Component { id: stubScene; StubBoardScene { } }
    Component { id: homeComponent; Home { controller: parentController } }
    Component { id: jewelsComponent; JewelsScreen { controller: parentController } }
    Component {
        id: areaComponent
        ParentArea {
            controller: parentController
            Component.onCompleted: parentController.unlockForAssistiveTechnology()
        }
    }
    Component {
        id: boxComponent
        Item {
            id: host
            readonly property var game: { fixture.reset(); return soloModel }
            SoloScreen {
                sceneComponent: stubScene
                anchors.fill: parent
                solo: host.game
            }
        }
    }
    Component {
        id: playComponent
        Item {
            id: host
            readonly property var game: { fixture.reset(); return soloModel }
            SoloScreen {
                sceneComponent: stubScene
                anchors.fill: parent
                solo: host.game
            }
            Timer {
                interval: 50
                running: true
                onTriggered: host.game.shake()
            }
            Timer {
                interval: 600
                running: true
                onTriggered: {
                    host.game.tapTile(2)
                    host.game.tapTile(5)
                }
            }
        }
    }
}
