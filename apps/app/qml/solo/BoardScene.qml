// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import SnapAndSpell
import SnapAndSpell.Scene
import "SceneScript.js" as Script

// THE board scene (docs/design/board.md, "The shake and the drop"): given a finished board and a seed,
// play the shake and the drop, and say when the dice are settled. Nothing else in the application
// touches it, and no game rule is in it: a tile is an opaque string and "" is a dead jewel.
//
// It is drawn in 2D as pixel art (docs/design/toybox/DESIGN.md, "Motion"): the closed box jumps in
// stepped hops, its lid lifts in four steps, and each die falls through six flip-book frames. Nothing
// is smooth and nothing is lit, so it runs in a browser as it does on a desktop. The poses are pure
// functions of the board's size, the seed and the time (SceneScript.js).
//
//   play(board, seed)   shake, lid, drop, settle, for board = { rows, columns, tiles,
//                       faces, up }; while busy it restarts
//   show(board)         the board already down, with no motion; no tiles is the box
//                       with its lid on (the box before the first shake, and after
//                       the lid goes back on)
//   prepare(board)      no-op: this scene has no faces to draw ahead of a board
//   reducedMotion       in: the cross-fading variant of every stage
//   pushMotion(ax, ay, az, wx, wy, wz)   in: one sample of the device's motion, the
//                       accelerometer's reading in g (gravity included) and the
//                       gyroscope's in rad/s; it changes how the box moves and how long
//                       it rattles, never where a die lands. This scene reduces it to
//                       two numbers (agitation, how hard the device is shaken now, 0 to
//                       1, and tilt, which way it leans, each axis -1 to 1), which it
//                       also takes directly
//   settled()           once for each play, when the dice have stopped
//   landed(index, pitch) a die has hit the floor: its place in the board, and a pitch
//                       for its click
//   rattled()           the box rattles, at most 8 a second, for the touch feedback
//   busy, rows, columns, tileSize, tileGap, boardRect   read-only: the geometry by the
//                       layout rule, for the 2D layer that traces over the scene
//   overdraw            read-only: 0, this scene draws inside its item
//   trayInset           in: how far the tray stands off the tiles (the flat board's)
Item {
    id: root

    // --- the seam ------------------------------------------------------------
    property bool reducedMotion: false
    property real agitation: 0
    property vector2d tilt: Qt.vector2d(0, 0)
    property real trayInset: Theme.trayOutset(tile)

    // One sample of the device's motion (the interface of the board scene): how far the reading is
    // from still is how hard the device is shaken, and where gravity is in it is which way it leans.
    function pushMotion(ax, ay, az, wx, wy, wz) {
        agitation = Math.min(1, Math.sqrt(ax * ax + ay * ay + (az - 1) * (az - 1)) / 1.2);
        tilt = Qt.vector2d(Math.max(-1, Math.min(1, -ax / 0.14)),
                           Math.max(-1, Math.min(1, ay / 0.14)));
    }

    signal settled
    signal landed(int index, real pitch)
    signal rattled

    readonly property bool busy: stage !== "idle"
    readonly property int rows: boardRows
    readonly property int columns: boardColumns
    readonly property real tileSize: tile
    readonly property real tileGap: gap
    readonly property rect boardRect: Qt.rect((width - spanX) / 2, (height - spanY) / 2, spanX, spanY)
    // The scene draws inside its item.
    readonly property real overdraw: 0

    // The faces are drawn when the board arrives, as the flat board draws them.
    function prepare(board) {
        Profiler.begin("scene.prepare");
        Profiler.end();
    }

    function play(board, seed) {
        Profiler.begin("scene.play");
        setBoard(board);
        script = Script.create(boardRows * boardColumns, seed, reducedMotion);
        landedCount = 0;
        landedMask = [];
        rattleClock = 0;
        stage = "shake";
        stageMs = 0;
        if (reducedMotion) {
            rattled();
        }
        apply();
        Profiler.end();
    }

    function show(board) {
        Profiler.begin("scene.show");
        setBoard(board);
        script = null;
        stage = "idle";
        stageMs = 0;
        apply();
        Profiler.end();
    }

    // --- the geometry, by the rule in board_layout.hpp ---------------------------
    readonly property int cellsAcross: Math.max(boardRows, boardColumns, 1)
    readonly property real inset: BoardGeometry.inset(width, height)
    readonly property real gap: BoardGeometry.gap(width, height)
    readonly property real tile: Math.max(1, BoardGeometry.tile(width, height, cellsAcross))
    readonly property real pitch: tile + gap
    readonly property real spanX: boardColumns * tile + (boardColumns - 1) * gap
    readonly property real spanY: boardRows * tile + (boardRows - 1) * gap

    // --- state -----------------------------------------------------------------
    property int boardRows: 4
    property int boardColumns: 4
    property var boardTiles: []
    property string stage: "idle"      // idle | shake | lid | drop | settle
    property real stageMs: 0
    property var script: null
    property var poses: []
    property point boxHop: Qt.point(0, 0)
    property real lidOpen: 1
    property int landedCount: 0
    property var landedMask: []
    property real rattleClock: 0

    readonly property bool boxClosed: boardTiles.length === 0
    readonly property bool lidShown: boxClosed || stage === "shake" || stage === "lid"

    function setBoard(board) {
        boardRows = board.rows;
        boardColumns = board.columns;
        boardTiles = board.tiles;
    }

    function enter(next) {
        stage = next;
        stageMs = 0;
    }

    // The zone a frame of the stage is timed in (docs/profiling.md, "QML and the scene").
    function stageZone() {
        switch (stage) {
        case "shake":
        case "lid":
            return "scene.shake";
        case "drop":
            return "scene.drop";
        case "settle":
            return "scene.settle";
        default:
            return "";
        }
    }

    function step(dtMs) {
        if (stage === "idle" || script === null) {
            return;
        }
        Profiler.begin(stageZone());
        stageMs += Math.min(dtMs, 50);

        if (stage === "shake") {
            const minimum = script.reduced ? Script.stageLength(script, "shake") : Script.SHAKE_MIN;
            const held = !script.reduced && agitation > 0.05 && stageMs < Script.SHAKE_MAX;
            rattleClock += dtMs;
            if (!script.reduced && rattleClock >= Script.RATTLE_GAP) {
                rattleClock = 0;
                rattled();
            }
            if (stageMs >= minimum && !held) {
                enter("lid");
            }
        } else if (stage === "lid") {
            if (stageMs >= Script.stageLength(script, "lid")) {
                enter("drop");
            }
        } else if (stage === "drop") {
            for (let i = 0; i < script.count; ++i) {
                if (landedMask[i] !== true && stageMs >= Script.landingTime(script, i)) {
                    landedMask[i] = true;
                    ++landedCount;
                    landed(i, script.pitch[i]);
                }
            }
            if (stageMs >= Script.stageLength(script, "drop")) {
                enter("settle");
            }
        } else if (stage === "settle") {
            if (stageMs >= Script.stageLength(script, "settle")) {
                stage = "idle";
                stageMs = 0;
                apply();
                Profiler.end();
                settled();
                return;
            }
        }
        apply();
        Profiler.end();
    }

    // The poses of every die for the stage and time we are at.
    function apply() {
        const next = [];
        const count = boardRows * boardColumns;
        for (let i = 0; i < count; ++i) {
            next.push(script === null ? { x: 0, y: 0, fall: 0, opacity: 1, frame: Script.FRAMES }
                                      : Script.pose(script, stage, stageMs, i, tile));
        }
        poses = next;
        const hop = script === null ? { x: 0, y: 0 }
                                    : Script.boxHop(script, stage, stageMs, agitation, tilt.x);
        boxHop = Qt.point(hop.x, hop.y);
        lidOpen = script === null ? (boxClosed ? 0 : 1) : Script.lidOpen(script, stage, stageMs);
    }

    FrameAnimation {
        running: root.busy
        onTriggered: root.step(frameTime * 1000)
    }

    // --- what is drawn -----------------------------------------------------------
    // The box: the tray with its teal well, which hops as a whole.
    Item {
        id: box
        x: root.boxHop.x
        y: root.boxHop.y
        width: root.width
        height: root.height

        BoardTray {
            x: root.boardRect.x - root.trayInset
            y: root.boardRect.y - root.trayInset
            width: root.boardRect.width + 2 * root.trayInset
            height: root.boardRect.height + 2 * root.trayInset
        }

        BoardWell {
            x: root.boardRect.x
            y: root.boardRect.y
            rows: root.boardRows
            columns: root.boardColumns
            tile: root.tile
            pitch: root.pitch
        }

        Repeater {
            model: root.boardRows * root.boardColumns

            SceneDie {
                required property int index
                readonly property var pose: root.poses[index] !== undefined
                                            ? root.poses[index]
                                            : { x: 0, y: 0, fall: 0, opacity: 1, frame: 6 }
                x: root.boardRect.x + (index % root.boardColumns) * root.pitch + pose.x
                y: root.boardRect.y + Math.floor(index / root.boardColumns) * root.pitch
                   + Math.round(pose.fall * root.tile)
                width: root.tile
                height: root.tile
                opacity: root.boxClosed ? 0 : pose.opacity
                letters: root.boardTiles[index] !== undefined ? root.boardTiles[index] : ""
            }
        }

        // The lid: a cream slab with an ink outline, a hard shadow and a coral label, which lifts away
        // in four steps. With reduced motion it does not move: it fades.
        PixelBox {
            readonly property real lift: root.reducedMotion ? 0 : root.lidOpen * root.tile * 0.8

            x: root.boardRect.x - root.trayInset
            y: root.boardRect.y - root.trayInset - lift
            width: root.boardRect.width + 2 * root.trayInset
            height: root.boardRect.height + 2 * root.trayInset
            visible: root.lidShown && root.lidOpen < 1
            opacity: root.reducedMotion ? 1 - root.lidOpen : 1
            radius: Theme.radiusLarge
            fill: Theme.lid
            edge: Theme.trayEdge
            edgeWidth: Theme.outlineFrame
            shadowOffset: Theme.shadowFrame

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width * 0.3, Theme.space16 * 2)
                height: Math.max(Theme.space4, width * 0.34)
                radius: Theme.radiusXs
                color: Theme.coral
                border.width: Theme.outlineControl
                border.color: Theme.trayEdge
            }
        }
    }
}
