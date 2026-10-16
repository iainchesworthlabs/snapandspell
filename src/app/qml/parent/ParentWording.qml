pragma Singleton
import QtQuick

// Every sentence a parent reads on the parent area's screens, in one place.
//
// The wording is a **proposal for the literacy review** (gate G7,
// docs/design/dashboard.md, "Wording"; docs/product-requirements.md, 12.6): it
// is observations and never assessments, and never about a gap. The sentences
// the design quotes are here as it quotes them:
//
//   "Words with ai have been on 6 boards. 4 were found."
//   "Found without a start tile: 3."
//   "This week: 3 boards cleared, 14 stars, 2 new words."
//   "not met yet"
//   "few words use this sound yet"
//   "on 6 boards; found on 1"
//
// A sound that has not been met says "not met yet", never "missed". Nothing is
// said about a week in which the child did not play: a gap in play is not a
// deficit. The test parent.wording holds the file to the design's sentences and
// to a list of words that must not be in it ("weak", "mastered", "missed"...).
//
// A number is never put in a sentence by pasting: each sentence that counts has
// its singular ("1 board") and its plural ("6 boards"), because the language of
// the sentence is this file's and not the code's.
QtObject {
    // --- the screens ------------------------------------------------------
    readonly property string parentArea: qsTr("Parent area")
    readonly property string back: qsTr("Back")
    readonly property string tabDashboard: qsTr("Dashboard")
    readonly property string tabSettings: qsTr("Settings")
    readonly property string tabAbout: qsTr("About")
    readonly property string nothingYet: qsTr("Nothing to show yet.")

    // --- the store's notes ---------------------------------------------------
    readonly property string noteUnreadable: qsTr("The saved progress could not be read, so a new one was started. The old file was kept on this device.")
    readonly property string noteNewer: qsTr("The saved progress was made by a newer version of this app. It has been left as it is, and nothing will be saved this time.")
    readonly property string noteUnsaved: qsTr("Progress could not be saved on this device.")

    // --- the gate -------------------------------------------------------------
    readonly property string gateTitle: qsTr("For parents")
    readonly property string gateInstruction: qsTr("Hold both circles at the same time for two seconds.")
    readonly property string gateKeys: qsTr("With a mouse and a keyboard: hold the left circle with the mouse and press Space.")
    readonly property string gateSpot: qsTr("Open the parent area. Hold both circles with two fingers, or activate this to open it.")

    // --- the dashboard ---------------------------------------------------------
    readonly property string soundsHeading: qsTr("Sounds")
    readonly property string vaultHeading: qsTr("The vault")
    readonly property string bestHeading: qsTr("Best")
    readonly property string notMetYet: qsTr("not met yet")
    readonly property string oftenFound: qsTr("Often found")
    readonly property string fewWords: qsTr("few words use this sound yet")

    function boards(n) {
        return n === 1 ? qsTr("1 board") : qsTr("%1 boards").arg(n);
    }

    function stars(n) {
        return n === 1 ? qsTr("1 star") : qsTr("%1 stars").arg(n);
    }

    // "Words with ai have been on 6 boards. 4 were found."  (letters in italics)
    function soundSentence(letters, met, found) {
        const foundPart = found === 1 ? qsTr("1 was found.") : qsTr("%1 were found.").arg(found);
        return qsTr("Words with <i>%1</i> have been on %2. %3").arg(letters).arg(boards(met)).arg(foundPart);
    }

    // "Found without a start tile: 3."
    function foundWithoutStartTile(n) {
        return qsTr("Found without a start tile: %1.").arg(n);
    }

    // "on 6 boards; found on 1": the same two counts, shorter.
    function onBoardsFoundOn(met, found) {
        return qsTr("on %1; found on %2").arg(boards(met)).arg(found);
    }

    // "This week: 3 boards cleared, 14 stars, 2 new words." A part that is
    // nothing is left out, and a week with nothing in it is not said at all
    // (weekPlayed is the caller's check): a gap in play is not a deficit.
    function weekSentence(cleared, starCount, newWords) {
        const parts = [];
        if (cleared > 0) {
            parts.push(cleared === 1 ? qsTr("1 board cleared") : qsTr("%1 boards cleared").arg(cleared));
        }
        if (starCount > 0) {
            parts.push(stars(starCount));
        }
        if (newWords > 0) {
            parts.push(newWords === 1 ? qsTr("1 new word") : qsTr("%1 new words").arg(newWords));
        }
        return qsTr("This week: %1.").arg(parts.join(", "));
    }

    function longestWord(word) {
        return qsTr("The longest word: %1.").arg(word);
    }

    // "3x3: 12 stars"
    function bestLine(size, starCount) {
        return qsTr("%1x%1: %2").arg(size).arg(stars(starCount));
    }

    // --- the settings ----------------------------------------------------------
    readonly property string settingsHelp: qsTr("Help")
    readonly property string settingsHelpHint: qsTr("The start tile: one tile glows to show where a word begins.")
    readonly property string settingsBoard: qsTr("Board")
    readonly property string settingsBoardHint: qsTr("Standard: dice and any word. Phonics: the words of a phase, with their sounds.")
    readonly property string settingsPhase: qsTr("Phase")
    readonly property string settingsFocus: qsTr("Focus sounds")
    readonly property string settingsFocusHint: qsTr("The sounds the school is on this week. The phonics board prefers words with them.")
    readonly property string settingsBoardSize: qsTr("Board size")
    readonly property string settingsBoardSizeHint: qsTr("The size the box offers first.")
    readonly property string settingsSoundItOut: qsTr("Sound it out")
    readonly property string settingsCelebration: qsTr("Celebration sounds")
    readonly property string settingsSound: qsTr("Sound")
    readonly property string settingsVibration: qsTr("Vibration")
    readonly property string settingsShake: qsTr("Shake the device")
    readonly property string settingsMotion: qsTr("Motion")
    readonly property string settingsMotionHint: qsTr("System follows the device's own setting.")
    readonly property string settingsTextSize: qsTr("Text size")
    readonly property string settingsSchoolFont: qsTr("School font")

    readonly property string valueOn: qsTr("On")
    readonly property string valueOff: qsTr("Off")
    readonly property string valueStandard: qsTr("Standard")
    readonly property string valuePhonics: qsTr("Phonics")
    readonly property string valueFull: qsTr("Full")
    readonly property string valueWordOnly: qsTr("The word only")
    readonly property string valueQuiet: qsTr("Quiet")
    readonly property string valueSystem: qsTr("System")
    readonly property string valueReduced: qsTr("Reduced")

    // --- reset -------------------------------------------------------------------
    readonly property string resetHeading: qsTr("Reset")
    readonly property string resetBoards: qsTr("Clear boards and finds")
    readonly property string resetBoardsSays: qsTr("This clears the last boards and the words found on them. Stars and stickers stay.")
    readonly property string resetStars: qsTr("Clear stars and stickers")
    readonly property string resetStarsSays: qsTr("This clears the stars, the best of each board size and the stickers. Boards and finds stay.")
    readonly property string resetConfirm: qsTr("Clear")
    readonly property string resetKeep: qsTr("Keep")
    readonly property string resetDone: qsTr("Cleared.")

    // --- about -----------------------------------------------------------------------
    readonly property string aboutHeading: qsTr("About")
    readonly property string licencesHeading: qsTr("Licences")
    readonly property string privacyHeading: qsTr("Privacy")

    function version(v) {
        return qsTr("Word Jewels Solver, version %1").arg(v);
    }

    readonly property string licenceNotice: qsTr("This program is free software. You can redistribute it and change it under the terms of the GNU General Public License, version 3, or (at your option) any later version.")
    readonly property string licenceParts: qsTr("It is built with Qt. The English word list is ENABLE, a public-domain list. The licences of everything in a package are listed with the package.")
    readonly property string privacyKept: qsTr("This app keeps what this screen shows, on this device only: the words found, the size of each board, the day, whether a start tile was shown, the stars and stickers, the last twenty boards and your settings.")
    readonly property string privacyNever: qsTr("It does not record sound or pictures, does not use the network and sends nothing anywhere. It does not keep how long anyone plays.")
    readonly property string privacyDevice: qsTr("Nothing moves between devices. A new device starts fresh.")
}
