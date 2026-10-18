pragma Singleton
import QtQuick

// Every sentence the Family Assistant's screens show, and every name they give
// to a screen reader, in one file: the i18n pass changed this file and no
// screen (docs/design/application.md, "Languages, right-to-left and fonts";
// game.md, "Mode A"). A screen says nothing of its own.
//
// The words are the design's. A message never blames a child for a camera or a
// reading: the reader's failures are "read again", and the camera's absence is a
// fact with the typed board beside it (docs/design/game.md, "No camera";
// docs/design/scanner.md, "A doubt is data, not a message").
QtObject {
    id: root

    // --- the way in and out --------------------------------------------------
    readonly property string back: qsTr("Back")
    readonly property string parent: qsTr("For grown-ups")

    // --- scan.ready ----------------------------------------------------------
    readonly property string readyTitle: qsTr("Put the board in the frame")
    readonly property string capture: qsTr("Capture")
    readonly property string typeBoard: qsTr("Type in your board")
    readonly property string reading: qsTr("Reading the board…")
    readonly property string cameraAbsent: qsTr("There is no camera on this device. You can type in your board.")
    readonly property string cameraRefused: qsTr("The camera is not allowed. You can type in your board.")
    readonly property string cameraBroken: qsTr("The camera did not start. You can type in your board.")
    readonly property string readAgainMessage: qsTr("I could not read the board. Try again, or type it in.")

    function cameraProblem(code) {
        switch (code) {
        case "no_camera":
        case "unavailable":
        case "no_such_device":
            return root.cameraAbsent;
        case "permission":
        case "permission_denied":
            return root.cameraRefused;
        case "already_running":
        case "failed":
            return root.cameraBroken;
        default:
            return "";
        }
    }

    function readProblem(code) {
        switch (code) {
        case "no_camera":
            return root.cameraAbsent;
        case "":
            return "";
        default:
            return root.readAgainMessage;
        }
    }

    // --- scan.check ----------------------------------------------------------
    readonly property string checkTitle: qsTr("Is this your board?")
    //: Rotate the replica of the real board a quarter turn.
    readonly property string turn: qsTr("Turn the board")
    readonly property string boardSize: qsTr("Board size")
    readonly property string doubtfulHint: qsTr("A ringed tile may be wrong. Tap it to change the letter.")
    readonly property string incomplete: qsTr("Every tile needs a letter before the board can play.")
    readonly property string noGame: qsTr("The board could not be started. Try again.")
    //: Go back to the camera and read the board again.
    readonly property string readAgain: qsTr("Read again")
    //: The grown-up confirms the board and starts the round.
    readonly property string yesPlay: qsTr("Yes, play")
    readonly property string doubtfulMark: "?"

    function sizeChoice(side) {
        return qsTr("%1 by %1 board").arg(side);
    }

    function sizeLabel(side) {
        return qsTr("%1 x %1").arg(side);
    }

    // --- the letter picker ---------------------------------------------------
    readonly property string pickerTitle: qsTr("Which letter is it?")
    readonly property string pickerSuggestions: qsTr("The reader's other thoughts")
    //: The two-letter tile as the dice print it: Q and U on one face.
    readonly property string pickerQu: qsTr("Qu")
    readonly property string pickerOther: qsTr("Other letters")
    readonly property string useLetters: qsTr("Use these")
    readonly property string closePicker: qsTr("Close")

    // --- assist.playing ------------------------------------------------------
    readonly property string wordsFound: qsTr("Words I found")
    readonly property string noWordsYet: qsTr("No words yet")
    readonly property string showStart: qsTr("Show me a start")
    readonly property string newBoard: qsTr("New board")
    //: The screen-reader name of the ring that lights the start tile.
    readonly property string startHere: qsTr("Start here")
    readonly property string earlierBoards: qsTr("Earlier boards")

    function tileName(letters, row, column) {
        return qsTr("%1, row %2, column %3").arg(letters).arg(row).arg(column);
    }

    function foundName(word) {
        return qsTr("%1, found").arg(word);
    }

    //: The screen-reader name of the replica of the board.
    readonly property string board: qsTr("Board")
}
