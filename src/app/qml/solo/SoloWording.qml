pragma Singleton
import QtQuick

// Every sentence and every screen-reader name of Solo Play's chrome, in one
// file: the bar's way back and corner buttons, the box's size choice and Shake,
// the tray, the hint, the history and the resting screen. A screen says nothing
// of its own.
//
// The game itself is English: the dice letters, the words in the tray and the
// word list are not translated (docs/audit-0.1.md, "Languages, RTL and i18n").
// These are the words around them.
QtObject {
    id: root

    // --- the bar -------------------------------------------------------------
    readonly property string back: qsTr("Back")
    readonly property string earlierBoards: qsTr("Earlier boards")
    readonly property string forGrownUps: qsTr("For grown-ups")

    // --- the box -------------------------------------------------------------
    //: Shake the box to roll the dice; the button and Space do the same.
    readonly property string shake: qsTr("Shake")

    //: The size of the board: "3 by 3 board".
    function sizeBoard(side) {
        return qsTr("%1 by %1 board").arg(side);
    }

    // --- the panel -----------------------------------------------------------
    readonly property string wordsFound: qsTr("Words I found")
    //: Light the first tile of a word that has not been found yet.
    readonly property string showStart: qsTr("Show me a start")
    //: Hand in the word that has been traced or tapped.
    readonly property string done: qsTr("Done")

    function found(word) {
        return qsTr("%1, found").arg(word);
    }

    // --- the history sheet -----------------------------------------------------
    readonly property string close: qsTr("Close")

    // --- the resting screen -----------------------------------------------------
    readonly property string playAgain: qsTr("Play again")

    // --- the board's names for a screen reader -----------------------------------
    readonly property string board: qsTr("Board")
    //: The screen-reader name of the ring that lights the start tile.
    readonly property string startHere: qsTr("Start here")

    function tileName(letters, row, column) {
        return qsTr("%1, row %2, column %3").arg(letters).arg(row).arg(column);
    }

    function jewelName(row, column) {
        return qsTr("Jewel, row %1, column %2").arg(row).arg(column);
    }
}
