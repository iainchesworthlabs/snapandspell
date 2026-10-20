pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver
import WordJewelsSolver.Scene

// Solo Play, Mode B. The closed jewel box and the flat board are 2D; the 3D
// scene is used only for the shake and the drop. A persistent app bar holds the
// way back and the grown-up entry, so no control lies over the board. The
// panel is one full-height column: the words, the hint, the board's clock and
// the Clear and Done actions all live in it, and nothing of the board moves
// when a path starts.
FocusScope {
    id: root

    required property var solo
    property bool shakeEnabled: true
    property var motionSource: null

    signal homeRequested
    signal parentRequested

    readonly property string soloState: solo.state
    readonly property bool boxShown: soloState === "box"
    readonly property bool shaking: soloState === "shaking" || soloState === "dropping"
    readonly property bool boardShown: soloState === "playing" || soloState === "word-found" || soloState === "cleared"
    // A board can be ended by the child before every dictionary word is found.
    // The summary is the same card, worded for a finish rather than a clear.
    property bool finishSummary: false
    // The round ended because the count-down reached zero (not the child's
    // Finish board): the summary says so.
    property bool finishedByClock: false
    readonly property bool summaryShown: soloState === "cleared" || root.finishSummary
    readonly property bool settling: settlingNow
    property bool settlingNow: false
    readonly property bool motionLive: motionSource !== null && motionSource.available
    readonly property bool motionNeeded: motionSource !== null && root.enabled && root.visible
                                        && root.shakeEnabled
                                        && (root.boxShown || root.shaking)
    readonly property bool mirrored: Theme.rightToLeft

    LayoutMirroring.enabled: root.mirrored
    LayoutMirroring.childrenInherit: true

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

    function boardForScene() {
        return {
            rows: root.solo.rows,
            columns: root.solo.columns,
            tiles: root.solo.tiles,
            faces: root.solo.faces,
            up: root.solo.up
        };
    }
    function closedBox() {
        return { rows: root.solo.boardSize, columns: root.solo.boardSize, tiles: [] };
    }

    // The board layout: the board in the leading two thirds and the panel
    // column in the trailing third of the whole stage, to the window's bottom
    // edge. The panel keeps a place for its own actions, so the board and the
    // panel never move when a path starts.
    SoloLayout {
        id: rule
        width: root.width
        height: root.height - Theme.appBarHeight
        boardSize: root.solo.boardSize
        rightToLeft: root.mirrored
    }

    // The stage already sits below the app bar, so the layout's rectangles are
    // used as they are.
    readonly property rect boardArea: Qt.rect(rule.boardArea.x, rule.boardArea.y,
                                              rule.boardArea.width, rule.boardArea.height)
    readonly property rect panelArea: Qt.rect(rule.panelArea.x, rule.panelArea.y,
                                              rule.panelArea.width, rule.panelArea.height)

    function relayout() {
        if (solo.rows > 0) {
            root.solo.setGeometry(rule.tile, rule.gap);
        }
    }
    Component.onCompleted: {
        root.relayout();
        syncMotion();
        // The scene is made lazily (the sceneLoader below) and is given the
        // closed box when it is ready: the 2D box is what the child sees.
    }
    onWidthChanged: resizeTimer.restart()
    onHeightChanged: resizeTimer.restart()
    Timer {
        id: resizeTimer
        interval: 50
        onTriggered: root.relayout()
    }

    property var pendingBoard: null
    property int pendingSeed: 0
    property string feedbackText: ""
    property string feedbackDetail: ""
    property bool feedbackWord: false
    // The scene, typed for the rest of the screen; null while the loader is
    // still making it. Once made it is kept for the run (sceneMade).
    readonly property BoardScene scene: sceneLoader.item
    property bool sceneMade: false

    function startPendingPlay() {
        if (pendingBoard === null || scene === null) {
            // The board stays waiting: either nothing shook yet, or the scene
            // is still being made and its onLoaded takes over.
            return;
        }
        const board = pendingBoard;
        pendingBoard = null;
        if (root.soloState === "shaking") {
            scene.play(board, pendingSeed);
        }
    }

    Timer {
        id: playStart
        interval: 16
        onTriggered: root.startPendingPlay()
    }

    Timer {
        id: feedbackTimer
        interval: 1800
        onTriggered: {
            root.feedbackText = "";
            root.feedbackDetail = "";
        }
    }

    // The 3D dice and the flat dice are one object seen twice: when the scene
    // says it has settled, let the 3D board fade out under the flat one instead
    // of cutting between two pictures.
    Timer {
        id: settleFade
        interval: Theme.screen
        onTriggered: root.settlingNow = false
    }

    function currentWord() {
        var letters = [];
        for (var i = 0; i < solo.path.length; ++i) {
            const tile = solo.tiles[solo.path[i]];
            letters.push(tile === "QU" ? "Qu" : tile);
        }
        return letters.join(" · ");
    }

    function finishBoard(byClock) {
        if (root.soloState !== "playing" && root.soloState !== "word-found") {
            return;
        }
        root.finishedByClock = byClock === true;
        root.finishSummary = true;
    }

    function leaveFinishSummary() {
        root.finishSummary = false;
        root.finishedByClock = false;
        if (root.soloState === "word-found") {
            root.solo.wordShown();
        }
        root.solo.lidOn();
    }

    // Earlier boards over whatever this screen shows: the way Home's "My
    // boards" opens the history without losing the board in front of the child.
    function openHistory() {
        history.open = true;
    }

    function back() {
        if (root.finishSummary) {
            root.leaveFinishSummary();
            root.homeRequested();
            return;
        }
        if (history.open) {
            history.close();
            return;
        }
        switch (solo.state) {
        case "box":
            root.homeRequested();
            break;
        case "resting":
            root.solo.again();
            root.homeRequested();
            break;
        case "cleared":
            root.solo.celebrated();
            root.solo.again();
            root.homeRequested();
            break;
        default:
            // Playing, a word being shown, the shake or the drop: the round is
            // left in progress. Home, and the next visit, finds it as it was -
            // the session is saved on every find, so nothing is lost.
            root.homeRequested();
            break;
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Space && root.boxShown) {
            root.solo.shake();
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    AppBar {
        id: appBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        showBack: true
        showWordmark: false
        showParent: true
        title: root.boxShown ? qsTr("Solo Play")
               : root.finishSummary ? qsTr("Board finished")
               : (root.soloState === "cleared" ? qsTr("All words found") : qsTr("Find a word"))
        onBack: root.back()
        onParentRequested: root.parentRequested()
    }

    // --- the stage ------------------------------------------------------------------
    Item {
        id: stage
        anchors.top: appBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Rectangle {
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            color: Theme.boardArea
        }

        // The closed box, before the shake. Its compartments change with the
        // board size, so the picture on the left answers the choice on the
        // right. The caption is aligned to the box, not to the whole window.
        JewelBox {
            id: boxArt
            readonly property real boxSide: Math.min(root.boardArea.width * 0.66,
                                                     root.boardArea.height * 0.64, 560)
            x: root.boardArea.x + (root.boardArea.width - width) / 2
            y: root.boardArea.y + (root.boardArea.height - height) / 2 - 28
            width: boxSide
            height: boxSide * 0.80
            closed: true
            asleep: false
            showDice: false
            boardSize: root.solo.boardSize
            visible: root.boxShown
            opacity: root.boxShown ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.base } }

            DragHandler {
                id: boxDrag
                target: null
                enabled: root.boxShown && !root.motionLive
                property real startX: 0
                onActiveChanged: {
                    if (active) {
                        startX = centroid.position.x;
                    }
                }
                onCentroidChanged: {
                    if (active && Math.abs(centroid.position.x - startX) > 42) {
                        startX = centroid.position.x;
                        root.solo.shake();
                    }
                }
            }
        }

        Text {
            id: boxCaption
            visible: root.boxShown
            width: Math.min(boxArt.width * 1.2, root.boardArea.width)
            x: boxArt.x + (boxArt.width - width) / 2
            y: boxArt.y + boxArt.height + Theme.space4
            horizontalAlignment: Text.AlignHCenter
            text: SoloWording.shakeToOpen
            font.family: Theme.fontFamily
            font.pixelSize: Theme.headingSize
            color: Theme.inkSecondary
        }

        // What is happening while the box shakes, as a quiet caption at the
        // foot of the board's area (the strip the action bar once reserved is
        // gone).
        Text {
            id: shakingCaption
            visible: root.shaking
            width: root.boardArea.width - 2 * Theme.space8
            x: root.boardArea.x + (root.boardArea.width - width) / 2
            y: root.boardArea.y + root.boardArea.height - Theme.space12
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Shaking the box…")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.headingSize
            color: Theme.inkSecondary
        }

        // The 3D theatre: the shake and the drop only. It is made by a Loader,
        // asynchronously and only once Solo Play is entered (the closed box),
        // so its first creation - the scene graph, the light probe and the
        // materials, whose shaders are made and cached on first use - happens
        // while the child reads the box instead of blocking the first shake.
        // It then lives for the rest of the run, so later shakes pay nothing.
        Loader {
            id: sceneLoader
            objectName: "sceneLoader"
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            // The full board rectangle is what aligns the settle; clipping keeps
            // the scene's overdraw out of the app bar above and the panel beside.
            clip: true
            asynchronous: true
            // The scene is made once Solo Play is shown - the closed box, not
            // the first shake - and kept for the rest of the run, so later
            // shakes pay nothing. It is not gated on the derived state
            // booleans: their re-evaluation after a state change is not
            // atomic, and the loader once saw a false in between, destroying
            // the scene mid-shake (measured in the box suites).
            active: root.sceneMade || (root.enabled && root.visible)
            visible: root.shaking || root.settling
            opacity: root.shaking ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.base } }

            sourceComponent: boardScene

            onLoaded: {
                root.sceneMade = true;
                if (root.shaking) {
                    // The shake began while the scene was still being made:
                    // hand it the board that has been waiting for it.
                    if (root.pendingBoard !== null) {
                        root.scene.prepare(root.pendingBoard);
                        root.startPendingPlay();
                    }
                } else if (root.boxShown) {
                    // The closed box: the scene keeps its seam and its own
                    // state, so the shake can replace the box without
                    // re-creating anything.
                    root.scene.show(root.closedBox());
                }
            }
        }
        Component {
            id: boardScene
            BoardScene {
                // Exactly the flat board's rectangle, with no inset: the settled
                // 3D dice and the flat dice have to occupy the same pixels for
                // the cross-fade to read as one object changing surface.
                reducedMotion: Theme.reducedMotion
                LayoutMirroring.enabled: false

                onSettled: root.solo.sceneSettled()
                onLanded: (index, pitch) => Feedback.cue("tile-landed", String(pitch))
                onRattled: Feedback.cue("rattle", "")
            }
        }

        // The flat board the child reads and traces.
        FlatBoard {
            id: flatBoard
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            solo: root.solo
            boardRect: rule.boardRect
            tileSize: rule.tile
            tileGap: rule.gap
            visible: root.boardShown || root.settling
            opacity: root.boardShown ? 1 : 0
            scale: root.boardShown ? 1 : 0.992
            transformOrigin: Item.Center
            Behavior on opacity { NumberAnimation { duration: Theme.screen; easing.type: Easing.OutCubic } }
            Behavior on scale { NumberAnimation { duration: Theme.turn; easing.type: Easing.OutCubic } }
            // While the finish summary covers the board, the board neither
            // takes the keyboard nor speaks to a screen reader: the summary's
            // own focus stays in its card, where the way on is.
            focus: root.boardShown && !root.summaryShown
            Accessible.ignored: root.summaryShown
            onVisibleChanged: if (root.boardShown && !root.summaryShown) forceActiveFocus()
        }

        // --- the panel ------------------------------------------------------------
        // One column from the app bar to the window's bottom edge. Its own
        // actions (Clear, Done) and the board's clock live at its foot, so the
        // bottom right of the window is the actions' home and the column fills
        // the full height.
        Item {
            id: panelSlot
            x: root.panelArea.x
            y: root.panelArea.y
            width: root.panelArea.width
            height: root.panelArea.height
            clip: true

            SoloBox {
                id: boxControls
                width: parent.width
                height: parent.height
                solo: root.solo
                layoutRule: rule
                wide: rule.wide
                opacity: root.boxShown ? 1 : 0
                visible: opacity > 0
                enabled: root.boxShown
                onShakeRequested: root.solo.shake()
                Behavior on opacity { NumberAnimation { duration: Theme.turn } }
            }

            PlayingPanel {
                id: panel
                solo: root.solo
                wide: rule.wide
                width: parent.width
                height: parent.height
                statusText: root.feedbackText
                statusDetail: root.feedbackDetail
                statusWord: root.feedbackWord
                property real shown: root.boardShown ? 1 : 0
                Behavior on shown { NumberAnimation { duration: Theme.turn; easing.type: Easing.OutCubic } }
                opacity: shown > 0 ? 1 : 0
                visible: shown > 0
                x: rule.wide && !Theme.reducedMotion ? (1 - shown) * width : 0
                y: !rule.wide && !Theme.reducedMotion ? (1 - shown) * height : 0
                onFinishRequested: root.finishBoard()
            }
        }

        // --- the clear summary ----------------------------------------------------
        // The board is over, one way or the other: the words are safe, and the
        // card says what was earned and offers the way on. The words scroll in
        // their own card, so the actions at the foot never move; the primary
        // action takes the focus and the reward is spoken.
        Item {
            id: summary
            anchors.fill: parent
            visible: root.summaryShown
            z: 30

            onVisibleChanged: {
                if (summary.visible) {
                    playAgain.forceActiveFocus();
                    Feedback.cue("speak", summaryTitle.text + ". "
                                              + SoloWording.summaryStars(root.solo.boardStars)
                                              + " and " + SoloWording.summaryPoints(root.solo.boardPoints));
                }
            }

            Rectangle {
                anchors.fill: parent
                color: Theme.scrim
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 2 * Theme.space8, 560)
                height: Math.min(parent.height - 2 * Theme.space8, 560)
                radius: Theme.radiusLarge
                color: Theme.surface
                border.width: Theme.hair
                border.color: Theme.line
                clip: true

                Accessible.role: Accessible.Pane
                Accessible.name: summaryTitle.text

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.space6
                    spacing: Theme.space3

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space1

                        Text {
                            id: summaryTitle
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: root.finishSummary
                                  ? SoloWording.finishTitle
                                  : root.solo.found.length === 1
                                    ? qsTr("You found the word!")
                                    : qsTr("You found all %1 words!").arg(root.solo.found.length)
                            font.family: Theme.uiFont
                            font.pixelSize: Theme.titleSize
                            font.weight: Font.ExtraBold
                            color: Theme.ink
                            wrapMode: Text.WordWrap
                        }

                        // The reward, front and centre: the stars (which feed
                        // the collection) and the points, large enough to read
                        // at arm's length.
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space2

                            Jewel {
                                Layout.preferredWidth: 30
                                Layout.preferredHeight: 30
                                tier: "power"
                                color: Theme.gold
                            }
                            Text {
                                text: SoloWording.summaryStars(root.solo.boardStars)
                                font.family: Theme.uiFont
                                font.pixelSize: Theme.headingSize
                                font.weight: Font.DemiBold
                                color: Theme.goldInk
                            }
                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                            }
                            Text {
                                text: SoloWording.summaryPoints(root.solo.boardPoints)
                                font.family: Theme.monoFont
                                font.pixelSize: Theme.headingSize
                                color: Theme.ink
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: root.finishSummary
                              ? SoloWording.finishBody
                              : qsTr("Look at the jewels you collected.")
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.solo.clockMode === "up"
                        horizontalAlignment: Text.AlignHCenter
                        text: SoloWording.playedFor(root.solo.clockText)
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.labelSize
                        color: Theme.inkSecondary
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.finishedByClock
                        horizontalAlignment: Text.AlignHCenter
                        text: SoloWording.timeRanOut
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                        wrapMode: Text.WordWrap
                    }

                    // The words, one jewel each, in their own scroll: the card
                    // stays one height however many were found, and the actions
                    // below it stay put.
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: Theme.radius
                        color: Theme.card
                        border.width: 1
                        border.color: Theme.line

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.space3
                            spacing: Theme.space2

                            Text {
                                Layout.fillWidth: true
                                text: SoloWording.yourWordsCount(root.solo.found.length)
                                font.family: Theme.monoFont
                                font.pixelSize: Theme.smallSize
                                color: Theme.inkSecondary
                            }

                            ListView {
                                id: summaryWords
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: root.solo.found
                                spacing: 2

                                delegate: Item {
                                    id: summaryRow
                                    required property int index
                                    required property var modelData
                                    width: summaryWords.width
                                    height: 42

                                    Accessible.role: Accessible.ListItem
                                    Accessible.name: SoloWording.found(summaryRow.modelData.word)

                                    Jewel {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 24
                                        height: 24
                                        tier: summaryRow.modelData.tier
                                        colorIndex: summaryRow.index
                                    }
                                    Text {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 36
                                        anchors.right: pointsText.left
                                        anchors.rightMargin: Theme.space2
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: summaryRow.modelData.word
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.wordSize
                                        color: Theme.ink
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        id: pointsText
                                        anchors.right: parent.right
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: qsTr("+%1").arg(summaryRow.modelData.points)
                                        font.family: Theme.monoFont
                                        font.pixelSize: Theme.monoSize
                                        color: Theme.goldInk
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.solo.found.length === 0
                                horizontalAlignment: Text.AlignHCenter
                                text: SoloWording.summaryNoWords
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.bodySize
                                color: Theme.inkSecondary
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // The way on: Play again leads, Home is the quiet way out.
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space3

                        AppButton {
                            id: playAgain
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.targetPrimary
                            Layout.alignment: Qt.AlignVCenter
                            variant: "primary"
                            size: "primary"
                            label: qsTr("Play again")
                            accessibleName: qsTr("Play again")
                            iconName: "again"
                            // Tab stays inside the card: the two ways on are
                            // each other's next stop, and the board behind the
                            // scrim is not focusable (FlatBoard above).
                            KeyNavigation.tab: homeButton
                            KeyNavigation.backtab: homeButton
                            onClicked: {
                                if (root.finishSummary) {
                                    root.leaveFinishSummary();
                                    root.solo.shake();
                                } else {
                                    root.solo.celebrated();
                                    root.solo.again();
                                }
                            }
                        }
                        AppButton {
                            id: homeButton
                            Layout.fillWidth: true
                            Layout.preferredHeight: Theme.targetChild
                            Layout.alignment: Qt.AlignVCenter
                            variant: "secondary"
                            size: "child"
                            label: qsTr("Home")
                            accessibleName: qsTr("Home")
                            KeyNavigation.tab: playAgain
                            KeyNavigation.backtab: playAgain
                            onClicked: {
                                if (root.finishSummary) {
                                    root.leaveFinishSummary();
                                } else {
                                    root.solo.celebrated();
                                    root.solo.again();
                                }
                                root.homeRequested();
                            }
                        }
                    }
                }
            }
        }
    }

    // --- resting --------------------------------------------------------------------
    Loader {
        anchors.top: appBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        active: root.soloState === "resting"
        asynchronous: true
        sourceComponent: SoloResting {
            onAgainRequested: root.solo.again()
            onBackRequested: {
                root.solo.again();
                root.homeRequested();
            }
        }
    }

    HistorySheet {
        id: history
        solo: root.solo
        onClosed: flatBoard.forceActiveFocus()
    }

    // The board's clock ticks only while this screen is the one on show and a
    // round is in play. The model owns the seconds; the screen owns the "now".
    Binding {
        target: root.solo
        property: "clockActive"
        value: root.enabled && root.visible
               && (root.soloState === "playing" || root.soloState === "word-found")
               && !root.finishSummary
    }

    // --- the model's moments ---------------------------------------------------------
    Connections {
        target: root.solo

        function onStateChanged() {
            switch (root.solo.state) {
            case "shaking": {
                settleFade.stop();
                root.settlingNow = false;
                const board = root.boardForScene();
                if (root.scene !== null) {
                    root.scene.prepare(board);
                }
                root.pendingBoard = board;
                root.pendingSeed = root.solo.seed;
                playStart.restart();
                root.feedbackText = "";
                root.feedbackDetail = "";
                break;
            }
            case "box":
                playStart.stop();
                settleFade.stop();
                root.settlingNow = false;
                root.pendingBoard = null;
                root.finishSummary = false;
                root.finishedByClock = false;
                root.feedbackText = "";
                root.feedbackDetail = "";
                break;
            case "playing":
                // The scene has settled. Keep it for one cross-fade, under the
                // flat board that the child will trace.
                root.relayout();
                root.settlingNow = true;
                settleFade.restart();
                break;
            case "cleared":
                settleFade.stop();
                root.settlingNow = false;
                root.feedbackText = "";
                root.feedbackDetail = "";
                break;
            }
        }

        function onClockExpired() {
            // A count-down board's time ran out: the round ends as Finish
            // board ends it, with everything found kept, and the summary
            // says the time ran out.
            root.finishBoard(true);
        }

        function onTraced(kind, path, word, tier, stars, again, newBest, cleared) {
            if (kind === "word") {
                root.feedbackWord = true;
                root.feedbackText = word.toUpperCase() + "!";
                root.feedbackDetail = again ? qsTr("You found this one already.")
                                            : qsTr("You found a word. +%1 points").arg(
                                                  root.solo.found.length > 0
                                                  ? root.solo.found[root.solo.found.length - 1].points : 0);
            } else if (kind === "tick") {
                root.feedbackWord = false;
                root.feedbackText = qsTr("That's a real word.");
                root.feedbackDetail = qsTr("It is not on this board.");
            } else {
                root.feedbackWord = false;
                root.feedbackText = qsTr("Not this time.");
                root.feedbackDetail = qsTr("Try another word.");
            }
            feedbackTimer.restart();
        }
    }

    Connections {
        target: root.motionSource
        function onSample(ax, ay, az, wx, wy, wz) {
            if (root.motionLive && (root.motionNeeded || root.shaking)
                    && root.scene !== null) {
                root.scene.pushMotion(ax, ay, az, wx, wy, wz);
            }
        }
        function onShakeDetected() {
            if (root.shakeEnabled && root.boxShown) {
                root.solo.shake();
            }
        }
    }
}
