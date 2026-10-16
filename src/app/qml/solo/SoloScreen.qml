pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// Solo Play, Mode B: solo.box, solo.playing and solo.resting (and the moments
// between them: shaking, dropping, word-found, cleared) on one screen, because the
// board scene is ONE component that stays while the lid lifts and the dice fall.
// docs/design/application.md ("Screens and navigation"), game.md ("Mode B").
//
// The window is split by its shape, not by what device it is: the board in the
// leading two thirds of a wide window and the panel in the trailing third; the
// board in the top two thirds of a tall one and the panel below. A bar above them
// holds the way back and the quiet corner buttons, so neither lies over the board.
// In the box the panel's place holds the size choice and Shake; in play it holds
// the tray, the hint and the history.
//
// A visible way back is always here: from the box it goes to home; from play it
// puts the lid on (the board is saved); from resting it goes to home. Esc and the
// system back do the same.
FocusScope {
    id: root

    required property var solo

    signal homeRequested
    signal parentRequested

    readonly property string soloState: solo.state
    readonly property real barOffset: Theme.barHeight

    SoloLayout {
        id: rule
        width: root.width
        height: root.height - root.barOffset
        boardSize: root.solo.boardSize
    }

    readonly property rect boardArea: Qt.rect(rule.boardArea.x, rule.boardArea.y + barOffset, rule.boardArea.width, rule.boardArea.height)
    readonly property rect panelArea: Qt.rect(rule.panelArea.x, rule.panelArea.y + barOffset, rule.panelArea.width, rule.panelArea.height)

    // The back control's meaning, shared by the button, Esc and the system back.
    function back() {
        if (history.open) {
            history.close();
            return;
        }
        switch (solo.state) {
        case "box":
            homeRequested();
            break;
        case "resting":
            solo.again();
            homeRequested();
            break;
        case "cleared":
        case "word-found":
            if (solo.state === "word-found") {
                solo.wordShown();
            }
            if (solo.state === "cleared") {
                solo.celebrated();
                solo.again();
                homeRequested();
            } else {
                solo.lidOn();
            }
            break;
        default:
            solo.lidOn();
            break;
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Space && solo.state === "box") {
            solo.shake();
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    // --- the board and the panel -------------------------------------------------------
    Item {
        id: stage
        anchors.fill: parent
        visible: root.soloState !== "resting"

        Rectangle {
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            color: Theme.boardArea
        }

        BoardScene {
            id: scene
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            reducedMotion: Theme.reducedMotion

            onSettled: root.solo.sceneSettled()
            onLanded: (index, pitch) => Feedback.cue("tile-landed", String(pitch))
            onRattled: Feedback.cue("rattle", "")
            onTileSizeChanged: root.solo.setGeometry(tileSize, tileGap)
            onTileGapChanged: root.solo.setGeometry(tileSize, tileGap)

            function closedBox() {
                return { rows: root.solo.boardSize, columns: root.solo.boardSize, tiles: [] };
            }
            // What is on the table when the screen opens: the lid on, as it is in the
            // box, or, for a game brought back from the run before, the board already
            // down and still. Resting shows no scene, so the box is there for Again.
            function opening() {
                if (root.solo.state === "playing") {
                    return { rows: root.solo.rows, columns: root.solo.columns, tiles: root.solo.tiles };
                }
                return closedBox();
            }
            Component.onCompleted: {
                show(opening());
                root.solo.setGeometry(tileSize, tileGap);
            }
        }

        // The pointer, the keyboard and the screen reader's tiles, over the scene.
        TraceLayer {
            id: trace
            x: scene.x
            y: scene.y
            width: scene.width
            height: scene.height
            solo: root.solo
            boardRect: scene.boardRect
            tileSize: scene.tileSize
            tileGap: scene.tileGap
            focus: true
        }

        // A single slow glow over the board when the last word lands. Once, never
        // faster than three a second, and not at all with reduced motion.
        Rectangle {
            id: glow
            x: scene.x + scene.boardRect.x
            y: scene.y + scene.boardRect.y
            width: scene.boardRect.width
            height: scene.boardRect.height
            color: Theme.dieLit
            opacity: 0
            SequentialAnimation on opacity {
                id: glowAnimation
                running: false
                NumberAnimation { to: Theme.glowPeak; duration: Theme.glowRise; easing.type: Easing.InOutQuad }
                NumberAnimation { to: 0; duration: Theme.glowFall; easing.type: Easing.InOutQuad }
            }
        }

        // --- the panel's place: the box's controls, or the panel ---------------------------
        Item {
            id: panelSlot
            x: root.panelArea.x
            y: root.panelArea.y
            width: root.panelArea.width
            height: root.panelArea.height
            clip: true

            readonly property bool boxShown: root.soloState === "box"
            readonly property bool panelShown: root.soloState === "playing" || root.soloState === "word-found" || root.soloState === "cleared"
            readonly property real distance: rule.wide ? width : height

            SoloBox {
                id: boxControls
                width: parent.width
                height: parent.height
                solo: root.solo
                layoutRule: rule
                wide: rule.wide
                opacity: panelSlot.boxShown ? 1 : 0
                visible: opacity > 0
                enabled: panelSlot.boxShown
                onShakeRequested: root.solo.shake()
                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.turn
                    }
                }
            }

            PlayingPanel {
                id: panel
                solo: root.solo
                wide: rule.wide
                width: parent.width
                height: parent.height
                // Slides in from the trailing edge or the foot; with reduced motion it
                // cross-fades instead.
                property real shown: panelSlot.panelShown ? 1 : 0
                Behavior on shown {
                    NumberAnimation {
                        duration: Theme.turn
                        easing.type: Easing.OutCubic
                    }
                }
                opacity: Theme.reducedMotion ? shown : 1
                visible: shown > 0
                x: rule.wide && !Theme.reducedMotion ? (1 - shown) * panelSlot.distance : 0
                y: !rule.wide && !Theme.reducedMotion ? (1 - shown) * panelSlot.distance : 0
                onHistoryRequested: history.open = true
            }
        }

        // --- the bar: the way back, and the quiet corner buttons --------------------------------
        AppButton {
            id: backButton
            x: Theme.space2
            y: Theme.space2 / 2
            iconName: "back"
            accessibleName: qsTr("Back")
            onClicked: root.back()
        }
        Row {
            anchors.right: parent.right
            anchors.rightMargin: Theme.space2
            y: Theme.space2 / 2
            spacing: Theme.targetGap
            visible: root.soloState === "box"

            AppButton {
                iconName: "history"
                accessibleName: qsTr("Earlier boards")
                onClicked: history.open = true
            }
            AppButton {
                visible: Navigation.parentAvailable
                iconName: "parent"
                accessibleName: qsTr("For grown-ups")
                onClicked: root.parentRequested()
            }
        }
    }

    // --- resting --------------------------------------------------------------------------------
    Loader {
        anchors.fill: parent
        active: root.soloState === "resting"
        sourceComponent: SoloResting {
            onAgainRequested: root.solo.again()
            onBackRequested: root.back()
        }
    }

    HistorySheet {
        id: history
        solo: root.solo
        onClosed: trace.forceActiveFocus()
    }

    // --- the moments the model asks the screen to time -----------------------------------------------
    Connections {
        target: root.solo

        function onStateChanged() {
            switch (root.solo.state) {
            case "shaking":
                scene.play({ rows: root.solo.rows, columns: root.solo.columns, tiles: root.solo.tiles },
                           root.solo.seed);
                break;
            case "box":
                scene.show(scene.closedBox());
                break;
            case "cleared":
                if (!Theme.reducedMotion) {
                    glowAnimation.restart();
                }
                clearedHold.restart();
                break;
            }
        }
    }

    // solo.cleared lasts about three seconds and then follows by itself, because
    // the board has no word left to find. Nothing is a countdown to the child.
    Timer {
        id: clearedHold
        interval: Theme.clearedHold
        onTriggered: root.solo.celebrated()
    }
}
