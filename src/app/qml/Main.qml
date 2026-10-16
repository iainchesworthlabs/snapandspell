pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import WordJewelsSolver

// The window: a thin shell that shows the screen Navigation names. Add a screen as
// one `case` and one `Component` line, and flip its availability on Navigation in
// the same change. Back goes where the person came from (Navigation), Esc and the
// system back do what the visible back control does, and on home they do nothing.
ApplicationWindow {
    id: window

    width: 1100
    height: 700
    minimumWidth: 320
    minimumHeight: 320
    visible: true
    title: qsTr("Word Jewels Solver")
    color: Theme.page

    // The Solo Play game lives for the whole run, so a board in the box survives a
    // trip to home and back.
    SoloModel {
        id: soloGame
        onCue: (kind, detail) => Feedback.cue(kind, detail)
    }

    // Esc, and the system's back key, go back. A screen that has a deeper state
    // (a path being made, a sheet) takes the key first and lets it through when it
    // has nothing to undo.
    FocusScope {
        id: shell
        anchors.fill: parent
        focus: true

        // The screen on show, untyped: a screen with a deeper state has a back().
        readonly property var current: screens.item

        function goBack() {
            if (current && typeof current.back === "function") {
                current.back();
            } else {
                Navigation.back();
            }
        }

        Keys.onEscapePressed: event => {
            if (Navigation.screen !== "home") {
                goBack();
                event.accepted = true;
            }
        }
        Keys.onBackPressed: event => {
            if (Navigation.screen !== "home") {
                goBack();
                event.accepted = true;
            }
        }

        Loader {
            id: screens
            anchors.fill: parent
            focus: true
            // Each screen fades in rather than replacing the last in one frame, and
            // takes the keyboard (Space on the box, the arrow keys on the board).
            onLoaded: {
                screenFade.restart();
                (item as Item).forceActiveFocus();
            }
            sourceComponent: {
                switch (Navigation.screen) {
                case "solo":
                    return soloScreen;
                default:
                    return homeScreen;
                }
            }
        }

        NumberAnimation {
            id: screenFade
            target: screens
            property: "opacity"
            from: 0
            to: 1
            duration: Theme.screen
            easing.type: Easing.OutCubic
        }
    }

    Component {
        id: homeScreen
        Home {
            onSoloRequested: Navigation.go("solo")
        }
    }
    Component {
        id: soloScreen
        SoloScreen {
            solo: soloGame
            onHomeRequested: Navigation.home()
        }
    }
}
