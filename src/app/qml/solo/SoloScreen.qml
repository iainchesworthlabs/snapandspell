pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver
import WordJewelsSolver.Scene

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
    // Whether shaking the device rolls the dice (Settings::shake). It gates the
    // drag that stands in for the accelerometer, never the Shake button or
    // Space, which are always there (docs/design/application.md, "Accessibility":
    // the Shake button, and shake detection can be switched off).
    property bool shakeEnabled: true
    // The device's motion, made by main.cpp and handed here by Main.qml. It is
    // null in a stub shell or a test suite, and on these desktops its available
    // is false: the drag below then stands in for the accelerometer.
    property var motionSource: null

    signal homeRequested
    signal parentRequested

    readonly property string soloState: solo.state
    // Qt Sensors is reading a real device now: its samples drive the scene and
    // a real shake rolls the box, in place of the drag.
    readonly property bool motionLive: motionSource !== null && motionSource.available
    // The sensors run only while the box or the falling dice need them, the
    // screen is the one showing, and the parent has Shake on. Leaving the
    // screen (or the box) stops them. On a desktop they are started too and
    // send nothing, so motionLive stays false and the drag stands in.
    readonly property bool motionNeeded: motionSource !== null && root.enabled && root.visible
                                        && root.shakeEnabled
                                        && (root.soloState === "box"
                                            || root.soloState === "shaking"
                                            || root.soloState === "dropping")

    function syncMotion() {
        if (motionSource === null) {
            return;
        }
        motionSource.shakeEnabled = root.shakeEnabled;
        motionSource.active = root.motionNeeded;
    }

    onMotionSourceChanged: syncMotion()
    onMotionNeededChanged: syncMotion()
    onShakeEnabledChanged: syncMotion()
    readonly property real barOffset: Theme.barHeight
    // Hebrew and Yiddish: the window mirrors (Main.qml sets the direction), so
    // the panel takes the leading side and the board the trailing one. The board
    // itself does not mirror: its tiles stay in reading order.
    readonly property bool mirrored: Theme.rightToLeft

    LayoutMirroring.enabled: root.mirrored
    LayoutMirroring.childrenInherit: true

    // The board as the scene is given it (docs/design/shake-prototype.md, "The
    // interface"): the tiles, and the six faces of the die on each with the one
    // that is up. The faces are empty when the board was not rolled from dice;
    // the scene then draws the letter on the face that is up.
    function boardForScene() {
        return {
            rows: root.solo.rows,
            columns: root.solo.columns,
            tiles: root.solo.tiles,
            faces: root.solo.faces,
            up: root.solo.up
        };
    }

    // The box with its lid on: no tiles, so the scene draws no dice.
    function closedBox() {
        return { rows: root.solo.boardSize, columns: root.solo.boardSize, tiles: [] };
    }

    // The board the scene should show at rest: the board in play, or the closed
    // box. Used at start-up and when the window changes shape: the 3D script is
    // made for one tile size, so a new layout is a new settled board rather than
    // a re-posed one (docs/architecture.md: rotating recomputes the geometry).
    function restingBoard() {
        if (root.soloState === "playing" || root.soloState === "word-found"
                || root.soloState === "cleared") {
            return boardForScene();
        }
        return closedBox();
    }

    property bool ready: false

    function relayout() {
        if (!ready || width <= 0 || height <= 0) {
            return;
        }
        // A play in progress owns the scene until it settles; the resize then
        // shows the board it left.
        if (root.soloState === "shaking" || root.soloState === "dropping") {
            return;
        }
        const board = restingBoard();
        scene.prepare(board);
        scene.show(board);
        root.solo.setGeometry(scene.tileSize, scene.tileGap);
    }

    Component.onCompleted: {
        // The scene's first layout: its prepare and show of the box, and the
        // geometry it hands the game. The component's own creation (the View3D
        // and its dice) is inside app.load_qml, where the engine builds it.
        Profiler.begin("app.scene_create");
        ready = true;
        relayout();
        syncMotion();
        Profiler.end();
    }

    onWidthChanged: if (ready) { resizeTimer.restart() }
    onHeightChanged: if (ready) { resizeTimer.restart() }

    // A window drag changes the width many times a frame; the scene is shown
    // once it has stopped for a moment.
    Timer {
        id: resizeTimer

        interval: 50
        repeat: false
        onTriggered: root.relayout()
    }

    // The board to start once prepare() has had its frame, and its seed.
    property var pendingBoard: null
    property var pendingSeed: 0

    function startPendingPlay() {
        if (pendingBoard === null) {
            return;
        }
        const board = pendingBoard;
        pendingBoard = null;
        if (root.soloState === "shaking") {
            scene.play(board, pendingSeed);
        }
    }

    // The shake and the drop are the scene's; its first frame carries the board's
    // faces if they were drawn a frame ahead (prepare), so the play starts on the
    // frame after the press rather than in the frame that makes the atlas.
    Timer {
        id: playStart

        interval: 16
        repeat: false
        onTriggered: root.startPendingPlay()
    }

    // --- the device's motion -----------------------------------------------------
    // On a tablet Qt Sensors (MotionSource) sends a hundred samples a second:
    // the sample drives the scene's dice and a real shake rolls the box, exactly
    // as the drag here does on a desktop. The drag is the stand-in kept for a
    // platform with no accelerometer (docs/design/shake-prototype.md, "The
    // drag"): the pointer's path is differentiated twice into an acceleration,
    // the board taken as 16 cm across, and the arm's up and down added. A drag
    // that reads as a shake starts a play, as a shaken device would, unless the
    // parent switched shaking off; the Shake button and Space are always there.
    Connections {
        target: root.motionSource

        function onSample(ax, ay, az, wx, wy, wz) {
            if (root.motionLive) {
                scene.pushMotion(ax, ay, az, wx, wy, wz);
            }
        }

        function onShakeDetected() {
            if (root.shakeEnabled && root.soloState === "box" && !scene.busy) {
                root.solo.shake();
            }
        }
    }

    QtObject {
        id: pointer

        property bool down: false
        property double at: 0
        property real x: 0
        property real y: 0
        property real vx: 0
        property real vy: 0
    }

    function clampTo(value, limit) {
        return Math.max(-limit, Math.min(limit, value));
    }

    // One sample of the device, from the pointer at (px, py) in the scene.
    function pointerSample(px, py) {
        const now = Date.now();
        const dt = Math.max(0.002, (now - pointer.at) / 1000);
        const vx = (px - pointer.x) / dt;
        const vy = (py - pointer.y) / dt;
        const ax = (vx - pointer.vx) / dt;
        const ay = (vy - pointer.vy) / dt;
        pointer.x = px;
        pointer.y = py;
        pointer.vx = vx;
        pointer.vy = vy;
        pointer.at = now;

        const toG = 0.16 / Math.max(1, scene.boardRect.width) / 9.81;
        const handX = clampTo(ax * toG, 5);
        const handY = clampTo(-ay * toG, 5);
        const roll = clampTo((px / Math.max(1, scene.width) - 0.5) * 2, 1) * 0.14;
        const pitch = clampTo((0.5 - py / Math.max(1, scene.height)) * 2, 1) * 0.14;
        const sx = -Math.sin(roll) + handX;
        const sy = Math.sin(pitch) + handY;
        // A mouse cannot lift the device. A hand that shakes one sideways also
        // moves it up and down, in time with the strokes, so that is added.
        const lift = clampTo(0.8 * (handX + handY), 4);
        const sz = Math.cos(roll) * Math.cos(pitch) + lift;
        const wx = clampTo(-vy * 0.001, 4);
        const wy = clampTo(vx * 0.001, 4);
        scene.pushMotion(sx, sy, sz, wx, wy, 0);

        if (root.shakeEnabled && root.soloState === "box" && !scene.busy
                && Math.hypot(handX, handY, lift) > 0.8) {
            pointer.down = false;
            root.solo.shake();
        }
    }

    // While the hand is down, or the dice are moving, the stand-in keeps sending
    // samples (a hundred a second), so a hand that has stopped reads as stopped.
    // A real device's samples are already a stream: the stand-in is silent then.
    Timer {
        interval: 10
        repeat: true
        running: !root.motionLive
                 && (root.soloState === "shaking" || root.soloState === "dropping"
                     || pointer.down)
        onTriggered: {
            if (Date.now() - pointer.at > 20) {
                root.pointerSample(pointer.x, pointer.y);
            }
        }
    }

    SoloLayout {
        id: rule
        width: root.width
        height: root.height - root.barOffset
        boardSize: root.solo.boardSize
        // The split mirrors with the language; the tiles do not.
        rightToLeft: root.mirrored
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

        // The 3D board scene (WordJewelsSolver.Scene): one component, made once
        // for the run behind home (Main.qml keeps this screen alive), so the
        // View3D is not created on the Shake press. The item is left unclipped:
        // the scene draws a little past it (overdraw), as a die at the edge of
        // the board and the box's rim need. Its dice never mirror: the board is
        // the same English grid in every language.
        BoardScene {
            id: scene
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            reducedMotion: Theme.reducedMotion
            LayoutMirroring.enabled: false

            onSettled: root.solo.sceneSettled()
            onLanded: (index, pitch) => Feedback.cue("tile-landed", String(pitch))
            onRattled: Feedback.cue("rattle", "")
            onTileSizeChanged: root.solo.setGeometry(tileSize, tileGap)
            onTileGapChanged: root.solo.setGeometry(tileSize, tileGap)

            // The drag that stands in for the accelerometer, on the box itself.
            // A device whose sensors are there does not use it: the samples come
            // from MotionSource above.
            DragHandler {
                id: boxDrag

                target: null
                enabled: root.soloState === "box" && !root.motionLive
                onCentroidChanged: {
                    if (active) {
                        root.pointerSample(centroid.position.x, centroid.position.y);
                    }
                }
                onActiveChanged: {
                    pointer.down = active;
                    if (active) {
                        pointer.x = centroid.position.x;
                        pointer.y = centroid.position.y;
                        pointer.vx = 0;
                        pointer.vy = 0;
                        pointer.at = Date.now();
                    }
                }
            }
        }

        // The pointer, the keyboard and the screen reader's tiles, over the scene.
        // Its coordinates are the board's own and are not mirrored.
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
            LayoutMirroring.enabled: false
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
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: Theme.space2
            anchors.topMargin: Theme.space2 / 2
            iconName: "back"
            accessibleName: SoloWording.back
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
                accessibleName: SoloWording.earlierBoards
                onClicked: history.open = true
            }
            AppButton {
                visible: Navigation.parentAvailable
                iconName: "parent"
                accessibleName: SoloWording.forGrownUps
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
            case "shaking": {
                // The board is dealt before anything moves. Its faces are drawn
                // into the spare atlas now, and play() starts on the next frame,
                // when that texture is already there.
                const board = root.boardForScene();
                scene.prepare(board);
                root.pendingBoard = board;
                root.pendingSeed = root.solo.seed;
                playStart.restart();
                break;
            }
            case "box":
                playStart.stop();
                root.pendingBoard = null;
                scene.show(root.closedBox());
                break;
            case "cleared":
                // The celebration's synchronous work: the one slow glow over the
                // board (not with reduced motion) and the hold that ends it. What
                // the glow animates afterwards is the animation system's, and shows
                // in the frames.
                Profiler.begin("scene.celebrate");
                if (!Theme.reducedMotion) {
                    glowAnimation.restart();
                }
                clearedHold.restart();
                Profiler.end();
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
