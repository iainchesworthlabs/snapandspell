// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import SnapAndSpell
import SnapAndSpell.Scan

// The size matrix over the Family Assistant's screen, playing, with nothing found and with a word found, at every
// shape (apps/app/ui-tests/SizeMatrix.js). It is its own suite, apart from the scanner's screens
// (tst_sizematrix_scan.qml), so that each takes a share of the time a slow runner gives a suite.
//
// TEST DATA: the board is SAT across the top and PIN below it.
SizeMatrixCase {
    id: matrix

    name: "SizeMatrixAssist"
    subjects: [assistComponent, assistFoundComponent]

    readonly property var board: ["S", "A", "T", "X", "P", "I", "N", "X",
                                  "X", "X", "X", "X", "X", "X", "X", "X"]

    // assist.playing with nothing found yet: the empty card, the help, New board.
    Component {
        id: assistComponent
        Item {
            readonly property bool prepared: { fixture.reset(); assistModel.begin(4, 4, matrix.board); return true }
            AssistScreen { anchors.fill: parent; assist: assistModel }
        }
    }

    // assist.playing with a word found: its row in the card.
    Component {
        id: assistFoundComponent
        Item {
            readonly property bool prepared: {
                fixture.reset()
                assistModel.begin(4, 4, matrix.board)
                assistModel.tapTile(0)
                assistModel.tapTile(1)
                assistModel.tapTile(2)
                assistModel.done()
                return true
            }
            AssistScreen { anchors.fill: parent; assist: assistModel }
        }
    }
}
