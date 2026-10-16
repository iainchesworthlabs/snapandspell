pragma Singleton
import QtQuick

// Where the person is, and what is wired. A screen is named by its id
// (docs/design/application.md, "Screens and navigation"); the solo.* ids are one
// screen here, "solo", whose own state says which of them it shows.
//
// Back goes where the person came from, with the exceptions the screens make
// (from solo.resting it goes home, from solo.playing it puts the lid on).
QtObject {
    id: root

    property string screen: "home"
    property var history: []

    // Whether a screen exists for these, so that Home shows a picture only when
    // pressing it does something. Whoever adds the screen sets them in the same
    // change (the family game's scanner; the parent area's gate is routed in Main).
    property bool parentAvailable: true
    property bool scanAvailable: false

    function go(id) {
        if (id === screen) {
            return;
        }
        history = history.concat([screen]);
        screen = id;
    }

    function back() {
        if (history.length === 0) {
            return;
        }
        screen = history[history.length - 1];
        history = history.slice(0, history.length - 1);
    }

    function home() {
        history = [];
        screen = "home";
    }
}
