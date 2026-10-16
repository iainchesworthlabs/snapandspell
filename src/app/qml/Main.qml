pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import WordJewelsSolver
import WordJewelsSolver.Parent

// The window: a thin shell that shows the screen Navigation names. Add a screen as
// one `case` and one `Component` line, and flip its availability on Navigation in
// the same change. Back goes where the person came from (Navigation), Esc and the
// system back do what the visible back control does, and on home they do nothing.
//
// The parent area (gate, dashboard, settings, about) is one screen of it, reached
// from home and from the box by the small button in the corner. It is the parent
// area's own gate that stands in front of it; nothing here opens it.
ApplicationWindow {
    id: window

    // The parent area's controller, made by main.cpp over the program's one store,
    // which the game also works on (so a change made in the settings is what the
    // next game is made from).
    required property var parentController

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

    // What the parent chooses for the whole program: the Motion override (the
    // system's own setting is not read yet) and the Text size.
    Binding {
        target: Theme
        property: "reducedMotion"
        value: window.parentController.reducedMotion
    }
    Binding {
        target: Theme
        property: "textScale"
        value: window.parentController.textScale
    }

    // A setting the parent changed, or a reset, is read by the next game.
    Connections {
        target: window.parentController
        function onStoreEdited() {
            soloGame.storeEdited();
        }
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
                case "parent":
                    return parentScreen;
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
            onParentRequested: Navigation.go("parent")
        }
    }
    Component {
        id: soloScreen
        SoloScreen {
            solo: soloGame
            onHomeRequested: Navigation.home()
            onParentRequested: Navigation.go("parent")
        }
    }
    Component {
        id: parentScreen
        // Back goes where the person came from, home or the box.
        ParentArea {
            controller: window.parentController
            onClosed: Navigation.back()
        }
    }
}
