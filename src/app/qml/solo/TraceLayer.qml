pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes
import WordJewelsSolver
import WordJewelsSolver.Scene

// The 2D layer over the board scene: where the child traces and taps, and what
// the tiles and the path look like while they do. It sits exactly on the scene's
// tiles (boardRect, tileSize and tileGap are the scene's) and is 2D whatever the
// scene is, so the 3D scene's dice are opaque and this is what the screen reader
// and the keyboard reach (docs/design/application.md, "Input").
//
//   Trace and tap in order are both always on, with a loose tolerance. The touch
//   logic is the model's (PathTracer); this item only forwards the first pointer.
//   A lit tile has a thicker edge and a scale, not colour alone. The path is a
//   pale casing over a dark core, so it shows on the dice and on the tray.
//   Tiles that make one sound share a bar under them.
//   The start tile is a pulsing ring: shape and motion, not colour alone.
Item {
    id: root

    required property var solo
    // Where the board is, in this item's own coordinates (the scene's).
    property rect boardRect: Qt.rect(0, 0, 0, 0)
    property real tileSize: 1
    property real tileGap: 0

    readonly property int rows: solo.rows
    readonly property int columns: solo.columns
    readonly property real pitch: tileSize + tileGap
    readonly property bool accepting: solo.state === "playing"
    readonly property bool presenting: solo.state === "word-found"
    // The slack around the board in which a finger still counts as on it.
    readonly property real slack: tileSize * 0.35

    // Which tile has the keyboard's focus.
    property int focusIndex: 0
    property bool keyboardUsed: false

    activeFocusOnTab: true
    // The board is not mirrored: its coordinates are the board's own, and the
    // tiles keep their reading order in every language. The window's mirroring
    // stops here (and at the scene).
    LayoutMirroring.enabled: false
    LayoutMirroring.childrenInherit: true
    Accessible.role: Accessible.Table
    Accessible.name: SoloWording.board

    function tileX(index) {
        return boardRect.x + (index % columns) * pitch;
    }
    function tileY(index) {
        return boardRect.y + Math.floor(index / columns) * pitch;
    }
    function centre(index) {
        return Qt.point(tileX(index) + tileSize / 2, tileY(index) + tileSize / 2);
    }
    function points(indices) {
        return indices.map(i => centre(i));
    }

    // --- lit tiles --------------------------------------------------------------
    // Time into the word's flash, in ms: the tiles of the word light in the order
    // they were traced, 60 ms apart.
    property real flashMs: 0
    readonly property int flashLength: solo.litPath.length * Theme.tileFlashStep + Theme.tileFlashLength + Theme.wordHold

    // The path being made, copied from the model once per change. The tiles'
    // lit tests and the path's points read this one JS array, instead of asking
    // the model for a fresh list per read (the model builds its list per read,
    // and a 6x6 board reads it once a tile and once a path line per move: the
    // pointer's jank is those rebuilds).
    property var pathList: []
    // The path's points, computed once per change for both stroked lines.
    readonly property var pathPoints: root.points(root.pathList)
    // The fading path's points, computed once per fade for both lines.
    readonly property var ghostPoints: root.points(root.ghostPath)

    function isLit(index) {
        if (root.pathList.indexOf(index) >= 0) {
            return true;
        }
        const place = solo.litPath.indexOf(index);
        if (place < 0) {
            return false;
        }
        if (Theme.reducedMotion) {
            return true;
        }
        const from = place * Theme.tileFlashStep;
        return flashMs >= from && flashMs < from + Theme.tileFlashLength;
    }

    // A path that was not a word (or was a word that is not today's) fades out where
    // it was.
    property var ghostPath: []
    property string ghostKind: ""

    Connections {
        target: root.solo

        function onTraced(kind, path) {
            if (kind === "tick" || kind === "fade") {
                root.ghostPath = path;
                root.ghostKind = kind;
                ghostFade.restart();
            }
        }
        function onPathChanged() {
            // One copy per change: the tiles and the path lines read this.
            root.pathList = root.solo.path;
        }
        function onStateChanged() {
            if (root.solo.state === "word-found") {
                flash.restart();
            }
        }
        function onBoardChanged() {
            root.focusIndex = 0;
        }
    }

    NumberAnimation {
        id: flash
        target: root
        property: "flashMs"
        from: 0
        to: root.flashLength
        duration: Theme.reducedMotion ? Theme.wordHold : root.flashLength
        onFinished: root.solo.wordShown()
    }

    NumberAnimation {
        id: ghostFade
        target: ghost
        property: "opacity"
        from: 1
        to: 0
        duration: root.ghostKind === "tick" ? Theme.quick * 2 : Theme.pathFade
    }

    // --- the tiles ----------------------------------------------------------------
    Repeater {
        model: root.solo.tiles

        Item {
            id: cell
            required property int index
            required property string modelData

            x: root.tileX(index)
            y: root.tileY(index)
            width: root.tileSize
            height: root.tileSize

            readonly property bool dead: modelData === ""
            readonly property bool lit: root.isLit(index)

            // A lit tile is drawn again over the scene's die, amber, with a thick
            // edge and a larger scale.
            TileFace {
                visible: cell.lit && !cell.dead
                anchors.fill: parent
                letters: cell.modelData
                lit: true
            }

            // The tile the keyboard is on.
            Rectangle {
                visible: root.accepting && root.activeFocus && root.keyboardUsed && root.focusIndex === cell.index
                x: -4
                y: -4
                width: parent.width + 8
                height: parent.height + 8
                color: "transparent"
                border.width: Theme.rule
                border.color: Theme.pathCasing
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -Theme.rule
                    color: "transparent"
                    border.width: Theme.rule
                    border.color: Theme.pathCore
                }
            }

            // "A, row 2, column 3": named by its letter and its place.
            Accessible.role: cell.dead ? Accessible.Graphic : Accessible.Button
            Accessible.name: cell.dead
                ? SoloWording.jewelName(Math.floor(cell.index / root.columns) + 1, cell.index % root.columns + 1)
                : SoloWording.tileName(cell.modelData === "QU" ? "Qu" : cell.modelData,
                                       Math.floor(cell.index / root.columns) + 1,
                                       cell.index % root.columns + 1)
            Accessible.checked: cell.lit
            Accessible.onPressAction: root.solo.tapTile(cell.index)
        }
    }

    // --- the start tile: a pulsing ring ----------------------------------------------
    Rectangle {
        id: ring
        visible: root.solo.helpTile >= 0 && root.accepting
        x: root.solo.helpTile >= 0 ? root.tileX(root.solo.helpTile) - Theme.startRing : 0
        y: root.solo.helpTile >= 0 ? root.tileY(root.solo.helpTile) - Theme.startRing : 0
        width: root.tileSize + 2 * Theme.startRing
        height: root.tileSize + 2 * Theme.startRing
        color: "transparent"
        border.width: Theme.litEdge
        border.color: Theme.pathCasing
        Accessible.role: Accessible.Graphic
        Accessible.name: SoloWording.startHere

        Rectangle {
            anchors.fill: parent
            anchors.margins: -Theme.rule
            color: "transparent"
            border.width: Theme.rule
            border.color: Theme.pathCore
        }

        // Never for more than 5 seconds (WCAG 2.2.2): five pulses, then it holds.
        SequentialAnimation on scale {
            id: pulse
            running: ring.visible && !Theme.reducedMotion
            loops: 5
            NumberAnimation { from: 1.0; to: 1.1; duration: Theme.ringPulse; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 1.1; to: 1.0; duration: Theme.ringPulse; easing.type: Easing.InOutQuad }
        }
    }

    // --- the grapheme bars ---------------------------------------------------------------
    // A block of tiles that make one sound is joined by a bar under their letters.
    Repeater {
        model: root.solo.blocks

        Rectangle {
            id: bar
            required property var modelData
            readonly property var first: root.pathList[modelData.first]
            readonly property var last: root.pathList[modelData.first + modelData.tiles - 1]
            readonly property bool valid: modelData.tiles > 1 && first !== undefined && last !== undefined
            readonly property point from: valid ? root.centre(first) : Qt.point(0, 0)
            readonly property point to: valid ? root.centre(last) : Qt.point(0, 0)
            readonly property real drop: root.tileSize * 0.36

            visible: valid
            x: from.x
            y: from.y + drop - height / 2
            width: Math.hypot(to.x - from.x, to.y - from.y)
            height: Theme.heavy * 2
            transformOrigin: Item.Left
            rotation: Math.atan2(to.y - from.y, to.x - from.x) * 180 / Math.PI
            color: Theme.dieInk
        }
    }

    // --- the path: a pale casing over a dark core ------------------------------------------------
    Shape {
        id: pathShape
        anchors.fill: parent
        visible: root.solo.path.length > 1
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.pathCasing
            strokeWidth: Theme.pathCasingWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline { path: root.pathPoints }
        }
        ShapePath {
            strokeColor: Theme.pathCore
            strokeWidth: Theme.pathCoreWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline { path: root.pathPoints }
        }
    }

    // --- a path that fades -----------------------------------------------------------------------------
    Shape {
        id: ghost
        anchors.fill: parent
        opacity: 0
        visible: opacity > 0 && root.ghostPath.length > 1
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.pathCasing
            strokeWidth: Theme.pathCasingWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline { path: root.ghostPoints }
        }
        ShapePath {
            strokeColor: Theme.pathCore
            strokeWidth: Theme.pathCoreWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline { path: root.ghostPoints }
        }
    }

    // --- the pointer ------------------------------------------------------------------------------------------
    // Only the first touch draws: a MouseArea is given the mouse events synthesised
    // from the first touch point, so a second finger or a resting palm is ignored.
    MouseArea {
        id: pointer
        x: root.boardRect.x - root.slack
        y: root.boardRect.y - root.slack
        width: root.boardRect.width + 2 * root.slack
        height: root.boardRect.height + 2 * root.slack
        enabled: root.accepting
        preventStealing: true

        function local(mouseX, mouseY) {
            return Qt.point(mouseX - root.slack, mouseY - root.slack);
        }
        function over(mouseX, mouseY) {
            const p = local(mouseX, mouseY);
            const s = root.slack;
            return p.x >= -s && p.y >= -s && p.x <= root.boardRect.width + s && p.y <= root.boardRect.height + s;
        }

        onPressed: mouse => {
            root.keyboardUsed = false;
            root.forceActiveFocus();
            const p = local(mouse.x, mouse.y);
            // The path work of a touch: the model's hit test, the path it
            // changes and every binding that follows it, until the frame.
            Profiler.begin("ui.trace");
            root.solo.pressAt(p.x, p.y);
            Profiler.end();
        }
        onPositionChanged: mouse => {
            const p = local(mouse.x, mouse.y);
            Profiler.begin("ui.trace");
            root.solo.moveTo(p.x, p.y);
            Profiler.end();
        }
        onReleased: mouse => {
            const p = local(mouse.x, mouse.y);
            Profiler.begin("ui.trace");
            root.solo.releaseAt(p.x, p.y, over(mouse.x, mouse.y));
            Profiler.end();
        }
        onCanceled: {
            Profiler.begin("ui.trace");
            root.solo.clearPath();
            Profiler.end();
        }
    }

    // --- the keyboard and the switch ------------------------------------------------------------------------------------
    // Arrow keys move the focus across tiles, Space adds the focused tile, Enter
    // hands the word in, Escape clears (and, with nothing to clear, goes back).
    Keys.onPressed: event => {
        if (!root.accepting || root.columns === 0) {
            return;
        }
        root.keyboardUsed = true;
        const row = Math.floor(root.focusIndex / root.columns);
        const column = root.focusIndex % root.columns;
        Profiler.begin("ui.trace");
        switch (event.key) {
        case Qt.Key_Left:
            root.focusIndex = row * root.columns + Math.max(0, column - 1);
            event.accepted = true;
            break;
        case Qt.Key_Right:
            root.focusIndex = row * root.columns + Math.min(root.columns - 1, column + 1);
            event.accepted = true;
            break;
        case Qt.Key_Up:
            root.focusIndex = Math.max(0, row - 1) * root.columns + column;
            event.accepted = true;
            break;
        case Qt.Key_Down:
            root.focusIndex = Math.min(root.rows - 1, row + 1) * root.columns + column;
            event.accepted = true;
            break;
        case Qt.Key_Space:
            root.solo.tapTile(root.focusIndex);
            event.accepted = true;
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
            root.solo.done();
            event.accepted = true;
            break;
        case Qt.Key_Backspace:
            root.solo.removeLast();
            event.accepted = true;
            break;
        case Qt.Key_Escape:
            if (root.solo.path.length > 0) {
                root.solo.clearPath();
                event.accepted = true;
            }
            break;
        default:
            break;
        }
        Profiler.end();
    }
}
