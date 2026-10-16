.pragma library

// The shake, the lid and the drop as a script: what every die, the box, the lid
// and the camera are doing at each moment of a play. It is plain JavaScript with
// no QML in it and no renderer, so the Qt Quick 3D scene (BoardScene.qml) is one
// way to draw it and a 2D scene would be another, and the two replay the same
// board and seed the same way.
//
// There is no physics engine, and the board is decided before the play starts.
// Everything is computed from the board, the seed and the time, in one of two
// ways (layout.weighted):
//
//   staged    every position and angle is a prescribed curve of the time: a
//             sine rattle, a fall on a parabola, one rebound.
//   weighted  a few lines of integration of our own, at a fixed 240 steps a
//             second, seeded and so replayable: dice that are thrown about a
//             box, bounce off its floor, lid and walls and off one another,
//             spin when they hit something, and then slot into their cells
//             under springs, with bounces that die away. Where a die ends is
//             still its cell, always; the integration only decides how it gets
//             there.
//
// The one live input is the device's motion: `agitation` and the tilt change
// how hard the box rattles and for how long, and never where a die lands.
//
//   state = create(board, seed, layout)   the script for a play, at time zero
//   advance(state, dtMs, agitation, tiltX, tiltY)   moves it on; collects events
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
// weighted, lidTiles }, in the pixels the scene is drawn in (lidTiles is the
// height of the lid's middle above the floor, in tiles).
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
    state.mode = 'staged';
    if (layout.weighted && !layout.reduced) {
        initWeighted(state, board, seed, layout);
    }
    evaluate(state);
    return state;
}

// ---- Weighted motion ---------------------------------------------------------
// Lengths are in tiles (one die's edge), times in seconds, angles in degrees.
// The time is slowed to about a third of a real die's, so that a fall of a tile
// is a fifth of a second and can be seen at 60 Hz; it is the shape of the
// motion (the acceleration, the stop, the rebound, the spin) that carries the
// weight.
var simStepMs = 1000 / 240;
var gravity = 40;                 // tiles/s^2
var driveLateral = 60;            // peak push of the shaken box, tiles/s^2
var driveVertical = 95;           // must beat gravity to throw a die
var cavitySlack = 0.2;            // how far a die may stray past its cell
var dieRadius = 0.52;
var slotStiffness = 700;          // the funnel into a cell, 1/s^2
var slotDamping = 45;
var restitutionWall = 0.4;
var restitutionDie = 0.45;
var restitutionLid = 0.3;

function wrapDegrees(a) {
    return ((a + 180) % 360 + 360) % 360 - 180;
}

function initWeighted(state, board, seed, layout) {
    state.mode = 'weighted';
    state.simAcc = 0;
    state.simSteps = 0;
    state.limitX = (state.columns - 1) / 2 * layout.pitch / layout.tile + cavitySlack;
    state.limitY = (state.rows - 1) / 2 * layout.pitch / layout.tile + cavitySlack;
    state.ceiling = layout.lidTiles - 0.03 - 0.5;   // the lid's underside, to a die's centre
    state.lastStartMs = 0;
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

// A die has hit something at `speed`: it spins a little, more the harder.
function kick(d, speed) {
    var amount = Math.min(1, speed / 3);
    d.wpsi += (d.rng() * 2 - 1) * 300 * amount;
    d.wphx += (d.rng() * 2 - 1) * 160 * amount;
    d.wphy += (d.rng() * 2 - 1) * 160 * amount;
}

function weightedEnvelope(state) {
    var inStage = state.clock - state.stageStart;
    var energy = 0.75 + 0.25 * state.smoothAgitation;
    if (state.stage === 'shake') {
        return Math.min(1, inStage / 120) * energy;
    }
    if (state.stage === 'lid') {
        return (1 - smooth(inStage / lidMs)) * energy;
    }
    return 0;
}

function stepWeighted(state) {
    var h = simStepMs / 1000;
    var ts = state.simSteps * h;
    var env = weightedEnvelope(state);
    var rest = 0.5;
    // The shaken box: how it is accelerated, and so how the dice in it are
    // pushed the other way.
    var two = 2 * Math.PI;
    var bx = env * driveLateral * (0.6 * Math.sin(two * 4.3 * ts) + 0.4 * Math.sin(two * 7.1 * ts + 1.1)) + state.tiltX * 6;
    var by = env * driveLateral * (0.6 * Math.sin(two * 3.7 * ts + 0.7) + 0.4 * Math.sin(two * 6.3 * ts + 2.0)) - state.tiltY * 6;
    var bz = env * driveVertical * Math.sin(two * 5.0 * ts + 0.3);
    var ceiling = state.stage === 'shake' ? state.ceiling : 1000;
    var settling = state.stage === 'settle';
    var inStage = state.clock - state.stageStart;

    for (var i = 0; i < state.count; ++i) {
        var d = state.dice[i];
        if (settling && !d.active && inStage >= d.startMs) {
            d.active = true;
            d.psi = wrapDegrees(d.psi);
        }
        var cellX = d.cx / state.tile;
        var cellY = d.cy / state.tile;
        var ax = -bx + 25 * (cellX - d.x) - 0.6 * d.vx;
        var ay = -by + 25 * (cellY - d.y) - 0.6 * d.vy;
        var az = -gravity - bz;
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

        // The floor.
        var onFloor = false;
        if (d.z <= rest) {
            d.z = rest;
            onFloor = true;
            if (d.vz < 0) {
                var impact = -d.vz;
                if (impact < 0.35) {
                    d.vz = 0;
                } else {
                    d.vz = impact * d.bounce;
                    d.vx *= 0.8;
                    d.vy *= 0.8;
                    kick(d, impact);
                    if (d.active && !d.landed) {
                        d.landed = true;
                        state.events.push({ kind: 'landed', index: i, pitch: d.pitchValue });
                    }
                }
            }
            d.vx *= 1 - 5 * h;
            d.vy *= 1 - 5 * h;
        }
        // The lid, while it is on.
        if (d.z > ceiling) {
            d.z = ceiling;
            if (d.vz > 0) {
                kick(d, d.vz);
                d.vz = -d.vz * restitutionLid;
            }
        }
        // The walls of the box.
        if (d.x > state.limitX) { d.x = state.limitX; if (d.vx > 0) { kick(d, d.vx * 0.5); d.vx = -d.vx * restitutionWall; } }
        if (d.x < -state.limitX) { d.x = -state.limitX; if (d.vx < 0) { kick(d, -d.vx * 0.5); d.vx = -d.vx * restitutionWall; } }
        if (d.y > state.limitY) { d.y = state.limitY; if (d.vy > 0) { kick(d, d.vy * 0.5); d.vy = -d.vy * restitutionWall; } }
        if (d.y < -state.limitY) { d.y = -state.limitY; if (d.vy < 0) { kick(d, -d.vy * 0.5); d.vy = -d.vy * restitutionWall; } }

        // Turning: yaw is free but for friction, and funnelled once slotting;
        // the tips flop flat on the floor and spin on in the air.
        d.psi += d.wpsi * h;
        d.wpsi *= 1 - (onFloor ? 7 : 1.5) * h;
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

function advanceWeighted(state, dtMs, agitation, tiltX, tiltY) {
    state.clock += dtMs;
    state.smoothAgitation += (agitation - state.smoothAgitation) * Math.min(1, dtMs / 90);
    state.tiltX = tiltX;
    state.tiltY = tiltY;
    var inStage = state.clock - state.stageStart;
    if (state.stage === 'shake') {
        state.tickAccumulator += dtMs;
        if (state.tickAccumulator >= rattleTickMs) {
            state.tickAccumulator -= rattleTickMs;
            state.events.push({ kind: 'rattled' });
        }
        if (inStage >= shakeMinMs && (agitation < 0.15 || inStage >= shakeMaxMs)) {
            enter(state, 'lid');
        }
    } else if (state.stage === 'lid') {
        if (inStage >= lidMs) {
            enter(state, 'settle');
        }
    }
    state.simAcc += dtMs;
    while (state.simAcc >= simStepMs) {
        state.simAcc -= simStepMs;
        stepWeighted(state);
    }
    if (state.stage === 'settle') {
        var settleFor = state.clock - state.stageStart;
        var still = true;
        for (var i = 0; i < state.count && still; ++i) {
            var d = state.dice[i];
            var cellX = d.cx / state.tile;
            var cellY = d.cy / state.tile;
            still = d.active
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
    var seconds = state.clock / 1000;
    var inStage = state.clock - state.stageStart;
    var env = weightedEnvelope(state);
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
        state.opacity[i] = 1;
        state.shadowRadius[i] = tile * 0.47 * (1 - 0.1 * Math.min(1.5, airborne));
        state.shadowOpacity[i] = 0.34 / (1 + 1.2 * airborne);
    }
    // The box: a tip of about eight degrees that dies away as the lid goes,
    // plus the lean the device is tilted to.
    var rattleX = env * boxDegrees * 0.8 * Math.sin(6.2832 * 5.3 * seconds);
    var rattleY = env * boxDegrees * 0.8 * Math.sin(6.2832 * 4.1 * seconds + 1.3);
    state.boxRx = Math.max(-boxDegrees, Math.min(boxDegrees, rattleX - state.tiltY * 5));
    state.boxRy = Math.max(-boxDegrees, Math.min(boxDegrees, rattleY + state.tiltX * 5));
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
function advance(state, dtMs, agitation, tiltX, tiltY) {
    if (state.mode === 'weighted') {
        advanceWeighted(state, dtMs, agitation, tiltX, tiltY);
        return;
    }
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
    var frameMs = 1000 / 60;
    for (var t = 0; t < ms; t += frameMs) {
        advance(state, Math.min(frameMs, ms - t), 0, 0, 0);
    }
    evaluate(state);
    return state;
}

// A checksum of every pose a board and seed produce, frame by frame at 60 Hz
// with the device still. The same board, seed and layout give the same string.
function digest(board, seed, layout) {
    var state = create(board, seed, layout);
    var sum = 0x811C9DC5;
    for (var frame = 0; state.stage !== "idle" && frame < 1000; ++frame) {
        advance(state, 1000 / 60, 0, 0, 0);
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
