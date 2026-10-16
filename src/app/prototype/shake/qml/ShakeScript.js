.pragma library

// The shake, the lid and the drop as a script: what every die, the box, the lid
// and the camera are doing at each moment of a play. It is plain JavaScript with
// no QML in it and no renderer, so the Qt Quick 3D scene (BoardScene.qml) is one
// way to draw it and a 2D scene would be another, and the two replay the same
// board and seed the same way.
//
// There is no physics engine, and the board is decided before the play starts.
// Where a die ends is its cell, always. How it gets there is computed from the
// board, the seed and the device's motion, in one of two ways (layout.weighted):
//
//   staged    every position and angle is a prescribed curve of the time: a
//             sine rattle, a fall on a parabola, one rebound.
//   weighted  a rigid-ish body simulation of our own, at a fixed 480 steps a
//             second, in the box's own frame: the dice are accelerated by the
//             *negative of what the accelerometer reads* (so gravity, tilt, a
//             shove and a jolt are all one thing), and by the centrifugal,
//             Coriolis and Euler terms of the gyroscope's rotation. They
//             slide with friction, leave the floor, hit the lid, the walls and
//             one another, spin when they hit, and slot into their cells under
//             springs once the lid is off.
//
// The device's motion comes in as samples (feed): the accelerometer's reading
// in g, gravity included, and the gyroscope's in rad/s, about 100 a second.
// With no device (the Shake button, a test, a replay) a seeded stand-in makes
// the same kind of stream (mockSampleAt): strokes with sharp turnarounds, spikes
// and noise, so a replay of a board and seed is the same shake.
//
//   state = create(board, seed, layout)   the script for a play, at time zero
//   advance(state, dtMs, tracker)         moves it on; collects events
//   evaluate(state)                       writes the pose of everything now
//
// The pose is in state.px, py, pz (the die's centre, in the layout's pixels
// from the centre of the board, y up, z up from the floor), rx, ry, rz
// (degrees), tw, tx, ty, tz (a quaternion: which of the die's six faces is up,
// and how it is turned over on the way there), opacity, shadowRadius,
// shadowOpacity, and state.boxRx, boxRy, lidAngle, lidLift, lidOpacity,
// leanFraction. The events (state.events) are { kind: "rattled" },
// { kind: "landed", index, pitch } and { kind: "settled" }.
//
// A die has six faces and the board says which is up (`board.faces[i]`, the six
// strings of the die on tile i, and `board.up[i]`, the index of the one that
// is). The script does not read the letters. It lays each die with its up face
// on top and its letter upright when the play is over, and makes the die show
// other faces on the way: in the weighted script a die turns a quarter or a
// half turn at each knock it takes in the box, and then turns to its up face as
// it slots in; in the staged script it lies on another face in the box and
// turns over as it falls. Which face is on which side of the cube is `sides`
// below, and the mesh and the atlas are made to agree with it. The faces have
// generators of their own, so nothing about where a die goes or how it moves
// depends on them.

// The stages and their durations: starting values to tune on a device
// (docs/design/board.md, "The stages").
var shakeMinMs = 700;
var shakeMaxMs = 3000;
var shakeStillMs = 300;       // reduced motion: the one soft rattle
var lidMs = 250;
var lidFadeMs = 120;          // reduced motion
var dropGapMaxMs = 30;
var dropGapSpanMs = 700;
var fallMinMs = 280;
var fallMaxMs = 420;
var dropFadeMs = 120;         // reduced motion
var settleMs = 150;
var boxDegrees = 8;
var rattleTickMs = 125;       // at most 8 a second

// mulberry32: a small, fast, seedable generator.
function random(seed) {
    var a = (seed | 0) ^ 0x5DEECE6;
    return function () {
        a = (a + 0x6D2B79F5) | 0;
        var t = Math.imul(a ^ (a >>> 15), 1 | a);
        t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

function smooth(u) {
    var x = Math.max(0, Math.min(1, u));
    return x * x * (3 - 2 * x);
}

// ---- The faces of a die ------------------------------------------------------
// Side k of the cube carries face k of the die. Each side has its outward
// normal n, and the directions r (right) and u (up) its letter is read in when
// the side is seen from outside, right-handed (r x u = n). Which faces are
// opposite is the cube's and not the die's: the library says only which six
// letters a die has, so which are neighbours is for show.
var sides = [
    { n: [0, 0, 1], r: [1, 0, 0], u: [0, 1, 0] },     // 0 top
    { n: [0, 0, -1], r: [1, 0, 0], u: [0, -1, 0] },   // 1 bottom
    { n: [0, -1, 0], r: [1, 0, 0], u: [0, 0, 1] },    // 2 toward the viewer
    { n: [0, 1, 0], r: [-1, 0, 0], u: [0, 0, 1] },    // 3 away
    { n: [1, 0, 0], r: [0, 1, 0], u: [0, 0, 1] },     // 4 right
    { n: [-1, 0, 0], r: [0, -1, 0], u: [0, 0, 1] }    // 5 left
];

// The atlas holds a block of 3 x 2 cells for each die, side k in the cell at
// column k % 3 and row floor(k / 3) from the top of the block.
var blockColumns = 3;
var blockRows = 2;

// The mesh of one die: a cube of edge 100 (the scene scales it), four vertices
// to a side, each side's texture coordinates in its own cell of the block.
// Plain arrays: the scene makes them a mesh.
function cubeMesh() {
    var positions = [];
    var normals = [];
    var uvs = [];
    var indexes = [];
    for (var k = 0; k < sides.length; ++k) {
        var side = sides[k];
        var column = k % blockColumns;
        var row = Math.floor(k / blockColumns);
        var u0 = column / blockColumns;
        var u1 = (column + 1) / blockColumns;
        // The atlas is an item's texture and an item's rows count from the
        // bottom of the texture: the top row of the block is the highest v.
        var v0 = (blockRows - 1 - row) / blockRows;
        var v1 = v0 + 1 / blockRows;
        var corners = [[-1, -1, u0, v0], [1, -1, u1, v0], [1, 1, u1, v1], [-1, 1, u0, v1]];
        var first = positions.length;
        for (var c = 0; c < 4; ++c) {
            var a = corners[c][0];
            var b = corners[c][1];
            positions.push([50 * (side.n[0] + a * side.r[0] + b * side.u[0]),
                            50 * (side.n[1] + a * side.r[1] + b * side.u[1]),
                            50 * (side.n[2] + a * side.r[2] + b * side.u[2])]);
            normals.push([side.n[0], side.n[1], side.n[2]]);
            uvs.push([corners[c][2], corners[c][3]]);
        }
        indexes.push(first, first + 1, first + 2, first, first + 2, first + 3);
    }
    return { positions: positions, normals: normals, uvs: uvs, indexes: indexes };
}

// Quaternions are [w, x, y, z].
function quatMultiply(a, b) {
    return [a[0] * b[0] - a[1] * b[1] - a[2] * b[2] - a[3] * b[3],
            a[0] * b[1] + a[1] * b[0] + a[2] * b[3] - a[3] * b[2],
            a[0] * b[2] - a[1] * b[3] + a[2] * b[0] + a[3] * b[1],
            a[0] * b[3] + a[1] * b[2] - a[2] * b[1] + a[3] * b[0]];
}

function quatAxis(x, y, z, degrees) {
    var half = degrees * Math.PI / 360;
    var s = Math.sin(half);
    return [Math.cos(half), x * s, y * s, z * s];
}

// The rotation that takes r to x, u to y and n to z: the matrix whose rows are
// r, u and n.
function quatFromBasis(r, u, n) {
    var trace = r[0] + u[1] + n[2];
    var s;
    if (trace > 0) {
        s = Math.sqrt(trace + 1) * 2;
        return [s / 4, (n[1] - u[2]) / s, (r[2] - n[0]) / s, (u[0] - r[1]) / s];
    }
    if (r[0] > u[1] && r[0] > n[2]) {
        s = Math.sqrt(1 + r[0] - u[1] - n[2]) * 2;
        return [(n[1] - u[2]) / s, s / 4, (r[1] + u[0]) / s, (r[2] + n[0]) / s];
    }
    if (u[1] > n[2]) {
        s = Math.sqrt(1 + u[1] - r[0] - n[2]) * 2;
        return [(r[2] - n[0]) / s, (r[1] + u[0]) / s, s / 4, (u[2] + n[1]) / s];
    }
    s = Math.sqrt(1 + n[2] - r[0] - u[1]) * 2;
    return [(u[0] - r[1]) / s, (r[2] + n[0]) / s, (u[2] + n[1]) / s, s / 4];
}

// The way round the shorter arc from a to b, t from 0 to 1.
function quatSlerp(a, b, t) {
    var dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    var sign = dot < 0 ? -1 : 1;
    dot = Math.abs(dot);
    var wa;
    var wb;
    if (dot > 0.9995) {
        wa = 1 - t;
        wb = t * sign;
    } else {
        var angle = Math.acos(Math.min(1, dot));
        var sine = Math.sin(angle);
        wa = Math.sin((1 - t) * angle) / sine;
        wb = Math.sin(t * angle) / sine * sign;
    }
    var w = wa * a[0] + wb * b[0];
    var x = wa * a[1] + wb * b[1];
    var y = wa * a[2] + wb * b[2];
    var z = wa * a[3] + wb * b[3];
    var length = Math.sqrt(w * w + x * x + y * y + z * z);
    return [w / length, x / length, y / length, z / length];
}

// Whether two quaternions are the same turn (q and -q are).
function quatSame(a, b) {
    return Math.abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]) > 0.99999;
}

// How a die lies with face k up and its letter upright, turned about the
// vertical by `quarters` quarter turns.
function faceUp(k, quarters) {
    var side = sides[k];
    var turn = quatFromBasis(side.r, side.u, side.n);
    return quarters ? quatMultiply(quatAxis(0, 0, 1, 90 * quarters), turn) : turn;
}

// The vector v turned by the quaternion q.
function quatRotate(q, v) {
    var t = quatMultiply(quatMultiply(q, [0, v[0], v[1], v[2]]), [q[0], -q[1], -q[2], -q[3]]);
    return [t[1], t[2], t[3]];
}

// Which side of a die turned by q is uppermost, and how far that side's
// letter is turned from upright (degrees, 0 to 180).
function topSide(q) {
    var best = 0;
    var height = -2;
    for (var k = 0; k < sides.length; ++k) {
        var z = quatRotate(q, sides[k].n)[2];
        if (z > height) {
            height = z;
            best = k;
        }
    }
    var right = quatRotate(q, sides[best].r);
    return { side: best, height: height, tilt: Math.abs(Math.atan2(right[1], right[0]) * 180 / Math.PI) };
}

// board is { rows, columns, faces, up } (and tiles, which the script does not
// read); layout is { tile, gap, pitch, reduced,
// weighted, lidTiles }, in the pixels the scene is drawn in (lidTiles is the
// height of the lid's middle above the floor, in tiles).
function create(board, seed, layout) {
    var rowCount = board.rows;
    var columnCount = board.columns;
    var count = rowCount * columnCount;
    var rand = random(seed);
    var faceRand = random((seed | 0) * 40503 + 2654435);
    var state = {
        count: count,
        rows: rowCount,
        columns: columnCount,
        tile: layout.tile,
        gap: layout.gap,
        pitch: layout.pitch,
        reduced: layout.reduced,
        stage: "shake",
        clock: 0,
        stageStart: 0,
        smoothAgitation: 0,
        tickAccumulator: 0,
        tiltX: 0,
        tiltY: 0,
        events: [],
        dice: [],
        dropMs: 0,
        px: new Array(count), py: new Array(count), pz: new Array(count),
        rx: new Array(count), ry: new Array(count), rz: new Array(count),
        tw: new Array(count), tx: new Array(count), ty: new Array(count), tz: new Array(count),
        opacity: new Array(count), shadowRadius: new Array(count), shadowOpacity: new Array(count),
        boxRx: 0, boxRy: 0, lidAngle: 0, lidLift: 0, lidOpacity: 1, leanFraction: 1
    };
    for (var i = 0; i < count; ++i) {
        var r = Math.floor(i / columnCount);
        var c = i % columnCount;
        state.dice.push({
            cx: (c - (columnCount - 1) / 2) * layout.pitch,
            cy: ((rowCount - 1) / 2 - r) * layout.pitch,
            // How it sits in the box before the lid goes: a little turned and
            // tipped, a little off its cell.
            baseX: (rand() - 0.5) * layout.gap * 0.6,
            baseY: (rand() - 0.5) * layout.gap * 0.6,
            baseYaw: (rand() - 0.5) * 16,
            baseTipX: (rand() - 0.5) * 8,
            baseTipY: (rand() - 0.5) * 8,
            // How it rattles: a rate (Hz) and a phase for each motion.
            rate: [3.5 + rand() * 3.5, 3.5 + rand() * 3.5, 5 + rand() * 4, 3 + rand() * 4, 3 + rand() * 4, 3 + rand() * 4],
            phase: [rand() * 6.2832, rand() * 6.2832, rand() * 6.2832, rand() * 6.2832, rand() * 6.2832, rand() * 6.2832],
            // How it falls: the spin it carries down, its time and its click.
            spinX: (rand() < 0.5 ? -1 : 1) * (40 + rand() * 70),
            spinY: (rand() < 0.5 ? -1 : 1) * (40 + rand() * 70),
            fallMs: fallMinMs + rand() * (fallMaxMs - fallMinMs),
            startMs: 0,
            pitchValue: 0.9 + rand() * 0.25,
            landed: false,
            // Which face is up at the end (the board's), and how the die lies
            // in the box: some face up, turned a quarter turn this way or that.
            target: faceUp(board.up && board.up[i] >= 0 && board.up[i] < sides.length ? board.up[i] : 0, 0),
            start: faceUp(Math.floor(faceRand() * sides.length), Math.floor(faceRand() * 4)),
            turn: null,
            flipping: false,
            flipFrom: null,
            flipTo: null,
            flipStart: 0,
            flipLength: 0,
            faceRng: random((seed | 0) * 7919 + i * 104729 + 31)
        });
        state.dice[i].turn = state.dice[i].start;
    }
    // The order they fall in: a seeded shuffle, the same for the same seed.
    var order = [];
    for (var n = 0; n < count; ++n) {
        order.push(n);
    }
    for (var k = count - 1; k > 0; --k) {
        var j = Math.floor(rand() * (k + 1));
        var swap = order[k];
        order[k] = order[j];
        order[j] = swap;
    }
    var startGap = Math.min(dropGapMaxMs, dropGapSpanMs / Math.max(1, count));
    var end = 0;
    for (var m = 0; m < count; ++m) {
        var die = state.dice[order[m]];
        die.startMs = m * startGap;
        end = Math.max(end, die.startMs + die.fallMs);
    }
    state.dropMs = state.reduced ? dropFadeMs : end;
    state.seed = seed;
    state.useMock = layout.mock !== false;      // no device: the seeded stand-in
    state.mockBurstMs = layout.mockBurstMs || 0;
    state.mode = 'staged';
    if (layout.weighted && !layout.reduced) {
        initWeighted(state, board, seed, layout);
    }
    evaluate(state);
    return state;
}

// ---- The device's motion -----------------------------------------------------
// Samples are the accelerometer's specific force in g (what it reads: gravity
// included, so a device lying face up reads 0, 0, 1; x to the right, y up the
// screen, z out of the screen) and the gyroscope's rate in rad/s.
var sampleMs = 10;                // the nominal interval, 100 samples a second
var shakeStillLevel = 0.25;       // g of "hand" (gravity taken out) below which it is not being shaken
var shakeStartLevel = 0.8;        // and above which a shake has started

function newTracker() {
    return { queue: [], f: [0, 0, 1], w: [0, 0, 0], wPrev: [0, 0, 0], g: [0, 0, 1], energy: 0, level: 0, count: 0 };
}

function feed(tracker, ax, ay, az, wx, wy, wz) {
    tracker.queue.push({ f: [ax, ay, az], w: [wx, wy, wz] });
    if (tracker.queue.length > 256) {
        tracker.queue.shift();
    }
    tracker.f = [ax, ay, az];
    tracker.wPrev = tracker.w;
    tracker.w = [wx, wy, wz];
    // Gravity is what the reading settles to; the rest is the hand.
    for (var i = 0; i < 3; ++i) {
        tracker.g[i] += (tracker.f[i] - tracker.g[i]) * 0.02;
    }
    var ux = ax - tracker.g[0];
    var uy = ay - tracker.g[1];
    var uz = az - tracker.g[2];
    tracker.energy += (ux * ux + uy * uy + uz * uz - tracker.energy) * 0.08;
    tracker.level = Math.sqrt(tracker.energy);
    tracker.count += 1;
}

function resetTracker(tracker) {
    tracker.queue = [];
    tracker.f = [0, 0, 1];
    tracker.w = [0, 0, 0];
    tracker.wPrev = [0, 0, 0];
    tracker.g = [0, 0, 1];
    tracker.energy = 0;
    tracker.level = 0;
}

// A uniform number in [0, 1) from a seed, a sample number and a channel: the
// stand-in's noise, so that it is a function of the seed and the time.
function hash(seed, k, channel) {
    var h = Math.imul((seed | 0) ^ 0x9E3779B9, 0x85EBCA6B) ^ Math.imul(k | 0, 0xC2B2AE35)
        ^ Math.imul(channel + 1, 0x27D4EB2F);
    h ^= h >>> 15;
    h = Math.imul(h, 0x2C1B3C6D);
    h ^= h >>> 12;
    h = Math.imul(h, 0x297A2A67);
    h ^= h >>> 15;
    return (h >>> 0) / 4294967296;
}

// The stand-in for a hand shaking a device: sample number k of the stream for a
// seed. A burst of strokes (a few hertz, with a third harmonic so the turns are
// sharp) on all three axes, a slow tilt of the device, sharp spikes, a little
// noise on every sample, and a twist on the gyroscope that goes with the
// strokes. burstMs of 0 is a burst of 0.7 to 1.0 s from the seed.
function mockSampleAt(seed, k, burstMs) {
    var t = k * sampleMs / 1000;
    var burst = (burstMs > 0 ? burstMs : 700 + 300 * hash(seed, 0, 90)) / 1000;
    var two = 2 * Math.PI;
    var env = 0;
    if (t < burst) {
        env = smooth(t / 0.1) * (1 - smooth((t - (burst - 0.12)) / 0.12));
    }
    var f1 = 4.2 + 1.8 * hash(seed, 0, 91);
    var f2 = 3.4 + 1.6 * hash(seed, 0, 92);
    var f3 = 6.0 + 2.0 * hash(seed, 0, 93);
    var p = [];
    for (var i = 0; i < 9; ++i) {
        p.push(two * hash(seed, 0, 100 + i));
    }
    var ax = env * 2.4 * (0.75 * Math.sin(two * f1 * t + p[0]) + 0.25 * Math.sin(two * 3 * f1 * t + p[3]));
    var ay = env * 2.0 * (0.70 * Math.sin(two * f2 * t + p[1]) + 0.30 * Math.sin(two * 2.6 * f2 * t + p[4]));
    var az = env * 1.8 * (0.8 * Math.sin(two * f3 * t + p[2]) + 0.2 * Math.sin(two * 2.7 * f3 * t + p[6]));
    // Spikes: a sample, and half of it on the next.
    for (var back = 0; back < 2; ++back) {
        var kk = k - back;
        if (kk >= 0 && hash(seed, kk, 3) < 0.09 * env) {
            var magnitude = (0.8 + 1.4 * hash(seed, kk, 4)) * (hash(seed, kk, 5) < 0.5 ? -1 : 1) * (back === 0 ? 1 : 0.5);
            var axis = Math.floor(hash(seed, kk, 6) * 3);
            if (axis === 0) { ax += magnitude; } else if (axis === 1) { ay += magnitude; } else { az += magnitude; }
        }
    }
    // The device tips a little as it is shaken.
    var roll = 0.17 * Math.sin(two * 0.6 * t + p[5]);
    var pitch = 0.14 * Math.sin(two * 0.45 * t + p[6]);
    var gx = -Math.sin(roll);
    var gy = Math.sin(pitch);
    var gz = Math.sqrt(Math.max(0, 1 - gx * gx - gy * gy));
    var noise = 0.05;
    var f = [gx + ax + noise * (hash(seed, k, 7) * 2 - 1),
             gy + ay + noise * (hash(seed, k, 8) * 2 - 1),
             gz + az + noise * (hash(seed, k, 9) * 2 - 1)];
    var w = [env * 2.6 * Math.sin(two * f2 * t + p[7]) + 0.04 * (hash(seed, k, 10) * 2 - 1),
             env * 2.2 * Math.sin(two * f1 * t + p[8]) + 0.04 * (hash(seed, k, 11) * 2 - 1),
             env * 1.4 * Math.sin(two * f3 * t + p[0]) + 0.04 * (hash(seed, k, 12) * 2 - 1)];
    return { f: f, w: w };
}

// The samples of the stand-in stream that fall in (fromMs, toMs], fed in order.
function mockFill(tracker, seed, burstMs, fromMs, toMs) {
    var first = Math.floor(fromMs / sampleMs) + 1;
    var last = Math.floor(toMs / sampleMs);
    for (var k = first; k <= last; ++k) {
        var sample = mockSampleAt(seed, k, burstMs);
        feed(tracker, sample.f[0], sample.f[1], sample.f[2], sample.w[0], sample.w[1], sample.w[2]);
    }
}

// What the staged script takes from the stream: how hard (0 to 1) and which
// way the device is tipped (each -1 to 1).
function agitationOf(tracker) {
    return Math.min(1, tracker.level / 1.2);
}

function tiltXOf(tracker) {
    return Math.max(-1, Math.min(1, -tracker.g[0] / 0.14));
}

function tiltYOf(tracker) {
    return Math.max(-1, Math.min(1, tracker.g[1] / 0.14));
}

// ---- Weighted motion ---------------------------------------------------------
// Lengths are in tiles (one die's edge, taken as 20 mm), times in seconds,
// angles in degrees. A tile is 20 mm, so 1 g is 490 tiles/s^2 and time is real:
// a die falls a tile in 64 ms and a hop of a tile and a half is a sixth of a
// second. Slowing time would make a shake at a few hertz too quick for the dice
// to follow, which is the opposite of heavy.
var simStepMs = 1000 / 480;
var gTiles = 9810 / 20;           // tiles/s^2 in 1 g
var cavitySlack = 0.2;            // how far a die may stray from the middle of its cell
var dieRadius = 0.52;
var slotStiffness = 700;          // the funnel into a cell, 1/s^2
var slotDamping = 45;
var restitutionWall = 0.4;
var restitutionDie = 0.45;
var restitutionLid = 0.3;
var floorFriction = 0.3;
var bounceSpeed = 8;              // tiles/s of impact that spins a die fully

function wrapDegrees(a) {
    return ((a + 180) % 360 + 360) % 360 - 180;
}

function initWeighted(state, board, seed, layout) {
    state.mode = 'weighted';
    state.simAcc = 0;
    state.simSteps = 0;
    state.ceiling = layout.lidTiles - 0.03 - 0.5;   // the lid's underside, to a die's centre
    state.lastStartMs = 0;
    state.f = [0, 0, 1];
    state.w = [0, 0, 0];
    state.alpha = [0, 0, 0];
    state.tipX = 0;
    state.tipY = 0;
    state.vibe = 0;
    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        state.lastStartMs = Math.max(state.lastStartMs, d.startMs);
        d.x = d.cx / layout.tile;
        d.y = d.cy / layout.tile;
        d.z = 0.5;
        d.vx = 0; d.vy = 0; d.vz = 0;
        d.psi = 0; d.wpsi = 0;
        d.phx = 0; d.wphx = 0;
        d.phy = 0; d.wphy = 0;
        d.bounce = 0.30 + 0.14 * ((d.pitchValue - 0.9) / 0.25);   // 0.30 to 0.44
        d.rng = random((seed | 0) * 31 + i * 7919 + 17);
        d.active = false;       // the cell's funnel is on
        d.landed = false;
    }
}

// How a die turns over, in seconds, while it is thrown about (a quarter turn,
// or a half turn for a hard knock) and as it slots in (to its up face, whatever
// it has been through); and how hard a knock turns it, in tiles/s.
var tumbleSeconds = 0.09;
var tumbleMinSpeed = 3.5;
var tumbleHalfTurnSpeed = 14;
var turnToFaceSeconds = 0.2;

// Where the die is in a turn, at time ts.
function turnPose(d, ts) {
    if (!d.flipping) {
        return d.turn;
    }
    return quatSlerp(d.flipFrom, d.flipTo, smooth((ts - d.flipStart) / d.flipLength));
}

// A die that is being thrown about has hit something at `speed`: it turns
// over, about a horizontal axis and the way it was going (straight down, by a
// coin toss), so that another face is up. One turn at a time: a knock that
// comes while it is turning is another knock, not another turn.
function tumble(d, speed) {
    if (d.active || d.flipping || speed < tumbleMinSpeed) {
        return;
    }
    var quarters = speed >= tumbleHalfTurnSpeed ? 2 : 1;
    var around;
    var sign;
    if (Math.abs(d.vx) + Math.abs(d.vy) < 0.4) {
        around = d.faceRng() < 0.5 ? 0 : 1;
        sign = d.faceRng() < 0.5 ? -1 : 1;
    } else if (Math.abs(d.vy) >= Math.abs(d.vx)) {
        around = 0;                       // going along y: about the x axis
        sign = d.vy > 0 ? -1 : 1;
    } else {
        around = 1;                       // going along x: about the y axis
        sign = d.vx > 0 ? 1 : -1;
    }
    var turn = around === 0 ? quatAxis(1, 0, 0, sign * 90 * quarters)
                            : quatAxis(0, 1, 0, sign * 90 * quarters);
    d.flipFrom = d.turn;
    d.flipTo = quatMultiply(turn, d.turn);
    d.flipStart = d.now;
    d.flipLength = tumbleSeconds * (quarters === 2 ? 1.4 : 1);
    d.flipping = true;
}

// The die is slotting in: from however it lies now, it turns to its up face.
function turnToFace(d, ts) {
    var from = turnPose(d, ts);
    d.flipping = false;
    d.turn = from;
    if (quatSame(from, d.target)) {
        return;
    }
    d.flipFrom = from;
    d.flipTo = d.target;
    d.flipStart = ts;
    d.flipLength = turnToFaceSeconds;
    d.flipping = true;
}

// A die has hit something at `speed`: it spins a little, more the harder, and
// turns over.
function kick(d, speed) {
    var amount = Math.min(1, speed / bounceSpeed);
    d.wpsi += (d.rng() * 2 - 1) * 240 * amount;
    d.wphx += (d.rng() * 2 - 1) * 180 * amount;
    d.wphy += (d.rng() * 2 - 1) * 180 * amount;
    tumble(d, speed);
}

// One fixed step of the box's frame. `f` is the accelerometer's reading in g,
// `w` and `alpha` the gyroscope's rate and its change.
function stepWeighted(state) {
    var h = simStepMs / 1000;
    var ts = state.simSteps * h;
    var rest = 0.5;
    var settling = state.stage === 'settle';
    var ceiling = state.stage === 'shake' ? state.ceiling : 4;
    var inStage = state.clock - state.stageStart;
    // Once the lid is lifting the device is put down: the reading fades to flat.
    var follow = state.stage === 'shake' ? 1
        : (state.stage === 'lid' ? 1 - smooth(inStage / lidMs) : 0);
    var fx = state.f[0] * follow;
    var fy = state.f[1] * follow;
    var fz = 1 + (state.f[2] - 1) * follow;
    var wx = state.w[0] * follow, wy = state.w[1] * follow, wz = state.w[2] * follow;
    var alx = state.alpha[0] * follow, aly = state.alpha[1] * follow, alz = state.alpha[2] * follow;
    var restSpeed = 3 * Math.max(0.2, fz) * gTiles * h;

    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        d.now = ts;
        if (d.flipping && ts >= d.flipStart + d.flipLength) {
            d.turn = d.flipTo;
            d.flipping = false;
        }
        if (settling && !d.active && inStage >= d.startMs) {
            d.active = true;
            d.psi = wrapDegrees(d.psi);
            turnToFace(d, ts);
        }
        var cellX = d.cx / state.tile;
        var cellY = d.cy / state.tile;
        // What the dice feel in the box's frame: minus what the accelerometer
        // reads, and the pseudo-forces of the box's spin (centrifugal, Coriolis,
        // Euler), the position taken from the middle of the box's floor.
        var rx = d.x, ry = d.y, rz = d.z;
        var cfx = -(wy * (wx * ry - wy * rx) - wz * (wz * rx - wx * rz));
        var cfy = -(wz * (wy * rz - wz * ry) - wx * (wx * ry - wy * rx));
        var cfz = -(wx * (wz * rx - wx * rz) - wy * (wy * rz - wz * ry));
        var cox = -2 * (wy * d.vz - wz * d.vy);
        var coy = -2 * (wz * d.vx - wx * d.vz);
        var coz = -2 * (wx * d.vy - wy * d.vx);
        var eux = -(aly * rz - alz * ry);
        var euy = -(alz * rx - alx * rz);
        var euz = -(alx * ry - aly * rx);
        var ax = -fx * gTiles + cfx + cox + eux + 10 * (cellX - d.x) - 0.6 * d.vx;
        var ay = -fy * gTiles + cfy + coy + euy + 10 * (cellY - d.y) - 0.6 * d.vy;
        var az = -fz * gTiles + cfz + coz + euz;
        if (d.active) {
            ax += slotStiffness * (cellX - d.x) - slotDamping * d.vx;
            ay += slotStiffness * (cellY - d.y) - slotDamping * d.vy;
        }
        d.vx += ax * h;
        d.vy += ay * h;
        d.vz += az * h;
        d.x += d.vx * h;
        d.y += d.vy * h;
        d.z += d.vz * h;

        // The floor, with friction against the weight on it.
        var onFloor = false;
        if (d.z <= rest) {
            d.z = rest;
            onFloor = true;
            if (d.vz < 0) {
                var impact = -d.vz;
                if (impact < restSpeed) {
                    d.vz = 0;
                } else {
                    d.vz = impact * d.bounce;
                    kick(d, impact);
                    if (impact > 4 && d.active && !d.landed) {
                        d.landed = true;
                        state.events.push({ kind: 'landed', index: i, pitch: d.pitchValue });
                    }
                }
            }
            var slide = Math.sqrt(d.vx * d.vx + d.vy * d.vy);
            if (slide > 0) {
                var drop = (d.active ? 0 : 1) * floorFriction * Math.max(0, fz) * gTiles * h;
                var scale = slide > drop ? (slide - drop) / slide : 0;
                d.vx *= scale;
                d.vy *= scale;
            }
        }
        // The lid, while it is on.
        if (d.z > ceiling) {
            d.z = ceiling;
            if (d.vz > 0) {
                kick(d, d.vz);
                d.vz = -d.vz * restitutionLid;
            }
        }
        // The walls of its pocket: the box is a grid of loose cells, so a die
        // can slide and bump its neighbours but cannot trade places with one.
        var lowX = cellX - cavitySlack, highX = cellX + cavitySlack;
        var lowY = cellY - cavitySlack, highY = cellY + cavitySlack;
        if (d.x > highX) { d.x = highX; if (d.vx > 0) { kick(d, d.vx * 0.5); d.vx = -d.vx * restitutionWall; } }
        if (d.x < lowX) { d.x = lowX; if (d.vx < 0) { kick(d, -d.vx * 0.5); d.vx = -d.vx * restitutionWall; } }
        if (d.y > highY) { d.y = highY; if (d.vy > 0) { kick(d, d.vy * 0.5); d.vy = -d.vy * restitutionWall; } }
        if (d.y < lowY) { d.y = lowY; if (d.vy < 0) { kick(d, -d.vy * 0.5); d.vy = -d.vy * restitutionWall; } }

        // Turning: yaw is free but for friction, and funnelled once slotting;
        // the tips flop flat on the floor and spin on in the air.
        d.psi += d.wpsi * h;
        d.wpsi *= 1 - (onFloor ? 10 : 1.5) * h;
        d.phx += d.wphx * h;
        d.phy += d.wphy * h;
        if (onFloor || d.active) {
            d.wphx += (-500 * d.phx - 26 * d.wphx) * h;
            d.wphy += (-500 * d.phy - 26 * d.wphy) * h;
        } else {
            d.wphx *= 1 - 1.2 * h;
            d.wphy *= 1 - 1.2 * h;
        }
        if (d.active) {
            d.wpsi += (-500 * d.psi - 38 * d.wpsi) * h;
        }
        d.phx = Math.max(-75, Math.min(75, d.phx));
        d.phy = Math.max(-75, Math.min(75, d.phy));
    }

    // Dice push one another apart (two dice at about the same height).
    var reach = 2 * dieRadius;
    for (var a = 0; a < state.count; ++a) {
        var da = state.dice[a];
        for (var b = a + 1; b < state.count; ++b) {
            var db = state.dice[b];
            if (Math.abs(da.z - db.z) > 0.9) {
                continue;
            }
            var dx = db.x - da.x;
            var dy = db.y - da.y;
            var distance = Math.sqrt(dx * dx + dy * dy);
            if (distance >= reach) {
                continue;
            }
            var nx = distance > 1e-6 ? dx / distance : 1;
            var ny = distance > 1e-6 ? dy / distance : 0;
            var push = (reach - distance) / 2;
            da.x -= nx * push; da.y -= ny * push;
            db.x += nx * push; db.y += ny * push;
            var approach = (da.vx - db.vx) * nx + (da.vy - db.vy) * ny;
            if (approach > 0) {
                var j = (1 + restitutionDie) * approach / 2;
                da.vx -= j * nx; da.vy -= j * ny;
                db.vx += j * nx; db.vy += j * ny;
                kick(da, approach * 0.5);
                kick(db, approach * 0.5);
            }
        }
    }
    state.simSteps += 1;
}

function advanceWeighted(state, dtMs, tracker) {
    state.clock += dtMs;
    var inStage = state.clock - state.stageStart;
    if (state.stage === 'shake') {
        state.tickAccumulator += dtMs;
        if (state.tickAccumulator >= rattleTickMs) {
            state.tickAccumulator -= rattleTickMs;
            state.events.push({ kind: 'rattled' });
        }
        if (inStage >= shakeMinMs && (tracker.level < shakeStillLevel || inStage >= shakeMaxMs)) {
            enter(state, 'lid');
        }
    } else if (state.stage === 'lid') {
        if (inStage >= lidMs) {
            enter(state, 'settle');
        }
    }
    // The samples that arrived this frame, spread over it: the physics takes
    // each as the reading from its place in the frame to the next.
    var samples = tracker.queue;
    tracker.queue = [];
    var steps = Math.max(1, Math.round((state.simAcc + dtMs) / simStepMs));
    var done = 0;
    state.simAcc += dtMs;
    while (state.simAcc >= simStepMs) {
        state.simAcc -= simStepMs;
        done += 1;
        if (samples.length > 0) {
            var index = Math.min(samples.length - 1, Math.floor(done / steps * samples.length));
            var sample = samples[index];
            var previousW = state.w;
            state.f = sample.f;
            state.w = sample.w;
            state.alpha = [Math.max(-100, Math.min(100, (sample.w[0] - previousW[0]) / (sampleMs / 1000))),
                           Math.max(-100, Math.min(100, (sample.w[1] - previousW[1]) / (sampleMs / 1000))),
                           Math.max(-100, Math.min(100, (sample.w[2] - previousW[2]) / (sampleMs / 1000)))];
        }
        stepWeighted(state);
    }
    // What the box shows of the device: how it is tipped, and a shiver.
    state.tipX = Math.asin(Math.max(-1, Math.min(1, state.f[1] / Math.max(0.2, Math.sqrt(state.f[0] * state.f[0] + state.f[1] * state.f[1] + state.f[2] * state.f[2]))))) * 180 / Math.PI;
    state.tipY = Math.asin(Math.max(-1, Math.min(1, -state.f[0] / Math.max(0.2, Math.sqrt(state.f[0] * state.f[0] + state.f[1] * state.f[1] + state.f[2] * state.f[2]))))) * 180 / Math.PI;
    state.vibe = tracker.level;
    if (state.stage === 'settle') {
        var settleFor = state.clock - state.stageStart;
        var still = true;
        for (var i = 0; i < state.count && still; ++i) {
            var d = state.dice[i];
            var cellX = d.cx / state.tile;
            var cellY = d.cy / state.tile;
            still = d.active
                && !d.flipping
                && Math.abs(d.x - cellX) < 0.004 && Math.abs(d.y - cellY) < 0.004
                && Math.abs(d.z - 0.5) < 0.004
                && Math.abs(d.vx) < 0.05 && Math.abs(d.vy) < 0.05 && Math.abs(d.vz) < 0.05
                && Math.abs(d.psi) < 0.3 && Math.abs(d.phx) < 0.3 && Math.abs(d.phy) < 0.3;
        }
        if (still || settleFor >= state.lastStartMs + 700) {
            for (var k = 0; k < state.count; ++k) {
                var e = state.dice[k];
                e.x = e.cx / state.tile; e.y = e.cy / state.tile; e.z = 0.5;
                e.vx = 0; e.vy = 0; e.vz = 0;
                e.psi = 0; e.phx = 0; e.phy = 0;
                e.flipping = false;
                e.turn = e.target;
                if (!e.landed) {
                    e.landed = true;
                    state.events.push({ kind: 'landed', index: k, pitch: e.pitchValue });
                }
            }
            enter(state, 'idle');
            state.events.push({ kind: 'settled' });
        }
    }
}

function evaluateWeighted(state) {
    var tile = state.tile;
    var inStage = state.clock - state.stageStart;
    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        var height = Math.max(0, d.z - 0.5);
        var airborne = Math.min(2, height);
        state.px[i] = d.x * tile;
        state.py[i] = d.y * tile;
        state.pz[i] = d.z * tile;
        state.rx[i] = d.phx;
        state.ry[i] = d.phy;
        state.rz[i] = d.psi;
        // Which face is up: at rest, the board's; before that, whichever the
        // knocks have left it on.
        var turn = state.stage === 'idle' ? d.target : turnPose(d, state.simSteps * simStepMs / 1000);
        state.tw[i] = turn[0];
        state.tx[i] = turn[1];
        state.ty[i] = turn[2];
        state.tz[i] = turn[3];
        state.opacity[i] = 1;
        state.shadowRadius[i] = tile * 0.47 * (1 - 0.1 * Math.min(1.5, airborne));
        state.shadowOpacity[i] = 0.34 / (1 + 1.2 * airborne);
    }
    // The box is shown tipped the way the device is, up to about eight degrees,
    // with a shiver in proportion to how hard it is being shaken.
    var shiver = state.stage === 'shake' ? Math.min(1, state.vibe / 1.2) : 0;
    var seconds = state.clock / 1000;
    var limit = 10;
    state.boxRx = Math.max(-limit, Math.min(limit, state.tipX + shiver * 1.2 * Math.sin(6.2832 * 23 * seconds)));
    state.boxRy = Math.max(-limit, Math.min(limit, state.tipY + shiver * 1.2 * Math.sin(6.2832 * 19 * seconds + 1.3)));
    // The lid.
    var lidU = state.stage === 'shake' ? 0 : (state.stage === 'lid' ? inStage / lidMs : 1);
    var lift = smooth(lidU);
    state.lidAngle = -72 * lift;
    state.lidLift = tile * 0.9 * lift;
    state.lidOpacity = state.stage === 'shake' ? 1
        : (state.stage === 'lid' ? 1 - smooth((lidU - 0.55) / 0.45) : 0);
    // The camera comes upright as the dice slot in.
    if (state.stage === 'shake' || state.stage === 'lid') {
        state.leanFraction = 1;
    } else if (state.stage === 'settle') {
        state.leanFraction = 1 - smooth(inStage / (state.lastStartMs + 400));
    } else {
        state.leanFraction = 0;
    }
}

function enter(state, name) {
    state.stage = name;
    state.stageStart = state.clock;
}

// A play that has already ended: the dice down, nothing moving.
function finish(state) {
    enter(state, "idle");
    evaluate(state);
}

// Moves the script on by dtMs of the device's motion.
// Moves the script on by dtMs. The tracker is the device's motion: samples fed
// to it are what the dice answer to. With no device (layout.mock) the seeded
// stand-in feeds it.
function advance(state, dtMs, tracker) {
    if (state.useMock) {
        mockFill(tracker, state.seed, state.mockBurstMs, state.clock, state.clock + dtMs);
    }
    if (state.mode === 'weighted') {
        advanceWeighted(state, dtMs, tracker);
        return;
    }
    // Staged: how hard and which way the device is tipped, from the stream.
    tracker.queue = [];
    advanceStaged(state, dtMs, agitationOf(tracker), tiltXOf(tracker), tiltYOf(tracker));
}

// Moves the staged script on by dtMs of the device's motion.
function advanceStaged(state, dtMs, agitation, tiltX, tiltY) {
    state.clock += dtMs;
    state.smoothAgitation += (agitation - state.smoothAgitation) * Math.min(1, dtMs / 90);
    var inStage = state.clock - state.stageStart;
    if (state.stage === "shake") {
        state.tickAccumulator += dtMs;
        if (!state.reduced && state.tickAccumulator >= rattleTickMs) {
            state.tickAccumulator -= rattleTickMs;
            state.events.push({ kind: "rattled" });
        }
        var limit = state.reduced ? shakeStillMs : shakeMaxMs;
        var least = state.reduced ? shakeStillMs : shakeMinMs;
        if (inStage >= least && (state.reduced || agitation < 0.15 || inStage >= limit)) {
            enter(state, "lid");
        }
    } else if (state.stage === "lid") {
        if (inStage >= (state.reduced ? lidFadeMs : lidMs)) {
            enter(state, "drop");
        }
    } else if (state.stage === "drop") {
        if (!state.reduced) {
            for (var i = 0; i < state.count; ++i) {
                var die = state.dice[i];
                if (!die.landed && inStage >= die.startMs + die.fallMs * 0.78) {
                    die.landed = true;
                    state.events.push({ kind: "landed", index: i, pitch: die.pitchValue });
                }
            }
        }
        if (inStage >= state.dropMs) {
            enter(state, state.reduced ? "idle" : "settle");
            if (state.reduced) {
                state.events.push({ kind: "settled" });
            }
        }
    } else if (state.stage === "settle") {
        if (inStage >= settleMs) {
            enter(state, "idle");
            state.events.push({ kind: "settled" });
        }
    }
    state.tiltX = tiltX;
    state.tiltY = tiltY;
}

// Writes the pose of everything at the script's present time.
function evaluate(state) {
    if (state.mode === 'weighted') {
        evaluateWeighted(state);
        return;
    }
    var tile = state.tile;
    var rest = tile / 2;
    var seconds = state.clock / 1000;
    var inStage = state.clock - state.stageStart;
    var energy = 0.55 + 0.45 * state.smoothAgitation;
    var tiltX = state.tiltX;
    var tiltY = state.tiltY;

    // Where each part of the sequence is: how much rattle there still is
    // (calm), how far up the dice have been lifted into the box (lifted), how
    // far through the lid, the drop and the settle.
    var calm = 0;
    var lifted = 0;
    var lidU = 1;
    var dropMs = -1;
    var settleU = 1;
    var resting = false;
    switch (state.stage) {
    case "shake":
        calm = state.reduced ? 0 : Math.min(1, inStage / 120);
        lifted = state.reduced ? 0 : smooth(inStage / 150);
        lidU = 0;
        break;
    case "lid":
        calm = state.reduced ? 0 : 1 - smooth(inStage / lidMs);
        lifted = state.reduced ? 0 : 1;
        lidU = inStage / (state.reduced ? lidFadeMs : lidMs);
        break;
    case "drop":
        lifted = state.reduced ? 0 : 1;
        dropMs = inStage;
        break;
    case "settle":
        lifted = 1;
        dropMs = Infinity;
        settleU = inStage / settleMs;
        break;
    default:
        resting = true;
        dropMs = Infinity;
        break;
    }

    var amplitude = calm * energy;
    for (var i = 0; i < state.count; ++i) {
        var die = state.dice[i];
        var x = die.cx;
        var y = die.cy;
        var z = rest;
        var rx = 0;
        var ry = 0;
        var rz = 0;
        var alpha = 1;
        var airborne = 0;     // 0 on the floor, 1 at the height of the box
        var turn = die.target;   // which face is up: the board's, but in flight

        if (state.reduced) {
            // Nothing moves: the dice are on their cells and fade in once the
            // lid has gone.
            alpha = state.stage === "drop" ? smooth(inStage / dropFadeMs)
                : (state.stage === "settle" || resting ? 1 : 0);
        } else if (dropMs < 0 || (dropMs !== Infinity && dropMs < die.startMs)) {
            // In the box, on whichever face it was left.
            turn = die.start;
            var rate = die.rate;
            var phase = die.phase;
            x += die.baseX + amplitude * state.gap * 0.35 * Math.sin(6.2832 * rate[0] * seconds + phase[0]) + tiltX * tile * 0.1;
            y += die.baseY + amplitude * state.gap * 0.35 * Math.sin(6.2832 * rate[1] * seconds + phase[1]) - tiltY * tile * 0.1;
            z = rest + lifted * (tile + amplitude * tile * 0.09 * Math.sin(6.2832 * rate[2] * seconds + phase[2]));
            rz = die.baseYaw + amplitude * 7 * Math.sin(6.2832 * rate[3] * seconds + phase[3]);
            rx = die.baseTipX + amplitude * 4 * Math.sin(6.2832 * rate[4] * seconds + phase[4]);
            ry = die.baseTipY + amplitude * 4 * Math.sin(6.2832 * rate[5] * seconds + phase[5]);
            airborne = lifted;
        } else if (dropMs !== Infinity && dropMs < die.startMs + die.fallMs) {
            // Falling.
            var u = (dropMs - die.startMs) / die.fallMs;
            var contact = 0.78;
            var w = Math.min(1, u / contact);
            var hoverZ = rest + tile;
            x = die.cx + (die.baseX + tiltX * tile * 0.1) * (1 - w);
            y = die.cy + (die.baseY - tiltY * tile * 0.1) * (1 - w);
            if (u < contact) {
                z = rest + tile * (1 - w * w);
            } else {
                // A small overshoot: a rebound of an eighth of the fall.
                var v = (u - contact) / (1 - contact);
                z = rest + tile * 0.125 * Math.sin(Math.PI * v) * (1 - v);
            }
            rz = die.baseYaw * (1 - 0.65 * Math.min(1, u));
            rx = die.baseTipX * (1 - w) + die.spinX * Math.sin(Math.PI * w) * (1 - 0.3 * w);
            ry = die.baseTipY * (1 - w) + die.spinY * Math.sin(Math.PI * w) * (1 - 0.3 * w);
            airborne = Math.max(0, (z - rest) / (hoverZ - rest));
            // It turns over as it falls, and has its face up when it lands.
            turn = quatSlerp(die.start, die.target, smooth(w));
        } else {
            // Down. The letters straighten through the settle.
            rz = die.baseYaw * 0.35 * (1 - smooth(settleU));
        }
        state.px[i] = x;
        state.py[i] = y;
        state.pz[i] = z;
        state.rx[i] = rx;
        state.ry[i] = ry;
        state.rz[i] = rz;
        state.tw[i] = turn[0];
        state.tx[i] = turn[1];
        state.ty[i] = turn[2];
        state.tz[i] = turn[3];
        state.opacity[i] = alpha;
        // A shadow that is smaller and fainter the higher the die is.
        state.shadowRadius[i] = tile * 0.47 * (1 - 0.12 * airborne);
        state.shadowOpacity[i] = alpha * 0.34 / (1 + 1.2 * airborne);
    }

    // The box: a rattle of about eight degrees that dies away as the lid goes,
    // plus the lean the device is tilted to.
    var rattleX = amplitude * boxDegrees * 0.8 * Math.sin(6.2832 * 5.3 * seconds);
    var rattleY = amplitude * boxDegrees * 0.8 * Math.sin(6.2832 * 4.1 * seconds + 1.3);
    var softX = state.reduced && state.stage === "shake"
        ? 2 * Math.sin(6.2832 * inStage / shakeStillMs) * (1 - inStage / shakeStillMs) : 0;
    state.boxRx = Math.max(-boxDegrees, Math.min(boxDegrees, rattleX - tiltY * 5 + softX));
    state.boxRy = Math.max(-boxDegrees, Math.min(boxDegrees, rattleY + tiltX * 5));

    // The lid lifts and tips away (hinged at the far edge); reduced motion fades
    // it instead.
    if (state.reduced) {
        state.lidAngle = 0;
        state.lidLift = 0;
        state.lidOpacity = state.stage === "shake" ? 1
            : (state.stage === "lid" ? 1 - smooth(lidU) : 0);
    } else {
        var lift = smooth(lidU);
        state.lidAngle = -72 * lift;
        state.lidLift = tile * 0.9 * lift;
        state.lidOpacity = state.stage === "shake" ? 1
            : (state.stage === "lid" ? 1 - smooth((lidU - 0.55) / 0.45) : 0);
    }

    // The camera comes upright over the drop and the settle. With reduced motion
    // it is upright throughout: nothing moves.
    if (state.reduced) {
        state.leanFraction = 0;
    } else if (state.stage === "shake" || state.stage === "lid") {
        state.leanFraction = 1;
    } else if (state.stage === "drop") {
        state.leanFraction = state.dropMs > 0 ? 1 - smooth(inStage / (state.dropMs + settleMs)) : 1;
    } else if (state.stage === "settle") {
        state.leanFraction = 1 - smooth((state.dropMs + inStage) / (state.dropMs + settleMs));
    } else {
        state.leanFraction = 0;
    }
}

// The script at `ms` into a play, the device still.
function at(board, seed, layout, ms) {
    var state = create(board, seed, layout);
    var tracker = newTracker();
    var frameMs = 1000 / 60;
    for (var t = 0; t < ms; t += frameMs) {
        advance(state, Math.min(frameMs, ms - t), tracker);
    }
    evaluate(state);
    return state;
}

// What a play of a board and seed does to the faces, frame by frame at 60 Hz
// with the device still: how many dice end with the board's face on top and the
// letter upright, and how many different faces each showed uppermost on the way
// (the sides that were the top at some frame; the tips of a die in flight are
// not counted, so it is the least a viewer saw).
function faceReport(board, seed, layout) {
    var state = create(board, seed, layout);
    var tracker = newTracker();
    var seen = [];
    for (var i = 0; i < state.count; ++i) {
        seen.push({});
    }
    var frames = 0;
    for (; state.stage !== "idle" && frames < 1000; ++frames) {
        advance(state, 1000 / 60, tracker);
        evaluate(state);
        for (var k = 0; k < state.count; ++k) {
            seen[k][topSide([state.tw[k], state.tx[k], state.ty[k], state.tz[k]]).side] = true;
        }
    }
    evaluate(state);
    var landed = 0;
    var total = 0;
    var most = 0;
    for (var d = 0; d < state.count; ++d) {
        var top = topSide([state.tw[d], state.tx[d], state.ty[d], state.tz[d]]);
        if (top.side === board.up[d] && top.tilt < 0.01 && top.height > 0.9999) {
            landed += 1;
        }
        var distinct = Object.keys(seen[d]).length;
        total += distinct;
        most = Math.max(most, distinct);
    }
    return { dice: state.count, landed: landed, facesSeenMean: total / state.count, facesSeenMost: most,
             frames: frames, finished: state.stage === "idle" };
}

// A checksum of every pose a board and seed produce, frame by frame at 60 Hz
// with the device still. The same board, seed and layout give the same string.
function digest(board, seed, layout) {
    var state = create(board, seed, layout);
    var tracker = newTracker();
    var sum = 0x811C9DC5;
    for (var frame = 0; state.stage !== "idle" && frame < 1000; ++frame) {
        advance(state, 1000 / 60, tracker);
        evaluate(state);
        for (var i = 0; i < state.count; ++i) {
            var values = [state.px[i], state.py[i], state.pz[i], state.rx[i], state.ry[i], state.rz[i],
                          state.tw[i], state.tx[i], state.ty[i], state.tz[i]];
            for (var v = 0; v < values.length; ++v) {
                sum = Math.imul(sum ^ (Math.round(values[v] * 100) | 0), 0x01000193);
            }
        }
    }
    return (sum >>> 0).toString(16);
}
