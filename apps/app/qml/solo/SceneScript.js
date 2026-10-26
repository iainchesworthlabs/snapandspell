// SPDX-License-Identifier: GPL-3.0-or-later
.pragma library

// The shake, the lid, the drop and the settle of the board scene (BoardScene.qml), as pure functions of
// the board's size, the seed and the time: no QML and no renderer in here. Toybox is pixel art, so
// nothing moves smoothly: the box jumps in whole steps, the lid lifts in four, and a die falls through
// six flip-book frames (docs/design/toybox/DESIGN.md, "Motion").
//
//   shake   the closed box jumps in stepped hops of 6 px, one every 75 ms (eight in 600 ms); at least
//           600 ms, 3 s at most while the device is still being shaken
//   lid     lifts in four steps, 250 ms
//   drop    each die falls through six frames in 380 ms, started in a seeded order 40 ms after the one
//           before (the gap shrinks on a big board so that the whole drop stays under 700 ms of
//           starts)
//   settle  a short hold, 150 ms, then the board is the flat board's
//
// Reduced motion: the box is still and rattles once, 300 ms; the lid cross-fades, 120 ms; the dice fade
// in together, 120 ms.

var SHAKE_MIN = 600;
var SHAKE_MAX = 3000;
var STEP_MS = 75;            // one hop of the box
var STEPS = 8;               // the hops in the table; a longer shake goes round it again
var JUMP = 6;                // px
var LID = 250;
var LID_STEPS = 4;
var SETTLE = 150;
var GAP = 40;                // ms between the starts of two dice
var GAP_SPREAD = 700;        // ms: the starts of all the dice fit in this
var DROP = 380;
var FRAMES = 6;
var REDUCED_SHAKE = 300;
var REDUCED_LID = 120;
var REDUCED_DROP = 120;
var RATTLE_GAP = 125;        // ms: at most 8 rattles a second
var LANDING_FRAME = 3;       // the frame in which a die meets the floor

// How high the die is, in tiles, in each frame of its fall.
var FALL = [-0.9, -0.6, -0.3, 0, -0.08, 0];

function mulberry32(seed) {
    var a = seed >>> 0;
    return function () {
        a = (a + 0x6D2B79F5) >>> 0;
        var t = a;
        t = Math.imul(t ^ (t >>> 15), t | 1);
        t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

function clamp01(x) {
    return x < 0 ? 0 : (x > 1 ? 1 : x);
}

// Everything that varies between plays of different seeds, drawn once.
function create(count, seed, reduced) {
    var rnd = mulberry32(seed);
    var order = [];
    var i;
    for (i = 0; i < count; ++i) {
        order.push(i);
    }
    for (i = count - 1; i > 0; --i) {
        var j = Math.floor(rnd() * (i + 1));
        var swap = order[i];
        order[i] = order[j];
        order[j] = swap;
    }

    var gap = Math.min(GAP, GAP_SPREAD / Math.max(1, count));
    var script = {
        count: count,
        reduced: reduced,
        order: order,
        start: [],
        pitch: [],
        hopX: [],
        hopY: [],
        dropLength: 0
    };
    for (i = 0; i < count; ++i) {
        script.start.push(0);
        script.pitch.push(0.9 + rnd() * 0.2);
    }
    for (var k = 0; k < count; ++k) {
        var die = order[k];
        script.start[die] = k * gap;
        script.dropLength = Math.max(script.dropLength, script.start[die] + DROP);
    }
    // The hops: each a step of 0 or one jump along each axis, never standing still, and never
    // the same way twice in a row along x (the box is shaken, not pushed).
    var last = 0;
    for (i = 0; i < STEPS; ++i) {
        var hx = (Math.floor(rnd() * 3) - 1) * JUMP;
        var hy = (Math.floor(rnd() * 3) - 1) * JUMP;
        if (hx === last && hx !== 0) {
            hx = -hx;
        }
        if (hx === 0 && hy === 0) {
            hx = (i % 2 === 0 ? 1 : -1) * JUMP;
        }
        last = hx;
        script.hopX.push(hx);
        script.hopY.push(hy);
    }
    return script;
}

// How long a stage lasts, apart from the shake, whose end depends on the device.
function stageLength(script, stage) {
    if (script.reduced) {
        return stage === "shake" ? REDUCED_SHAKE : stage === "lid" ? REDUCED_LID
             : stage === "drop" ? REDUCED_DROP : 0;
    }
    return stage === "shake" ? SHAKE_MIN : stage === "lid" ? LID
         : stage === "drop" ? script.dropLength : stage === "settle" ? SETTLE : 0;
}

// The time at which die `i` meets the floor, in the drop stage.
function landingTime(script, i) {
    if (script.reduced) {
        return REDUCED_DROP;
    }
    return script.start[i] + DROP * LANDING_FRAME / FRAMES;
}

// The frame of its fall die `i` is in at time `t` of the drop: -1 before it starts, 0 to 5 while it
// falls, 6 once it is down.
function frame(script, t, i) {
    var local = t - script.start[i];
    if (local < 0) {
        return -1;
    }
    if (local >= DROP) {
        return FRAMES;
    }
    return Math.floor(local / DROP * FRAMES);
}

// Where a die is, away from its tile: x and y in whole pixels, `fall` (how far above its tile it is, as a
// share of the tile, for a scene that scales the tile itself), its opacity, and the frame it is in.
// A die is not seen while the box is closed, and not before its own turn to fall.
function pose(script, stage, t, i, tile) {
    var p = { x: 0, y: 0, fall: 0, opacity: 1, frame: FRAMES };
    if (stage === "idle" || stage === "settle") {
        return p;
    }
    if (script.reduced) {
        p.opacity = stage === "drop" ? clamp01(t / REDUCED_DROP) : 0;
        return p;
    }
    if (stage === "shake" || stage === "lid") {
        p.opacity = 0;
        p.frame = -1;
        return p;
    }
    var f = frame(script, t, i);
    p.frame = f;
    if (f < 0) {
        p.opacity = 0;
    } else if (f < FRAMES) {
        p.fall = FALL[f];
        p.y = Math.round(FALL[f] * tile);
    }
    return p;
}

// The box's hop at time `t` of the shake: whole pixels. A harder shake (`agitation`, 0 to 1) hops
// faster, and a lean to one side (`lean`, -1 to 1) moves the hops along x. Neither changes where a die
// lands.
function boxHop(script, stage, t, agitation, lean) {
    if (script.reduced || stage !== "shake") {
        return { x: 0, y: 0 };
    }
    var step = STEP_MS - 25 * Math.max(0, Math.min(1, agitation));
    var n = Math.floor(t / step) % STEPS;
    return {
        x: script.hopX[n] + Math.round(Math.max(-1, Math.min(1, lean)) * 2) * 2,
        y: script.hopY[n]
    };
}

// The lid: how far it has lifted (0 closed, 1 gone), in quarters unless motion is reduced.
function lidOpen(script, stage, t) {
    if (stage === "shake") {
        return 0;
    }
    if (stage === "lid") {
        var u = clamp01(t / stageLength(script, "lid"));
        return script.reduced ? u : Math.floor(u * LID_STEPS) / LID_STEPS;
    }
    return 1;
}
