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
//   weighted  dice simulated by us at a fixed 480 steps a second, in the
//             box's own frame. Each die is a square in the plan (it turns, so
//             its corners are what meet the walls and its neighbours) with a
//             height: it slides with friction, leaves the floor, hits the lid,
//             stacks on a neighbour and slides off, and bumps and turns what
//             it meets with a proper impulse. It is accelerated by the
//             *negative of what the accelerometer reads* (so gravity, tilt, a
//             shove and a jolt are all one thing) and by the centrifugal,
//             Coriolis and Euler terms of the gyroscope's rotation. Its tip
//             (how far it is rocked off the floor) follows its impacts through
//             damped springs. When the shaking stops each die hops to its own
//             cell, in a seeded order, and lands there.
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
// (degrees), opacity, shadowRadius, shadowOpacity, and state.boxRx, boxRy,
// lidAngle, lidLift, lidOpacity, leanFraction. The events (state.events) are
// { kind: "rattled" }, { kind: "landed", index, pitch } and { kind: "settled" }.

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

// board is { rows, columns, tiles }; layout is { tile, gap, pitch, reduced,
// weighted, lidTiles, slackTiles, mock, mockBurstMs }, in the pixels the scene
// is drawn in (lidTiles is the height of the lid's middle above the floor, and
// slackTiles how far the box's walls are past the board, both in tiles).
function create(board, seed, layout) {
    var rowCount = board.rows;
    var columnCount = board.columns;
    var count = rowCount * columnCount;
    var rand = random(seed);
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
            landed: false
        });
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
var shakeStillLevel = 0.3;        // g of "hand" (gravity taken out) below which it is not being shaken
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
    tracker.energy += (ux * ux + uy * uy + uz * uz - tracker.energy) * 0.15;
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
// seed. A burst of strokes: back and forth along an axis that drifts, at three
// to five hertz that wanders, rising to about 1.2 g at the ends of a stroke
// (where the hand turns round), with the up and down of the arm, a little
// ripple from the box and the dice in it, now and then a small knock, noise on
// every sample, a slow tip of the device, and a twist on the gyroscope that goes
// with the strokes. burstMs of 0 is a burst of 0.6 to 0.9 s from the seed.
function mockSampleAt(seed, k, burstMs) {
    var t = k * sampleMs / 1000;
    var burst = (burstMs > 0 ? burstMs : 600 + 300 * hash(seed, 0, 90)) / 1000;
    var two = 2 * Math.PI;
    var env = 0;
    if (t < burst) {
        env = smooth(t / 0.12) * (1 - smooth((t - (burst - 0.15)) / 0.15));
    }
    var p = [];
    for (var i = 0; i < 9; ++i) {
        p.push(two * hash(seed, 0, 100 + i));
    }
    var f0 = 3.2 + 1.4 * hash(seed, 0, 91);
    // The phase is the integral of a frequency that wanders around f0.
    var phase = two * f0 * t - 0.6 / 0.9 * Math.cos(two * 0.9 * t + p[1]);
    var stroke = Math.sin(phase) + 0.12 * Math.sin(2 * phase + p[2]);
    var amount = 1.2 * env * (0.85 + 0.15 * Math.sin(two * 1.3 * t + p[3]));
    var axis = p[0] + 0.9 * Math.sin(two * 0.7 * t + p[4]);
    var ax = amount * stroke * Math.cos(axis);
    var ay = amount * stroke * Math.sin(axis);
    var az = 2.6 * amount * Math.sin(phase + 1.2);
    var ripple = 0.08 * env * Math.sin(two * 23 * t + p[5]);
    ax += ripple; ay -= ripple;
    // A small knock, one or two samples.
    for (var back = 0; back < 2; ++back) {
        var kk = k - back;
        if (kk >= 0 && hash(seed, kk, 3) < 0.012 * env) {
            var knock = (0.25 + 0.25 * hash(seed, kk, 4)) * (hash(seed, kk, 5) < 0.5 ? -1 : 1) * (back === 0 ? 1 : 0.5);
            var which = Math.floor(hash(seed, kk, 6) * 3);
            if (which === 0) { ax += knock; } else if (which === 1) { ay += knock; } else { az += knock; }
        }
    }
    // The device tips a little as it is shaken.
    var roll = 0.05 * Math.sin(two * 1.1 * t + p[6]);
    var pitch = 0.04 * Math.sin(two * 0.9 * t + p[7]);
    var gx = -Math.sin(roll);
    var gy = Math.sin(pitch);
    var gz = Math.sqrt(Math.max(0, 1 - gx * gx - gy * gy));
    var noise = 0.02;
    var f = [gx + ax + noise * (hash(seed, k, 7) * 2 - 1),
             gy + ay + noise * (hash(seed, k, 8) * 2 - 1),
             gz + az + noise * (hash(seed, k, 9) * 2 - 1)];
    var twist = 1.6 * env * Math.sin(phase + p[8]);
    var w = [twist * Math.sin(axis) + 0.02 * (hash(seed, k, 10) * 2 - 1),
             -twist * Math.cos(axis) + 0.02 * (hash(seed, k, 11) * 2 - 1),
             0.3 * twist + 0.02 * (hash(seed, k, 12) * 2 - 1)];
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

// ---- Weighted motion: dice in a box --------------------------------------------
// Lengths are in tiles (one die's edge, taken as 20 mm), times in seconds. A
// tile is 20 mm, so 1 g is 490 tiles/s^2 and time is real: a die falls a tile
// in 64 ms. A die is a unit square in the plan, of mass 1 and moment of inertia
// 1/6, with a height, and a tip that is only for the eye.
var simStepMs = 1000 / 480;
var gTiles = 9810 / 20;           // tiles/s^2 in 1 g
var restitutionFloor = 0.38;
var restitutionDie = 0.35;
var restitutionWall = 0.3;
var restitutionLid = 0.25;
var frictionFloor = 0.5;
var frictionDie = 0.3;
var frictionWall = 0.2;
var restThreshold = 3;            // tiles/s: below this a contact does not bounce
var inverseInertia = 6;
var linearDamping = 0.15;         // per second
var yawDampingFloor = 6;
var yawDampingAir = 0.6;
var maxYawRate = 25;              // rad/s: about 0.4 of a turn a frame at 60 Hz
var maxSlide = 30;                // tiles/s: half a tile a frame at 60 Hz, what the eye can follow
var tipStiffness = 500;           // the tip follows impacts through a spring,
var tipDamping = 45;              // critically damped: it rocks and settles, it does not buzz
var maxTip = 0.6;                 // rad
var pairPasses = 3;               // rounds of pushing crowded dice apart
var hopRiseNear = 0.3;           // how far above where it is a die hops to reseat, tiles
var hopRiseFar = 1.0;             // and to cross the box
var hopDelayMs = 90;              // the lid is up before the first die hops
var landingBounceMs = 100;

function initWeighted(state, board, seed, layout) {
    state.mode = 'weighted';
    state.simAcc = 0;
    state.simSteps = 0;
    state.halfX = ((state.columns - 1) * layout.pitch / layout.tile + 1) / 2 + layout.slackTiles;
    state.halfY = ((state.rows - 1) * layout.pitch / layout.tile + 1) / 2 + layout.slackTiles;
    state.lidZ = layout.lidTiles - 0.03;              // the lid's underside
    state.lastStartMs = 0;
    state.f = [0, 0, 1];
    state.w = [0, 0, 0];
    state.alpha = [0, 0, 0];
    state.tipX = 0;
    state.tipY = 0;
    state.vibe = 0;
    state.hits = 0;
    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        state.lastStartMs = Math.max(state.lastStartMs, d.startMs);
        d.hx = d.cx / layout.tile;                    // its cell
        d.hy = d.cy / layout.tile;
        d.rng = random((seed | 0) * 31 + i * 7919 + 17);
        // Dice are not alike: a little bouncier or grippier than its neighbour,
        // which, with the knocks they give one another, is what keeps a box of
        // dice from moving as one. At rest each is exactly on its cell.
        d.x = d.hx;
        d.y = d.hy;
        d.z = 0.5;
        d.psi = 0;
        d.vx = 0; d.vy = 0; d.vz = 0; d.wpsi = 0;
        d.phx = 0; d.phy = 0; d.wphx = 0; d.wphy = 0;
        d.bounceVariety = 0.8 + 0.4 * d.rng();
        d.gripVariety = 0.8 + 0.4 * d.rng();
        d.grounded = true;
        d.hop = null;           // while hopping home: where from, and how long
        d.launchAt = -1;
        d.done = false;
        d.landed = false;
    }
}

// The kick a tip gets from an impact: a die rocks the way it was hit.
function rock(d, speed, nx, ny) {
    var amount = Math.min(1, speed / 12);
    d.wphx += (ny * 6 + (d.rng() * 2 - 1) * 2) * amount;
    d.wphy += (-nx * 6 + (d.rng() * 2 - 1) * 2) * amount;
}

// A contact in the plan between die a and die b (or a fixed wall, b = null) at
// the point (px, py), the normal (nx, ny) pointing from a to b (a to the wall):
// gives the impulse, friction and yaw that go with it, and returns the impulse.
function contactImpulse(a, b, px, py, nx, ny, restitution, friction) {
    var rax = px - a.x, ray = py - a.y;
    var vax = a.vx - a.wpsi * ray, vay = a.vy + a.wpsi * rax;
    var vbx = 0, vby = 0, rbx = 0, rby = 0;
    if (b) {
        rbx = px - b.x; rby = py - b.y;
        vbx = b.vx - b.wpsi * rby; vby = b.vy + b.wpsi * rbx;
    }
    var rvx = vbx - vax, rvy = vby - vay;
    var vn = rvx * nx + rvy * ny;
    if (vn >= 0) {
        return 0;
    }
    var ran = rax * ny - ray * nx;
    var denom = 1 + inverseInertia * ran * ran;
    var rbn = 0;
    if (b) {
        rbn = rbx * ny - rby * nx;
        denom += 1 + inverseInertia * rbn * rbn;
    }
    var e = vn < -restThreshold ? restitution : 0;
    var j = -(1 + e) * vn / denom;
    a.vx -= j * nx; a.vy -= j * ny;
    a.wpsi -= inverseInertia * ran * j;
    if (b) {
        b.vx += j * nx; b.vy += j * ny;
        b.wpsi += inverseInertia * rbn * j;
    }
    // friction along the contact
    var tx = -ny, ty = nx;
    rvx = (b ? b.vx - b.wpsi * rby : 0) - (a.vx - a.wpsi * ray);
    rvy = (b ? b.vy + b.wpsi * rbx : 0) - (a.vy + a.wpsi * rax);
    var vt = rvx * tx + rvy * ty;
    var rat = rax * ty - ray * tx;
    var denomT = 1 + inverseInertia * rat * rat;
    if (b) {
        var rbt = rbx * ty - rby * tx;
        denomT += 1 + inverseInertia * rbt * rbt;
    }
    var jt = -vt / denomT;
    var limit = friction * j;
    jt = Math.max(-limit, Math.min(limit, jt));
    a.vx -= jt * tx; a.vy -= jt * ty;
    a.wpsi -= inverseInertia * (rax * ty - ray * tx) * jt;
    if (b) {
        b.vx += jt * tx; b.vy += jt * ty;
        b.wpsi += inverseInertia * (rbx * ty - rby * tx) * jt;
    }
    return j;
}

// One fixed step of the box's frame. state.f is the accelerometer's reading in
// g, state.w and state.alpha the gyroscope's rate and its change.
function stepWeighted(state) {
    var h = simStepMs / 1000;
    var inStage = state.clock - state.stageStart;
    // Once the lid is lifting the device is put down: the reading fades to flat.
    var follow = state.stage === 'shake' ? 1
        : (state.stage === 'lid' ? 1 - smooth(inStage / lidMs) : 0);
    var fx = state.f[0] * follow;
    var fy = state.f[1] * follow;
    var fz = 1 + (state.f[2] - 1) * follow;
    var wx = state.w[0] * follow, wy = state.w[1] * follow, wz = state.w[2] * follow;
    var alx = state.alpha[0] * follow, aly = state.alpha[1] * follow, alz = state.alpha[2] * follow;
    var n = state.count;
    var dampLinear = 1 - linearDamping * h;
    var ceiling = state.stage === 'shake' ? state.lidZ - 0.5 : 1000;

    for (var i = 0; i < n; ++i) {
        var d = state.dice[i];
        if (d.hop !== null || d.done) {
            continue;
        }
        // What the dice feel in the box's frame: minus what the accelerometer
        // reads, and the pseudo-forces of the box's spin (centrifugal,
        // Coriolis, Euler), the position taken from the middle of the box's floor.
        var rx = d.x, ry = d.y, rz = d.z;
        var ax = -fx * gTiles - (wy * (wx * ry - wy * rx) - wz * (wz * rx - wx * rz))
            - 2 * (wy * d.vz - wz * d.vy) - (aly * rz - alz * ry);
        var ay = -fy * gTiles - (wz * (wy * rz - wz * ry) - wx * (wx * ry - wy * rx))
            - 2 * (wz * d.vx - wx * d.vz) - (alz * rx - alx * rz);
        var az = -fz * gTiles - (wx * (wz * rx - wx * rz) - wy * (wy * rz - wz * ry))
            - 2 * (wx * d.vy - wy * d.vx) - (alx * ry - aly * rx);
        d.vx = (d.vx + ax * h) * dampLinear;
        d.vy = (d.vy + ay * h) * dampLinear;
        d.vz = (d.vz + az * h) * dampLinear;
        d.x += d.vx * h;
        d.y += d.vy * h;
        d.z += d.vz * h;
        d.psi += d.wpsi * h;
        // the tip leans against an acceleration, and rocks from impacts
        var leanX = Math.max(-0.25, Math.min(0.25, -ay / gTiles * 0.25));
        var leanY = Math.max(-0.25, Math.min(0.25, ax / gTiles * 0.25));
        d.wphx += (-tipStiffness * (d.phx - leanX) - tipDamping * d.wphx) * h;
        d.wphy += (-tipStiffness * (d.phy - leanY) - tipDamping * d.wphy) * h;
        d.phx += d.wphx * h;
        d.phy += d.wphy * h;
        d.phx = Math.max(-maxTip, Math.min(maxTip, d.phx));
        d.phy = Math.max(-maxTip, Math.min(maxTip, d.phy));

        // The floor, with friction against the weight on it.
        var onFloor = false;
        if (d.z <= 0.5) {
            d.z = 0.5;
            onFloor = true;
            if (d.vz < 0) {
                var impact = -d.vz;
                if (impact > restThreshold) {
                    d.vz = impact * restitutionFloor * d.bounceVariety;
                    // an impact grips: some of the sliding speed goes
                    var slideNow = Math.sqrt(d.vx * d.vx + d.vy * d.vy);
                    if (slideNow > 1e-6) {
                        var lose = Math.min(slideNow, frictionFloor * d.gripVariety * (1 + restitutionFloor) * impact);
                        d.vx *= 1 - lose / slideNow;
                        d.vy *= 1 - lose / slideNow;
                    }
                    rock(d, impact, d.vx / Math.max(1, Math.sqrt(d.vx * d.vx + d.vy * d.vy)), d.vy / Math.max(1, Math.sqrt(d.vx * d.vx + d.vy * d.vy)));
                    d.wpsi += (d.rng() * 2 - 1) * Math.min(6, impact * 0.25);
                    state.hits += 1;
                } else {
                    d.vz = 0;
                }
            }
            var slide = Math.sqrt(d.vx * d.vx + d.vy * d.vy);
            if (slide > 0) {
                var drop = frictionFloor * d.gripVariety * Math.max(0, fz) * gTiles * h;
                var keep = slide > drop ? (slide - drop) / slide : 0;
                d.vx *= keep;
                d.vy *= keep;
            }
        }
        // The lid, while it is on.
        if (d.z > ceiling) {
            d.z = ceiling;
            if (d.vz > 0) {
                rock(d, d.vz, 0, 0);
                d.vz = -d.vz * restitutionLid;
            }
        }
        d.wpsi *= 1 - (onFloor ? yawDampingFloor : yawDampingAir) * h;
        d.wpsi = Math.max(-maxYawRate, Math.min(maxYawRate, d.wpsi));
        d.grounded = onFloor;
    }

    // The walls of the box, met by a die's corners.
    wallsOf(state);
    // Nothing moves faster than the eye can follow at 60 Hz, or turns faster.
    for (var y = 0; y < n; ++y) {
        var dy2 = state.dice[y];
        dy2.wpsi = Math.max(-maxYawRate, Math.min(maxYawRate, dy2.wpsi));
        var speed = Math.sqrt(dy2.vx * dy2.vx + dy2.vy * dy2.vy);
        if (speed > maxSlide) {
            dy2.vx *= maxSlide / speed;
            dy2.vy *= maxSlide / speed;
        }
    }

    // Dice push one another apart: in the plan by their corners, or, if one is
    // above the other, by the height.
    for (var pass = 0; pass < pairPasses; ++pass) {
        for (var a = 0; a < n; ++a) {
            var da = state.dice[a];
            if (da.hop !== null || da.done) {
                continue;
            }
            for (var b2 = a + 1; b2 < n; ++b2) {
                var db = state.dice[b2];
                if (db.hop !== null || db.done) {
                    continue;
                }
                pairContact(state, da, db, pass === 0);
            }
        }
    }
    state.simSteps += 1;
}

// A die and the walls: each of its four sides against the box, by the corners.
function wallsOf(state) {
    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        if (d.hop !== null || d.done) {
            continue;
        }
        var c = Math.cos(d.psi), s = Math.sin(d.psi);
        var ex = 0.5 * (Math.abs(c) + Math.abs(s));
        // The four walls, as (axis, side).
        for (var wall = 0; wall < 4; ++wall) {
            var horizontal = wall < 2;
            var side = (wall & 1) ? -1 : 1;
            var limit = horizontal ? state.halfX : state.halfY;
            var position = horizontal ? d.x : d.y;
            var reach = ex;                   // a rotated square's half extent is the same on both axes
            var over = side * position + reach - limit;
            if (over <= 0) {
                continue;
            }
            // the corner that is furthest into the wall
            var best = -1e9, cornerX = 0, cornerY = 0;
            for (var q = 0; q < 4; ++q) {
                var lx = (q & 1) ? 0.5 : -0.5, ly = (q & 2) ? 0.5 : -0.5;
                var gx = c * lx - s * ly, gy = s * lx + c * ly;
                var score = side * (horizontal ? gx : gy);
                if (score > best) { best = score; cornerX = gx; cornerY = gy; }
            }
            if (horizontal) {
                d.x -= side * over;
            } else {
                d.y -= side * over;
            }
            // the normal points from the die to the wall
            var nx = horizontal ? side : 0, ny = horizontal ? 0 : side;
            var speedIn = d.vx * nx + d.vy * ny;
            if (speedIn > 0) {
                var impulse = contactImpulse(d, null, d.x + cornerX, d.y + cornerY, nx, ny, restitutionWall, frictionWall);
                rock(d, speedIn, nx, ny);
                if (impulse > 0) {
                    state.hits += 1;
                }
            }
        }
    }
}

// Two dice: if their squares overlap in the plan and their heights overlap,
// push them out of one another along the way that takes least, and give the
// impulse. Dice more than a die apart in height do not touch.
function pairContact(state, a, b, first) {
    var dz = b.z - a.z;
    var apart = Math.abs(dz);
    if (apart >= 1) {
        return;
    }
    var dx = b.x - a.x, dy = b.y - a.y;
    if (dx * dx + dy * dy > 2.02) {
        return;
    }
    var ca = Math.cos(a.psi), sa = Math.sin(a.psi);
    var cb = Math.cos(b.psi), sb = Math.sin(b.psi);
    // the four axes, each a face normal of one of the squares
    var best = 1e9, bnx = 0, bny = 0, fromA = true;
    for (var k = 0; k < 4; ++k) {
        var ux, uy, owner;
        if (k === 0) { ux = ca; uy = sa; owner = true; }
        else if (k === 1) { ux = -sa; uy = ca; owner = true; }
        else if (k === 2) { ux = cb; uy = sb; owner = false; }
        else { ux = -sb; uy = cb; owner = false; }
        var ra = 0.5 * (Math.abs(ux * ca + uy * sa) + Math.abs(-ux * sa + uy * ca));
        var rb = 0.5 * (Math.abs(ux * cb + uy * sb) + Math.abs(-ux * sb + uy * cb));
        var distance = dx * ux + dy * uy;
        var depth = ra + rb - Math.abs(distance);
        if (depth <= 0) {
            return;
        }
        if (depth < best) {
            best = depth;
            bnx = distance >= 0 ? ux : -ux;
            bny = distance >= 0 ? uy : -uy;
            fromA = owner;
        }
    }
    var depthZ = 1 - apart;
    if (depthZ < best) {
        // one sits on the other: push them apart in height
        var upper = dz > 0 ? b : a;
        var lower = dz > 0 ? a : b;
        var half = depthZ * 0.5;
        upper.z += half;
        lower.z -= half;
        if (lower.z < 0.5) {
            upper.z += 0.5 - lower.z;
            lower.z = 0.5;
        }
        var relative = upper.vz - lower.vz;
        if (relative < 0) {
            var e = relative < -restThreshold ? restitutionDie : 0;
            var jz = -(1 + e) * relative / 2;
            upper.vz += jz;
            lower.vz -= jz;
            if (lower.z <= 0.5) {
                lower.vz = Math.max(0, lower.vz);
            }
        }
        // sliding on one another: the two drag one another along
        var tx = upper.vx - lower.vx, ty = upper.vy - lower.vy;
        var drag = Math.min(1, frictionDie * Math.max(0, -relative) * 0.05 + 0.02);
        upper.vx -= tx * drag; upper.vy -= ty * drag;
        lower.vx += tx * drag; lower.vy += ty * drag;
        return;
    }
    // in the plan: the point of contact is the deepest corner of the other
    // square, taken on the side that the normal leaves from
    var px = 0, py = 0, score = -1e9;
    var from = fromA ? b : a;                    // the square whose corner meets the face
    var cf = fromA ? cb : ca, sf = fromA ? sb : sa;
    var sign = fromA ? -1 : 1;                   // toward the other square along -n or +n
    for (var q = 0; q < 4; ++q) {
        var lx = (q & 1) ? 0.5 : -0.5, ly = (q & 2) ? 0.5 : -0.5;
        var gx = cf * lx - sf * ly, gy = sf * lx + cf * ly;
        var t = sign * (gx * bnx + gy * bny);
        if (t > score) { score = t; px = from.x + gx; py = from.y + gy; }
    }
    // push apart (a toward -n, b toward +n), then the impulse
    var move = best * 0.5 * (first ? 1 : 0.6);
    a.x -= bnx * move; a.y -= bny * move;
    b.x += bnx * move; b.y += bny * move;
    var impulse = contactImpulse(a, b, px, py, bnx, bny, restitutionDie, frictionDie);
    if (impulse > 0.5) {
        var along = Math.abs(a.vx * bnx + a.vy * bny) + Math.abs(b.vx * bnx + b.vy * bny);
        rock(a, along * 0.5, bnx, bny);
        rock(b, along * 0.5, -bnx, -bny);
        state.hits += 1;
    }
}

// A die lets go of the physics and hops home: a ballistic arc from where it is
// to its own cell, turning flat and upright on the way.
function startHop(state, d, index) {
    var distance = Math.sqrt((d.hx - d.x) * (d.hx - d.x) + (d.hy - d.y) * (d.hy - d.y));
    var rise = Math.min(hopRiseFar, hopRiseNear + 0.55 * distance) * (0.9 + 0.2 * d.rng());
    var z0 = Math.max(0.5, d.z);
    var up = Math.sqrt(2 * rise / gTiles);
    var down = Math.sqrt(2 * (z0 + rise - 0.5) / gTiles);
    // yaw by the shortest way round
    var psi = d.psi - 2 * Math.PI * Math.round(d.psi / (2 * Math.PI));
    d.hop = {
        t0: state.clock,
        T: (up + down) * 1000,
        x0: d.x, y0: d.y, z0: z0,
        vz: gTiles * up,
        psi0: psi, phx0: d.phx, phy0: d.phy,
        wobble: (d.rng() * 2 - 1),
        index: index
    };
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
            // Each die hops home in the seeded order, once the lid is up.
            for (var i = 0; i < state.count; ++i) {
                state.dice[i].launchAt = state.clock + hopDelayMs + state.dice[i].startMs;
            }
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
        for (var s = 0; s < state.count; ++s) {
            var ds = state.dice[s];
            if (ds.launchAt >= 0 && ds.hop === null && !ds.done && state.clock >= ds.launchAt) {
                startHop(state, ds, s);
            }
        }
        stepWeighted(state);
    }
    // Where the hopping dice are now: arcs, not physics.
    var remaining = 0;
    for (var j = 0; j < state.count; ++j) {
        var dj = state.dice[j];
        if (dj.done) {
            continue;
        }
        remaining += 1;
        if (dj.hop === null) {
            continue;
        }
        var hop = dj.hop;
        var tau = (state.clock - hop.t0) / 1000;
        var T = hop.T / 1000;
        if (tau < T) {
            var u = smooth(tau / T);
            dj.x = hop.x0 + (dj.hx - hop.x0) * (tau / T);
            dj.y = hop.y0 + (dj.hy - hop.y0) * (tau / T);
            dj.z = hop.z0 + hop.vz * tau - 0.5 * gTiles * tau * tau;
            dj.psi = hop.psi0 * (1 - u);
            dj.phx = hop.phx0 * (1 - u);
            dj.phy = hop.phy0 * (1 - u);
        } else {
            if (!dj.landed) {
                dj.landed = true;
                state.events.push({ kind: 'landed', index: j, pitch: dj.pitchValue });
            }
            var after = (state.clock - hop.t0 - hop.T) / landingBounceMs;
            if (after < 1) {
                // a small rebound and a settling wobble, then it is down
                dj.x = dj.hx; dj.y = dj.hy; dj.psi = 0;
                dj.z = 0.5 + 0.07 * Math.sin(Math.PI * after) * (1 - after);
                dj.phx = 0.06 * hop.wobble * Math.sin(2 * Math.PI * 1.5 * after) * (1 - after);
                dj.phy = 0;
            } else {
                dj.x = dj.hx; dj.y = dj.hy; dj.z = 0.5;
                dj.psi = 0; dj.phx = 0; dj.phy = 0;
                dj.vx = 0; dj.vy = 0; dj.vz = 0; dj.wpsi = 0;
                dj.done = true;
                remaining -= 1;
            }
        }
    }
    // What the box shows of the device: how it is held, which is gravity as the
    // reading settles to it, not each stroke (the dice answer to the strokes).
    var gv = tracker.g;
    var norm = Math.max(0.2, Math.sqrt(gv[0] * gv[0] + gv[1] * gv[1] + gv[2] * gv[2]));
    state.tipX = Math.asin(Math.max(-1, Math.min(1, gv[1] / norm))) * 180 / Math.PI;
    state.tipY = Math.asin(Math.max(-1, Math.min(1, -gv[0] / norm))) * 180 / Math.PI;
    state.vibe = tracker.level;
    if (state.stage === 'settle' || state.stage === 'lid') {
        if (remaining === 0) {
            enter(state, 'idle');
            state.events.push({ kind: 'settled' });
        } else if (state.stage === 'settle' && state.clock - state.stageStart > state.lastStartMs + 1200) {
            for (var k = 0; k < state.count; ++k) {
                var e = state.dice[k];
                e.x = e.hx; e.y = e.hy; e.z = 0.5;
                e.psi = 0; e.phx = 0; e.phy = 0;
                e.vx = 0; e.vy = 0; e.vz = 0; e.wpsi = 0;
                e.hop = null; e.done = true;
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
    var toDegrees = 180 / Math.PI;
    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        var height = Math.max(0, d.z - 0.5);
        var airborne = Math.min(2, height);
        state.px[i] = d.x * tile;
        state.py[i] = d.y * tile;
        state.pz[i] = d.z * tile;
        state.rx[i] = d.phx * toDegrees;
        state.ry[i] = d.phy * toDegrees;
        state.rz[i] = d.psi * toDegrees;
        state.opacity[i] = 1;
        state.shadowRadius[i] = tile * 0.47 * (1 - 0.1 * Math.min(1.5, airborne));
        state.shadowOpacity[i] = 0.34 / (1 + 1.2 * airborne);
    }
    // The box is shown tipped the way the device is held, up to about eight
    // degrees, and otherwise still: a box that shivers with every stroke reads
    // as judder, and the dice already answer to the strokes.
    var limit = 8;
    state.boxRx = Math.max(-limit, Math.min(limit, state.tipX));
    state.boxRy = Math.max(-limit, Math.min(limit, state.tipY));
    // The lid.
    var lidU = state.stage === 'shake' ? 0 : (state.stage === 'lid' ? inStage / lidMs : 1);
    var lift = smooth(lidU);
    state.lidAngle = -72 * lift;
    state.lidLift = tile * 0.9 * lift;
    state.lidOpacity = state.stage === 'shake' ? 1
        : (state.stage === 'lid' ? 1 - smooth((lidU - 0.55) / 0.45) : 0);
    // The camera comes upright as the dice hop home.
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

        if (state.reduced) {
            // Nothing moves: the dice are on their cells and fade in once the
            // lid has gone.
            alpha = state.stage === "drop" ? smooth(inStage / dropFadeMs)
                : (state.stage === "settle" || resting ? 1 : 0);
        } else if (dropMs < 0 || (dropMs !== Infinity && dropMs < die.startMs)) {
            // In the box.
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
            var values = [state.px[i], state.py[i], state.pz[i], state.rx[i], state.ry[i], state.rz[i]];
            for (var v = 0; v < values.length; ++v) {
                sum = Math.imul(sum ^ (Math.round(values[v] * 100) | 0), 0x01000193);
            }
        }
    }
    return (sum >>> 0).toString(16);
}
