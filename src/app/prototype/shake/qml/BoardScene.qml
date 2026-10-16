import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import "ShakeScript.js" as Script

// The board scene: the one component the application would put the shake and
// the drop in (docs/design/board.md, "Decided (the author, 2026-10-16)").
//
// What it is given: a finished board, the dice it was rolled with, and a seed.
// What it does: plays the shake, the lid and the drop, and says when the dice
// are settled. It decides nothing about the game: a face is an opaque string to
// it, "" is a dead jewel, and the letters on the dice are the ones it was
// handed. Nothing outside this file touches the scene, and everything it needs
// is in this file's interface:
//
//   in    play(board, seed), show(board), reducedMotion, pushMotion(...)
//   out   settled(), landed(index, pitch), rattled(), busy, and the geometry the
//         2D layer over it lays out by: boardRect, tileSize, tileGap, and how
//         far past its item the scene draws: overdraw
//
// board is { rows, columns, tiles, faces, up }, each of the three arrays with an
// entry for each tile, row-major: tiles[t] is the letters of the finished
// board's tile t, faces[t] the six strings on the die that is on it ("" is the
// jewel, and a dead jewel is a die of six), and up[t] which of the six is up,
// so that faces[t][up[t]] === tiles[t]. The scene draws each die with its six
// faces, shows the others as it tumbles, and lands it with the up face on top,
// the letter upright. It reads no letter and decides no face: they are the
// library's roll (wjs::roll_with_placements), passed through.
//
// The drop is a pure function of the board, the seed and the time. There is no
// physics engine: every position and angle is computed by ShakeScript.js, a
// script that `seed` makes and that has no renderer in it, so a replay is the
// same frame for frame (digest() shows it) and a 2D scene would draw the same
// script. This file turns the script's poses into Qt Quick 3D nodes.
// The rattle is the one live part, and it answers the device's motion sample
// for sample: pushMotion() takes the accelerometer's reading and the
// gyroscope's, about a hundred a second, and the dice are accelerated by the
// negative of the reading (gravity, tilt, a shove and a jolt are all one thing)
// and by the spin. The motion decides how the dice move and how long the box
// rattles, never where a die lands. With no device (the Shake button, a test)
// a seeded stand-in makes the stream, so that a replay is the same shake.
//
// The View3D is made once and kept: a new board is a new set of dice inside it.
Item {
    id: root

    // ---- The seam: in ------------------------------------------------------
    // Cross-fades instead of moving (docs/design/application.md, "Motion").
    property bool reducedMotion: false

    // One sample of the device's motion: the accelerometer's reading in g,
    // gravity included (a device lying face up reads 0, 0, 1; x to the right,
    // y up the screen, z out of the screen), and the gyroscope's in rad/s.
    // Call it for every sample the sensors give, about a hundred a second.
    function pushMotion(ax, ay, az, wx, wy, wz) {
        Script.feed(tracker, ax, ay, az, wx, wy, wz);
        lastMotionMs = Date.now();
    }

    // ---- The seam: out -----------------------------------------------------
    // The dice have stopped. Emitted once, at the end of the settle.
    signal settled()
    // A die has hit the floor: its index in the board, and a pitch to play its
    // click at (0.9 to 1.15, a little different for each die).
    signal landed(int index, real pitch)
    // The box rattles: at most 8 a second, for the haptic tap.
    signal rattled()

    readonly property bool busy: stage !== "idle"
    // The board's size and square, in this item's own coordinates, by the rule
    // in docs/design/board.md ("The rule"): the largest square that fits less an
    // inset, tiles a gap apart. The 2D layer for tracing is laid out by these.
    readonly property int rows: boardRows
    readonly property int columns: boardColumns
    readonly property real tileSize: tile
    readonly property real tileGap: gap
    readonly property rect boardRect: Qt.rect((width - spanX) / 2, (height - spanY) / 2, spanX, spanY)
    // How far past this item's edges the scene draws, on every side: a die that
    // tumbles at the edge of the board, and the box's rim as it tips, are not
    // cut off. The item is still the board's area and boardRect is still by the
    // rule; the application only has to leave the item unclipped (no `clip`).
    readonly property real overdraw: tile * (slackTiles + 0.4)

    // Plays the shake, the lid and the drop for a board. The dice are in the
    // box before the first frame, and `settled` follows the last of them.
    function play(board, seed) {
        frames.running = false;
        // A device that has sent a sample in the last 300 ms is the shake; if
        // none has, the stand-in shakes (a button, a test, a replay).
        const live = Date.now() - lastMotionMs < 300;
        if (!live) {
            Script.resetTracker(tracker);
        }
        begin(board, seed, !live);
        stage = current.stage;
        apply();
        frames.running = true;
    }

    // Shows a board already dropped: no motion. For a game that is resumed, or
    // a window that is rotated.
    function show(board) {
        frames.running = false;
        begin(board, 0);
        Script.finish(current);
        if (warmUp && !warmedOnce) {
            warmedOnce = true;
            warmFrames = 3;
            frames.running = true;
        }
        apply();
        stage = "idle";
    }

    // A checksum of every pose the board and seed produce, frame by frame at
    // 60 Hz with the device still. Two plays of the same board and seed give
    // the same string; it is how a replay is tested.
    function digest(board, seed) {
        return Script.digest(board, seed, layout());
    }

    // What a play of the board and seed does to the faces, device still: how
    // many dice end with their up face on top and the letter upright, and how
    // many different faces each die showed uppermost on the way.
    function faceReport(board, seed) {
        return Script.faceReport(board, seed, layout());
    }

    // Puts the scene where the script for a board and seed is `ms` into the
    // play, with the device still, and holds it there. For tests and for
    // pictures: the same board, seed and time always give the same picture.
    function seek(board, seed, ms) {
        frames.running = false;
        setBoard(board);
        current = Script.at(board, seed, layout(), ms);
        stage = current.stage;
        apply();
    }

    // ---- Prototype settings, not part of the seam --------------------------
    // How far the camera leans from straight down while the dice are in the
    // air, in degrees. It comes upright as they settle, so the 2D layer lines up.
    property real lean: 0
    // "none", "procedural" (Qt's sky, prefiltered on the first frame) or
    // "baked" (the same, prefiltered in advance and read from a file).
    property string probeMode: "baked"
    property real cameraDistance: 2.2   // times the board's side
    // Weighted motion: the dice are thrown about the box, bounce, collide and
    // slot into their cells (ShakeScript.js). False is the prescribed curves.
    property bool weighted: true
    // How long the stand-in's burst of shaking lasts, when there is no device:
    // 0 takes 0.7 to 1.0 s from the seed.
    property real mockBurstMs: 0
    // While the first board is shown, draw the lid, the shadows and the dice
    // through their blended paths for three frames at 2% opacity (Qt skips
    // anything under 1%), so the first shake does not pay for creating them.
    property bool warmUp: true

    // ---- The board and the layout -----------------------------------------
    property int boardRows: 4
    property int boardColumns: 4
    // The six faces of the die on each tile, and which is up (see above).
    property var boardFaces: []
    property var boardUp: []

    readonly property real shortSide: Math.max(1, Math.min(width, height))
    readonly property real inset: shortSide < 600 ? 12 : 24
    readonly property real gap: shortSide < 600 ? 6 : 8
    readonly property int cellsAcross: Math.max(boardRows, boardColumns, 1)
    readonly property real tile: Math.max(1, (shortSide - 2 * inset - (cellsAcross - 1) * gap) / cellsAcross)
    readonly property real pitch: tile + gap
    readonly property real spanX: boardColumns * tile + (boardColumns - 1) * gap
    readonly property real spanY: boardRows * tile + (boardRows - 1) * gap
    readonly property real rimWidth: inset * 0.7
    // The box's cavity is a little roomier than the board, so a die has
    // something to slide into and bump against when it is shaken.
    readonly property real slackTiles: 0.6
    readonly property real slack: tile * slackTiles
    readonly property real boxX: spanX + 2 * slack
    readonly property real boxY: spanY + 2 * slack
    readonly property real boxDepth: tile * 0.34
    readonly property real lidHeight: tile * 2.45

    // ---- The play in progress ---------------------------------------------
    property string stage: "idle"
    // The script being played (see script()), or null before the first board.
    property var current: null
    property int warmFrames: 0
    // The device's motion as it has come in (see pushMotion), and when.
    property var tracker: Script.newTracker()
    property double lastMotionMs: -1e9
    property bool warmedOnce: false

    // ---- Driving the scene --------------------------------------------------
    // What the script needs of the layout, in this item's pixels.
    function layout() {
        return { tile: tile, gap: gap, pitch: pitch, reduced: reducedMotion, weighted: weighted,
                 lidTiles: 2.45, slackTiles: slackTiles, mock: true, mockBurstMs: mockBurstMs };
    }

    // The board's dice. A board whose faces do not make its tiles is drawn as
    // given and reported: the scene lands the up face, so a mismatch would show
    // the wrong letter without a sound.
    function setBoard(board) {
        boardRows = board.rows;
        boardColumns = board.columns;
        boardFaces = board.faces;
        boardUp = board.up;
        if (board.tiles) {
            for (let t = 0; t < board.tiles.length; ++t) {
                if (board.faces[t][board.up[t]] !== board.tiles[t]) {
                    console.warn("BoardScene: tile " + t + " is '" + board.tiles[t]
                                 + "' but the face that is up is '" + board.faces[t][board.up[t]] + "'");
                    break;
                }
            }
        }
    }

    function begin(board, seed, standIn) {
        setBoard(board);
        const settings = layout();
        settings.mock = standIn !== false;
        current = Script.create(board, seed, settings);
        // The dice are made as the count changes; a die that is not there yet
        // is posed when it arrives (onObjectAdded below).
    }

    function apply() {
        const state = current;
        if (!state) {
            return;
        }
        const warm = warmFrames > 0;
        for (let i = 0; i < state.count; ++i) {
            const die = diceRepeater.objectAt(i);
            if (!die) {
                continue;
            }
            die.body.position = Qt.vector3d(state.px[i] - die.x, state.py[i] - die.y, state.pz[i]);
            if (state.useQuaternion) {
                die.body.rotation = Qt.quaternion(state.qw[i], state.qx[i], state.qy[i], state.qz[i]);
            } else {
                die.body.eulerRotation = Qt.vector3d(state.rx[i], state.ry[i], state.rz[i]);
            }
            // Which face is up, and how the die is turned over getting there.
            die.cube.rotation = Qt.quaternion(state.tw[i], state.tx[i], state.ty[i], state.tz[i]);
            die.body.visible = state.opacity[i] > 0.001;
            die.body.opacity = warm ? 0.98 : state.opacity[i];
            const lift = Math.max(0, state.pz[i] - tile / 2);
            die.shadow.position = Qt.vector3d(state.px[i] - die.x + 0.45 * lift, state.py[i] - die.y - 0.45 * lift, 0.2);
            const radius = state.shadowRadius[i] / 50;
            die.shadow.scale = Qt.vector3d(radius, radius, 0.02);
            die.shadow.opacity = warm ? Math.max(state.shadowOpacity[i], 0.02) : state.shadowOpacity[i];
        }
        boxNode.eulerRotation = Qt.vector3d(state.boxRx, state.boxRy, 0);
        lidHinge.eulerRotation = Qt.vector3d(state.lidAngle, 0, 0);
        lidHinge.position = Qt.vector3d(0, boxY / 2 + rimWidth, lidHeight + state.lidLift);
        lid.opacity = warm ? Math.max(state.lidOpacity, 0.02) : state.lidOpacity;
        lidHinge.visible = warm || state.lidOpacity > 0.001;
        leanNow = lean * state.leanFraction;
    }

    function step(dtMs) {
        const state = current;
        if (!state) {
            return;
        }
        if (warmFrames > 0) {
            warmFrames -= 1;
            if (state.stage === "idle") {
                apply();
                if (warmFrames === 0) {
                    frames.running = false;
                }
                return;
            }
        }
        Script.advance(state, dtMs, tracker);
        Script.evaluate(state);
        apply();
        stage = state.stage;
        const events = state.events;
        state.events = [];
        for (let k = 0; k < events.length; ++k) {
            const event = events[k];
            if (event.kind === "rattled") {
                rattled();
            } else if (event.kind === "landed") {
                landed(event.index, event.pitch);
            } else if (event.kind === "settled") {
                frames.running = false;
                settled();
            }
        }
    }

    property real leanNow: lean

    // One tick for each frame the window draws; a stalled frame counts as 50 ms,
    // so a hitch slows the play down instead of skipping through it.
    FrameAnimation {
        id: frames

        running: false
        onTriggered: root.step(Math.min(frameTime * 1000, 50))
    }

    // The board changing size creates the dice; give them their poses as they
    // arrive.
    onBoardFacesChanged: Qt.callLater(apply)

    // ---- The scene ----------------------------------------------------------
    // The mesh of a die, as plain arrays from the script.
    readonly property var meshData: Script.cubeMesh()

    // The atlas is not part of the scene's tree; the textures read it.
    property Item atlasItem: FaceAtlas {
        faces: root.boardFaces
        rows: root.boardRows
        columns: root.boardColumns
    }

    // Made once, when the scene is, for the mode asked for: a Texture has to
    // live in the 3D scene, so it is made under the scene's own root.
    property Texture probeTexture: root.probeMode === "none" ? null
        : (root.probeMode === "baked" ? bakedProbe.createObject(probeHost)
                                      : proceduralProbe.createObject(probeHost))

    Component {
        id: proceduralProbe

        // Qt's own sky, used only as a light. Qt prefilters it into a cube map
        // on the first frame of the view.
        Texture {
            textureData: ProceduralSkyTextureData {}
        }
    }

    Component {
        id: bakedProbe

        // The same sky, prefiltered in advance by tools/lightprobe, so Qt does
        // no prefiltering at run time.
        Texture {
            source: "lightprobe/sky.ktx"
        }
    }

    // The distance at which, upright, one scene unit is one pixel on the top
    // of the dice, and the field of view that gives, for the view, which is the
    // item and its overdraw. While the camera leans it also stands a little back
    // (and comes in as it comes upright), so a board that fills its square
    // upright is not cut off by the lean.
    readonly property real viewDistance: cameraDistance * Math.max(spanX, spanY)
    readonly property real fieldOfView: 2 * Math.atan((height + 2 * overdraw) / 2 / viewDistance) * 180 / Math.PI
    readonly property real standBack: 1 + 0.3 * (lean > 0 ? Math.min(1, leanNow / lean) : 0)

    View3D {
        id: view

        x: -root.overdraw
        y: -root.overdraw
        width: root.width + 2 * root.overdraw
        height: root.height + 2 * root.overdraw
        camera: camera

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.Medium
            lightProbe: root.probeTexture
            probeExposure: 0.55
        }

        // Looks at the middle of the dice's top faces, from a distance chosen
        // so that, upright, one scene unit is one pixel at that height: the 2D
        // tiles over the scene are then exactly the dice's tops.
        PerspectiveCamera {
            id: camera

            readonly property real radians: root.leanNow * Math.PI / 180
            fieldOfView: root.fieldOfView
            fieldOfViewOrientation: PerspectiveCamera.Vertical
            clipNear: 10
            clipFar: root.viewDistance * 5
            position: Qt.vector3d(0, -root.viewDistance * root.standBack * Math.sin(radians),
                                  root.tile + root.viewDistance * root.standBack * Math.cos(radians))
            eulerRotation: Qt.vector3d(root.leanNow, 0, 0)
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(-38, -28, 0)
            brightness: 0.7
            ambientColor: Qt.rgba(0.20, 0.20, 0.22, 1)
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(28, 24, 0)
            brightness: 0.3
        }

        // Where the probe's texture and the dice's mesh live.
        Node {
            id: probeHost

            // The one cube every die shares, six sides each with its own cell
            // of the die's block in the atlas. Made from the script's table of
            // sides, so that it and the die's orientation agree on which side
            // is which face.
            ProceduralMesh {
                id: dieMesh

                primitiveMode: ProceduralMesh.Triangles
                positions: root.meshData.positions.map(p => Qt.vector3d(p[0], p[1], p[2]))
                normals: root.meshData.normals.map(p => Qt.vector3d(p[0], p[1], p[2]))
                uv0s: root.meshData.uvs.map(t => Qt.vector2d(t[0], t[1]))
                indexes: root.meshData.indexes
            }
        }

        // The box, with the dice and the lid in it, tilts as one.
        Node {
            id: boxNode

            // The floor and the low rim.
            Model {
                source: "#Cube"
                position: Qt.vector3d(0, 0, -root.boxDepth * 0.15)
                scale: Qt.vector3d((root.boxX + 2 * root.rimWidth) / 100,
                                   (root.boxY + 2 * root.rimWidth) / 100,
                                   root.boxDepth * 0.3 / 100)
                castsShadows: false
                receivesShadows: false
                materials: PrincipledMaterial {
                    baseColor: "#2C4C61"
                    roughness: 0.85
                }
            }

            Model {
                source: "#Cube"
                position: Qt.vector3d(0, root.boxY / 2 + root.rimWidth / 2, root.boxDepth / 2)
                scale: Qt.vector3d((root.boxX + 2 * root.rimWidth) / 100, root.rimWidth / 100, root.boxDepth / 100)
                materials: rimMaterial
            }

            Model {
                source: "#Cube"
                position: Qt.vector3d(0, -root.boxY / 2 - root.rimWidth / 2, root.boxDepth / 2)
                scale: Qt.vector3d((root.boxX + 2 * root.rimWidth) / 100, root.rimWidth / 100, root.boxDepth / 100)
                materials: rimMaterial
            }

            Model {
                source: "#Cube"
                position: Qt.vector3d(root.boxX / 2 + root.rimWidth / 2, 0, root.boxDepth / 2)
                scale: Qt.vector3d(root.rimWidth / 100, root.boxY / 100, root.boxDepth / 100)
                materials: rimMaterial
            }

            Model {
                source: "#Cube"
                position: Qt.vector3d(-root.boxX / 2 - root.rimWidth / 2, 0, root.boxDepth / 2)
                scale: Qt.vector3d(root.rimWidth / 100, root.boxY / 100, root.boxDepth / 100)
                materials: rimMaterial
            }

            PrincipledMaterial {
                id: rimMaterial

                baseColor: "#1F3646"
                roughness: 0.7
            }

            Repeater3D {
                id: diceRepeater

                model: root.boardRows * root.boardColumns

                delegate: Die {
                    atlas: root.atlasItem
                    mesh: dieMesh
                    columns: root.boardColumns
                    rows: root.boardRows
                    size: root.tile
                    x: ((index % root.boardColumns) - (root.boardColumns - 1) / 2) * root.pitch
                    y: ((root.boardRows - 1) / 2 - Math.floor(index / root.boardColumns)) * root.pitch
                }

                onObjectAdded: Qt.callLater(root.apply)
            }

            // The lid: a pane of frosted glass hinged on the far edge.
            Node {
                id: lidHinge

                position: Qt.vector3d(0, root.boxY / 2 + root.rimWidth, root.lidHeight)

                Model {
                    id: lid

                    source: "#Cube"
                    position: Qt.vector3d(0, -(root.boxY / 2 + root.rimWidth), 0)
                    scale: Qt.vector3d((root.boxX + 2 * root.rimWidth) / 100,
                                       (root.boxY + 2 * root.rimWidth) / 100,
                                       root.tile * 0.06 / 100)
                    castsShadows: false
                    receivesShadows: false
                    materials: PrincipledMaterial {
                        baseColor: Qt.rgba(0.86, 0.93, 0.97, 0.28)
                        alphaMode: PrincipledMaterial.Blend
                        roughness: 0.25
                    }
                }
            }
        }
    }
}
