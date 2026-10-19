pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver
import WordJewelsSolver.Scene

// Solo Play, Mode B. The closed jewel box and the flat board are 2D; the 3D
// scene is used only for the shake and the drop. A persistent app bar holds the
// way back and the grown-up entry, so no control lies over the board.
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
    readonly property bool summaryShown: soloState === "cleared" || root.finishSummary
    readonly property bool actionShown: soloState === "playing" || soloState === "word-found"
    // The strip is reserved for the whole shake and landing, so the 3D theatre
    // and the flat board keep one geometry through the moment the dice settle.
    readonly property bool actionBarShown: actionShown || shaking
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

    // The board layout. The action bar takes its own strip out of the play
    // height, so the board and panel never move when a path starts.
    SoloLayout {
        id: rule
        width: root.width
        height: root.height - Theme.appBarHeight - (root.actionBarShown ? Theme.actionBarHeight : 0)
        boardSize: root.solo.boardSize
        rightToLeft: root.mirrored
    }

    // The stage already sits below the app bar, so the layout's rectangles are
    // used as they are; adding the bar offset again would push the board and the
    // panel under the action bar.
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
        // Give the scene the closed box once, even though the 2D box is what
        // the child sees: the scene keeps its seam and its own state, and the
        // shake can replace that box without re-creating anything.
        if (root.boxShown) {
            scene.show(root.closedBox());
        }
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

    function finishBoard() {
        if (root.soloState !== "playing" && root.soloState !== "word-found") {
            return;
        }
        root.finishSummary = true;
    }

    function leaveFinishSummary() {
        root.finishSummary = false;
        if (root.soloState === "word-found") {
            root.solo.wordShown();
        }
        root.solo.lidOn();
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
        case "word-found":
            root.solo.wordShown();
            root.solo.lidOn();
            break;
        default:
            root.solo.lidOn();
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

        // The 3D theatre: the shake and the drop only.
        BoardScene {
            id: scene
            // Exactly the flat board's rectangle, with no inset: the settled 3D
            // dice and the flat dice have to occupy the same pixels for the
            // cross-fade to read as one object changing surface. The scene's
            // own overdraw may paint past this rect; the panel is drawn after
            // it, so nothing spills into the panel.
            x: root.boardArea.x
            y: root.boardArea.y
            width: root.boardArea.width
            height: root.boardArea.height
            // The full board rectangle is what aligns the settle; clipping keeps
            // the scene's overdraw out of the app bar above and the panel beside.
            clip: true
            reducedMotion: Theme.reducedMotion
            LayoutMirroring.enabled: false
            visible: root.shaking || root.settling
            opacity: root.shaking ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.base } }

            onSettled: root.solo.sceneSettled()
            onLanded: (index, pitch) => Feedback.cue("tile-landed", String(pitch))
            onRattled: Feedback.cue("rattle", "")
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
            focus: root.boardShown
            onVisibleChanged: if (root.boardShown) forceActiveFocus()
        }

        // --- the panel ------------------------------------------------------------
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
                property real shown: root.boardShown ? 1 : 0
                Behavior on shown { NumberAnimation { duration: Theme.turn; easing.type: Easing.OutCubic } }
                opacity: shown > 0 ? 1 : 0
                visible: shown > 0
                x: rule.wide && !Theme.reducedMotion ? (1 - shown) * width : 0
                y: !rule.wide && !Theme.reducedMotion ? (1 - shown) * height : 0
                onHistoryRequested: history.open = true
                onFinishRequested: root.finishBoard()
            }
        }

        // --- the action bar -------------------------------------------------------
        // It belongs to the board, not to the whole window: Clear and Done line
        // up with the board's edge and never sit under the panel. The two
        // controls share a baseline and a gap, so Done is no longer a giant
        // green block floating beside a small Clear.
        Rectangle {
            visible: root.actionBarShown
            x: root.boardArea.x
            y: stage.height - Theme.actionBarHeight
            width: root.boardArea.width
            height: Theme.actionBarHeight
            color: Theme.surface
            border.width: 0
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: Theme.hair
                color: Theme.line
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.space6
                anchors.rightMargin: Theme.space6
                spacing: Theme.space4

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 2
                    Text {
                        Layout.fillWidth: true
                        // A found word is the big line; the praise is the small
                        // one. Keeping them apart is what stops "SIR — you found
                        // a word!" from eliding in the board's half of the bar.
                        text: root.shaking ? qsTr("Shaking the box…")
                                           : (root.feedbackText.length > 0 ? root.feedbackText
                                              : (root.solo.path.length > 0 ? root.currentWord()
                                                                           : qsTr("Make a word")))
                        font.family: root.feedbackWord || root.solo.path.length > 0
                                     ? Theme.fontFamily : Theme.uiFont
                        font.pixelSize: root.feedbackWord || root.solo.path.length > 0
                                        ? Theme.wordSize : Theme.headingSize
                        font.weight: Font.DemiBold
                        color: root.shaking ? Theme.inkSecondary
                                            : (root.feedbackText.length > 0 && !root.feedbackWord
                                               ? Theme.inkSecondary : Theme.ink)
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: !root.shaking
                                 && (root.feedbackDetail.length > 0 || root.solo.path.length > 0)
                        text: root.feedbackDetail.length > 0 ? root.feedbackDetail
                                                             : qsTr("Tap the last die again, or press Done")
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.smallSize
                        color: Theme.inkSecondary
                        elide: Text.ElideRight
                    }
                }

                AppButton {
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 64
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.actionShown
                    size: "child"
                    variant: "secondary"
                    label: qsTr("Clear")
                    accessibleName: qsTr("Clear the word")
                    enabled: root.solo.path.length > 0 || root.feedbackText.length > 0
                    onClicked: root.solo.clearPath()
                }
                AppButton {
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: Theme.targetPrimary
                    Layout.alignment: Qt.AlignVCenter
                    visible: root.actionShown
                    size: "primary"
                    variant: "primary"
                    iconName: "done"
                    label: qsTr("Done")
                    accessibleName: qsTr("Done")
                    enabled: root.solo.path.length > 0
                    onClicked: root.solo.done()
                }
            }
        }

        // --- the clear summary ----------------------------------------------------
        Item {
            id: summary
            anchors.fill: parent
            visible: root.summaryShown
            z: 30

            Rectangle {
                anchors.fill: parent
                color: Theme.scrim
            }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width - 2 * Theme.space8, 560)
                height: Math.min(parent.height - 2 * Theme.space8, 500)
                radius: Theme.radiusLarge
                color: Theme.surface
                border.width: Theme.hair
                border.color: Theme.line

                Column {
                    anchors.centerIn: parent
                    width: parent.width - 2 * Theme.space8
                    spacing: Theme.space4

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: root.finishSummary
                              ? SoloWording.finishTitle
                              : qsTr("You found all %1 words!").arg(root.solo.found.length)
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.titleSize
                        font.weight: Font.ExtraBold
                        color: Theme.ink
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("%1 points  ·  %2 stars").arg(root.solo.boardPoints).arg(root.solo.boardStars)
                        font.family: Theme.monoFont
                        font.pixelSize: Theme.labelSize
                        color: Theme.goldInk
                    }
                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: root.finishSummary
                              ? SoloWording.finishBody
                              : qsTr("Look at the jewels you collected.")
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        color: Theme.inkSecondary
                        wrapMode: Text.WordWrap
                    }
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: Theme.space2
                        Repeater {
                            model: root.solo.found
                            Jewel {
                                required property int index
                                required property var modelData
                                width: Theme.jewelPower
                                height: width
                                tier: modelData.tier
                                colorIndex: index
                            }
                        }
                    }
                    Flow {
                        width: parent.width
                        spacing: Theme.space2
                        Repeater {
                            model: root.solo.found
                            Text {
                                required property var modelData
                                text: modelData.word
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.headingSize
                                color: Theme.ink
                            }
                        }
                    }
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: Theme.space3
                        AppButton {
                            variant: "secondary"
                            size: "child"
                            label: qsTr("Home")
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
                        AppButton {
                            variant: "primary"
                            size: "primary"
                            label: qsTr("Play again")
                            iconName: "again"
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

    // --- the model's moments ---------------------------------------------------------
    Connections {
        target: root.solo

        function onStateChanged() {
            switch (root.solo.state) {
            case "shaking": {
                settleFade.stop();
                root.settlingNow = false;
                const board = root.boardForScene();
                scene.prepare(board);
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
            if (root.motionLive && (root.motionNeeded || root.shaking)) {
                scene.pushMotion(ax, ay, az, wx, wy, wz);
            }
        }
        function onShakeDetected() {
            if (root.shakeEnabled && root.boxShown) {
                root.solo.shake();
            }
        }
    }
}
