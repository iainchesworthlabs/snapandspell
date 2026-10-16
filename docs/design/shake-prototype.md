# Shake prototype

The board scene of [Board](board.md#the-shake-and-the-drop), built in Qt Quick 3D
behind its one-component seam, and what it cost on one desktop. A prototype:
it answers "can it be done, what does it cost, and what must the seam say", and
it is not the game's board.

**Nothing here was measured on an iPad, or on any tablet or phone.** The shared
Qt tree has no iOS kit, so the prototype could not be built for one. Every
figure is from one Windows desktop (below), and the multiple to a tablet is
still the estimate [Performance](performance.md#budgets) already makes.

**Licence.** A build that links Qt Quick 3D is **GPL-3.0 only**: Qt offers
`Quick3D` as `LicenseRef-Qt-Commercial OR GPL-3.0-only` (SBOM, checked in
[Board](board.md#rendered-3d-or-drawn-in-2d)). `ShakePrototype` links it, so it
is GPL-3.0 only, and it is a program of its own so that `WordJewelsSolver`
does not. The decision stands as the author made it on 2026-10-16: the MVP is
a private build, and D2 is taken before any store, with the seam in hand.

**Checked, not changed:** with the option off nothing links Qt Quick 3D and no
`Qt6Quick3D.dll` is deployed. But the application's deploy directory already
holds `Qt6Quick3DUtils.dll` (also GPL-3.0 only in the SBOM), because
`windeployqt` copies the QML debugger's `qmltooling\qmldbg_quick3dprofiler.dll`,
which depends on it. The application does not link it and the plugin loads only
when QML debugging is switched on, but it is in the folder that would ship.
`windeployqt --skip-plugin-types qmltooling` would keep it out; that is the
platform fragment's to change, not this page's, and is left for the author.

## What was built

| Part | Where | What it is |
| --- | --- | --- |
| The scene | `src/app/prototype/shake/qml/BoardScene.qml` | The component: a `View3D`, the box, the lid, one `Model` for each die sharing the `#Cube` mesh, a letter atlas, a baked light probe. Draws the poses the script gives it |
| The script | `qml/ShakeScript.js` | The shake, lid and landing as a function of the board, the seed and the time, in two ways: prescribed curves (staged) or a small integrator of our own (weighted). Plain JavaScript, no QML and no renderer |
| The faces | `qml/FaceAtlas.qml`, `qml/Die.qml` | One texture for the board's tiles, each die sampling its own cell; a dead jewel is a cell of four facets |
| The harness | `qml/ShakeWindow.qml`, `frame_stats.*`, `main.cpp` | A window with a Shake button, a mouse drag standing in for the accelerometer, a reduced-motion switch, a 2D tile layer over the scene, and the timing and measurement runs |
| The probe | `tools/lightprobe`, `qml/lightprobe/sky.ktx` | Qt's procedural sky prefiltered in advance (128 x 128 texels a face, 1 MB), made by Cubeulator's tool over the same sky |

It is behind `-DWJS_BUILD_SHAKE_PROTOTYPE=ON`, off by default, and the default
build is unchanged: the option adds one program and finds one more Qt module.
The scene has no game rules in it: a tile is an opaque string and `""` is a dead
jewel.

### The interface

| Member | Kind | Meaning |
| --- | --- | --- |
| `play(board, seed)` | function | Shake, lid, drop, settle, for `board = { rows, columns, tiles }`. The dice are in the box at the first frame |
| `show(board)` | function | The board already down, with no motion: a resumed game, a rotated window |
| `reducedMotion` | in | The cross-fading variant of every stage |
| `pushMotion(ax, ay, az, wx, wy, wz)` | function | One sample of the device's motion, about a hundred a second: the accelerometer's reading in g, gravity included (face up and still it reads 0, 0, 1; x right, y up the screen, z out of it), and the gyroscope's in rad/s. The dice answer to it sample for sample, and the rattle holds on, up to 3 s, while it reads as shaking. With none, a seeded stand-in shakes |
| `settled()` | signal | Once per play, when the dice have stopped |
| `landed(index, pitch)` | signal | A die has hit the floor: its place in the board, and a pitch for its click |
| `rattled()` | signal | The box rattles, at most 8 a second, for the haptic tap |
| `busy`, `rows`, `columns`, `tileSize`, `tileGap`, `boardRect` | read-only | The geometry by [the rule](board.md#the-rule), for the 2D layer that traces over the scene |
| `overdraw` | read-only | How far past its item the scene draws, on every side (half a tile), so a die tumbling at the edge and the tipping rim are not cut off. The item is still the board's area; the application must not `clip` it |

`lean`, `probeMode`, `cameraDistance`, `warmUp`, `weighted` and `mockBurstMs`
are settings of the 3D implementation and are not part of the seam. `seek(board, seed, ms)` and
`digest(board, seed)` are for tests and pictures.

### The two scripts

**Weighted** (the default) and **staged** are two ways of making the same
play, chosen by `weighted`; reduced motion is the same cross-fade in both. The
harness has a switch for it so they can be compared in one window.

**Staged** is [the stages of the board page](board.md#the-stages) as prescribed
curves: dice hover a tile above their cells and jitter on sine waves, then fall
in a seeded order on a parabola with one rebound, and the camera comes upright
over the fall.

**Weighted** is the same play with the stages kept (rattle for 0.7 s or while
the device shakes, up to 3 s; the lid; the dice go home) and the motion between
them simulated, at a fixed 480 steps a second, in the box's own frame, from what
the device does:

- **The input is the device's own signal.** A die's acceleration in the box is
  *minus what the accelerometer reads*: gravity, a tilt, a shove and a jolt are
  one thing, and a spike in the reading is a spike in the dice. The gyroscope
  adds the centrifugal, Coriolis and Euler terms of the box's spin. That is six
  axes, sampled about a hundred times a second, and the dice take each sample
  as it comes, so a jerky hand is a jerky shake.
- **A box with room in it.** The cavity is 0.6 of a tile wider than the board
  on every side, and a lid 2.4 tiles up. A die is a square in the plan with a
  height: it slides with friction against its weight, leaves the floor when the
  box is thrown away from it, hits the lid and the walls, and stacks on a
  neighbour and slides off. Dice can trade places, as real ones do.
- **Contact is an impulse.** Where two dice, or a die and a wall, meet, the
  corners that meet are found and each side gets an impulse along the normal and
  friction along the face, at that point, so a corner that hits first turns the
  die. The tip (how far it is rocked off the floor) follows its impacts through
  a critically damped spring, so it rocks and settles and does not buzz.
- **Real time, within what the eye can follow.** A tile is taken as 20 mm, so
  1 g is 490 tiles a second squared and a die falls a tile in 64 ms. Nothing
  slides faster than 30 tiles a second (half a tile a frame at 60 Hz) or turns
  faster than 25 rad/s. (The first version slowed time to a third, which makes a
  shake at a few hertz too quick for the dice to follow: it reads light.)
- **Home.** When the reading goes still the lid lifts and each die, in the
  seeded order and from wherever it has got to, hops to its own cell on an arc
  over the others, turning flat and upright as it goes, and lands there with a
  small rebound. The board is decided before the play, so a die that has traded
  places in the box goes back to its own cell; there is no relabelling.

With no device there is a stand-in, `mockSampleAt(seed, k)`, a function of the
seed and the sample number. It is meant to be a hand, not a signal generator:
a burst of 0.6 to 0.9 s of strokes at three to five hertz that wanders, back
and forth along an axis that drifts, reaching 1.2 g at the turns, with the up
and down of the arm (to 3 g) so that dice leave the floor, a little ripple from
the box, small knocks now and then, noise on every sample, a slow tip of the
device, and a twist on the gyroscope that goes with the strokes. The Shake
button, a replay and the tests use it, so they are the same shake every time;
a real device, or the drag in the harness, is live.

It is still seeded and replays: the same board, seed and stream give the same
checksum, because the integration is a fixed step with a generator for each
die. It is not a physics engine and does not decide where a die lands; that is
its cell, always.

| Play (weighted) | Measured, play to `settled` | What it is |
| --- | --- | --- |
| 4x4, full motion | 1.5 to 1.7 s | A 0.6 to 0.9 s burst, about 0.2 s for the reading to go still, the lid and the hops (the lid lifts while the first dice are going) |
| 3x3, full motion | 1.3 to 1.5 s | The same |
| Either, reduced motion | 0.55 to 0.57 s | 0.30 rattle, 0.12 lid fade, 0.12 dice fade |
| Rattle held at full | 3.6 to 3.8 s | A 4 s burst, cut at the 3 s limit, then the lid and the hops |

(Staged: 1.97 s at 4x4 and 1.73 s at 3x3, with a 0.87 s drop.)

How it moves, from the script run by itself, four seeds on the 4x4, as the
screen at 60 Hz sees it:

| | |
| --- | --- |
| Shake, then the lid, then home | 0.85 to 1.0 s, then 0.25 s, then 0.5 s |
| Two dice overlapping in the plan by more than 0.02 of a tile | 0 to 4% of solver steps; never by 0.1 |
| Distance a die moves in a frame | median 0.01 to 0.03 of a tile, 95th percentile 0.32 to 0.42, none over 0.6 |
| Turn in a frame | none over 0.5 rad |
| Dice off the floor by more than 0.15 of a tile | 24 to 30% of die-frames |
| Share of a die's movement that is faster than about 15 Hz (the buzz) | 0.5 to 1.1% |

## How it was measured

| | |
| --- | --- |
| Machine | Windows 11 Pro for Workstations 10.0.26200; AMD Ryzen 9 5950X (16 cores, 32 threads); NVIDIA GeForce RTX 4090 (driver 32.0.16.1742). Shared with other work, not quiesced |
| Renderer | Qt RHI on Direct3D 11, threaded render loop, flip-model swap chain, MSAA Medium. Qt reports the window's display at 60 Hz (a vsync of 16.68 ms) |
| Software | Qt 6.12.0 (`msvc2022_64` of the shared tree), MSVC 19.51, Release (`/O2`), QML compiled to C++ ahead of time |
| Window | 560 x 900 logical pixels, the scene in the top two thirds. The tables are from the **staged** script, which was the only one when they were taken; the weighted one is under [the weighted script](#the-weighted-script-in-the-same-harness) |
| Also tried | Qt's bundled Mesa llvmpipe through OpenGL (`QSG_RHI_BACKEND=opengl`, `QT_OPENGL=software`), as a stand-in for a device with a weak GPU; **not** a model of an iPad |

Frame times are the interval between `frameSwapped` signals. The render thread's
CPU time for a frame is `beforeFrameBegin` to `afterFrameEnd`; **it is the time
spent recording and submitting the frame, not the GPU's.** GPU time was not
measured. With a 60 Hz display every interval is near 16.7 ms, so "over 16.7 ms"
counts about 40% of all frames by a fraction of a millisecond. A missed refresh
is an interval of 25 ms or more, so the tables give over 20, 25 and 33 ms.

```powershell
cmake --preset windows-msvc-release -DWJS_BUILD_APP=ON -DWJS_BUILD_SHAKE_PROTOTYPE=ON
cmake --build --preset windows-msvc-release --target wjs_shake_prototype
$env:QT_ASSUME_STDERR_HAS_CONSOLE = 1      # the program is a GUI program; this shows its log
build\windows-msvc-release\bin\ShakePrototype.exe                       # to play with
build\windows-msvc-release\bin\ShakePrototype.exe --auto --runs 6 --out results   # timing
build\windows-msvc-release\bin\ShakePrototype.exe --auto --shots --out shots      # stills
build\windows-msvc-release\bin\ShakePrototype.exe --auto --shots --frames 30 --size 720x900 --out frames   # every frame, for a video
build\windows-msvc-release\bin\ShakePrototype.exe --auto --digest --out digest    # replay check
```

## What it cost

### Making the view

| | Cold pipeline cache | Warm cache |
| --- | --- | --- |
| Load `BoardScene.qml` (the Quick 3D module, the component) | 14 to 17 ms | 14 ms |
| Create it (`createObject`: the `View3D`, 16 dice, their materials) | 3 ms | 3 ms |
| First frame after that, to the swap | 454 ms (447 ms of it on the render thread) | 214 ms |
| The same with the first-shake warm-up off | 411 ms | 171 ms |

Cubeulator measured about 140 ms to create each of its cube views
([Board](board.md#rendered-3d-or-drawn-in-2d)); here the objects are made in
3 ms and **the cost is the first frame**, which is Qt Quick 3D generating and
compiling the shaders of the materials, once, at run time. The cache is the
pipeline cache Qt writes beside the application's data; a hot one is about half
the cold. The driver keeps its own, which was not cleared. **So the view is
made once, behind the home screen, and not on the press of Shake.**

### The first shake

The first play of a process is a different case, because a path that is only
drawn while things move (the translucent lid, the shadows, a die with an
opacity) has not been used yet.

| First shake of the process | Press to first frame | Render-thread time of that frame | Next five frames |
| --- | --- | --- | --- |
| No warm-up | 47.7 ms | 42.4 ms | 1.2, 4.2, 8.2, 10.9, 4.0 ms |
| With the warm-up | 17.9 ms | 1.1 ms | 0.9, 0.3, 1.0, 0.4, 0.3 ms |

The warm-up is three frames at 2% opacity of the lid, shadows and dice, drawn
once while the first board is shown. (The first try, at 0.4%, did nothing: Qt
does not draw below some small opacity, and the threshold was not looked up.)
It moves about 40 ms into the creation frame above. The 47.7 ms without it is
the only frame on Direct3D 11, after the scene was made, over the
[33 ms budget](performance.md#budgets).

### Two cadences

The same binary, run twice in a row on the same machine, presented at two
different rhythms: intervals of 16.7 ms and a press-to-first-frame of about
16 ms, and then intervals of 11 to 12 ms and a press-to-first-frame of 53 to
65 ms, with the same render-thread time. It happened with the overdraw off and
on, so it is not the scene. The suspect is which of the machine's displays (it
has a 100 Hz mode) the window opened on; it was not investigated. The tables
below are from runs in the 16.7 ms rhythm, and the 18 ms first-shake figure
with the warm-up is from there; the 48 ms without it is in the same one.

### During the shake and the drop

All the plays of three processes, each process's first play left out
(see above), 9,337 frames; Direct3D 11, the baked probe, the warm-up on.

| Play | Plays | Frames | Interval p50 / p99 / max | Over 20 / 25 / 33 ms | Render thread mean / max |
| --- | --- | --- | --- | --- | --- |
| 3x3 full | 15 | 1,545 | 16.7 / 17.2 / 17.8 ms | 0 / 0 / 0 | 0.35 / 0.92 ms |
| 4x4 full | 18 | 2,112 | 16.7 / 17.4 / 19.8 ms | 0 / 0 / 0 | 0.45 / 1.45 ms |
| 3x3 rattle held 3 s | 9 | 2,161 | 16.7 / 17.3 / 20.0 ms | 0 / 0 / 0 | 0.40 / 1.38 ms |
| 4x4 rattle held 3 s | 9 | 2,296 | 16.7 / 17.2 / 19.6 ms | 0 / 0 / 0 | 0.47 / 1.50 ms |
| 3x3 reduced | 18 | 612 | 16.7 / 17.2 / 18.1 ms | 0 / 0 / 0 | 0.29 / 0.74 ms |
| 4x4 reduced | 18 | 611 | 16.7 / 17.3 / 19.5 ms | 0 / 0 / 0 | 0.38 / 1.02 ms |

On this machine the scene is far inside its frame: no frame was late, the
render thread spends about half a millisecond on a frame, and the longest
interval was 20 ms. (Drawing past the item, below, makes the view a tenth
bigger each way and added about 0.1 ms to the render thread's mean, from two
more processes of six plays each; the intervals did not change.) That says
nothing about a tablet's GPU, which does the work the render thread only
submits. The two things to measure on the device are the
GPU time of a frame with 16 dice and MSAA, and the first-frame cost above.

### Replay

`--auto --digest` checksums every pose of a play, frame by frame at 60 Hz with
the device still. The same board and seed gave the same checksum twice, a
different seed a different one, and reduced motion its own:

| Script | Board | Seed 100, then again | Seed 101 | Reduced, seed 100, then again |
| --- | --- | --- | --- | --- |
| Staged | 3x3 | `7cfdfd74`, `7cfdfd74` | `c5dc0064` | `4a2172c5`, `4a2172c5` |
| Staged | 4x4 | `ea3b8ceb`, `ea3b8ceb` | `cd5e72f7` | `e5250505`, `e5250505` |
| Weighted, on the stand-in stream | 3x3 | `8a6a57`, `8a6a57` | `b2bb6402` | `25436ec5`, `25436ec5` |
| Weighted, on the stand-in stream | 4x4 | `bb86ad02`, `bb86ad02` | `ce6d71b1` | `856e1c45`, `856e1c45` |

A checksum is of the poses in pixels, so it is for one scene size: the staged
rows are from the first window and the weighted from the later one (which is
why the reduced rows, the same script in both, differ between them).

Moving the script out of the scene file into `ShakeScript.js` left the stills
identical, pixel for pixel but one pixel one level off. What the checksum does
not cover is a live device: the same board with a different shake is a
different play, by design, and ends on the same cells.

### The drag

A mouse drag stands in for the device. The pointer's position at each event is
differentiated twice into an acceleration (the board is taken as 16 cm across),
so the stream is as jerky as the hand: a spike at every start and stop. Where
the pointer is in the scene tips the device up to 8 degrees, and its speed
twists it. Each sample goes to `pushMotion`, and to a stand-in for the shake
detector, which starts a play when the hand reads 0.8 g or more of shaking: so
the way to use it is to take hold of the board and shake it, and the dice start
to go when the hand does, and are held for as long as it keeps shaking, up to
3 s.

One measured play sent the window a scripted back-and-forth pointer path for
two seconds (`dragBetween`, so the pointer path through the scene is exercised
without a hand): the hand's shaking peaked at 5.9 g, which is a mouse being
swept across the board and not a hand holding a device, the play was 1.7 s, and
the longest interval was 17 ms. How it *feels* in a hand is unjudged.

### The light probe

| Probe | Cost on Direct3D 11 | On the software renderer |
| --- | --- | --- |
| None | First frame 356 ms (cold); a flatter look | Runs |
| Prefiltered at run time (Qt's sky) | +33 ms to create it; first frame 400 and 586 ms in two cold runs | **Never finished a run** in 4 minutes (twice): Qt's pre-filter pipeline failed to build, 2,813 times (below) |
| Baked in advance (the file) | None: first frame 410 and 419 ms in two cold runs | Runs |

On a GPU it does not matter to the cost, as for Cubeulator. On a software
renderer it decides whether the scene draws at all. Against the run-time probe
the baked file's pictures differ by at most 3 of 255 in a channel (mean 0.01 to
0.09), which is not visible; against no probe they differ by up to 43, mean 6,
because the ambient light is missing. **The probe is baked in advance, 1 MB.**

### The software renderer

Qt's bundled Mesa llvmpipe through OpenGL (Mesa 11.2.2, a GL 3.0 context), two
plays of each, the baked probe. A stand-in for a device with no GPU to speak
of; it is **not** a model of an iPad's GPU, and shows only which parts of the
scene cost the most when the pixels are the bottleneck.

| Play | Frames | Interval p50 / p99 / max | Over 20 / 25 / 33 ms | Render thread mean / max |
| --- | --- | --- | --- | --- |
| 3x3 full | 215 | 16.0 / 18.4 / 20.6 ms | 1 / 0 / 0 | 10.3 / 14.8 ms |
| 4x4 full | 229 | 17.1 / 22.3 / 24.0 ms | 26 / 0 / 0 | 13.6 / 20.8 ms |
| 3x3 rattle held | 505 | 15.9 / 18.2 / 28.3 ms | 1 / 1 / 0 | 10.8 / 14.5 ms |
| 4x4 rattle held | 514 | 16.6 / 19.2 / 20.1 ms | 1 / 0 / 0 | 12.8 / 16.7 ms |
| 3x3 reduced | 68 | 16.2 / 56.2 / 122.9 ms | 2 / 1 / 1 | 12.4 / 112.7 ms |
| 4x4 reduced | 55 | 20.4 / 28.3 / 29.7 ms | 31 / 4 / 0 | 16.3 / 24.8 ms |

The first frame after creating the scene took 0.9 s (0.8 s with no probe).
A frame is 10 to 14 ms of the CPU's rasterising, so 60 Hz is held at 3x3 and
almost at 4x4. The reduced-motion plays are the heaviest, which is the part of
the scene drawn blended, and one frame of the 3x3 reduced plays took 123 ms
(85 ms with no probe): the warm-up did not remove it on this renderer.

**The run-time probe did not work on it.** The pre-filter's shaders were not
there for the GLSL this context accepts (it looked for 130 and 120; the file
has 100, 150 and 300 es), so Qt logged "No GLSL shader code found" and "failed
to create pre-filter env map pipeline state" 2,813 times in 4 minutes, and the
run never produced a result (the first attempt, with a 280 s limit, did not
either). This is a limit of that Mesa and not of CPU renderers in general
(Cubeulator got 10 to 16 s on another Mesa). What it shows is that a probe
prefiltered at run time can stop the scene drawing where the GL is old, and a
file cannot. The baked probe ran on it as above.

### The weighted script in the same harness

Direct3D 11, the baked probe, four plays of each case after the first: 2,978
frames, the longest interval 23 ms (two frames of 20 and 23 ms, in a held
shake), and the render thread's mean is 0.35 to 0.6 ms a frame, the same as
staged. The simulation (8 steps a frame, 16 dice, pair tests only between dice
within a die and a half of each other) does not show in the frame times.
Checksums for replay are in [Replay](#replay).

## What it looked like

The author's first look at the staged version, running it: **it had no weight.**
Everything moved on curves, the dice did not collide or slide into their cells
the way a real set does (curved corners, a tap to seat them), and there was no
sense of mass. Looking straight down was right for how the board is held and
shaken, and 3x3 was fine. Hence the weighted script, and top-down as the
default (`lean` 0).

The second look, at the first weighted version: **it had weight, and did not
react.** The box was still being driven by a prescribed sine wave and one number
for how hard, so the dice answered a curve and not a motion. A real device gives
a spiky stream on several axes, and the dice should have inertia and bang
against one another as that stream pushes them. Hence the input is now the
device's own signal (six axes, a hundred samples a second), and the dice are
simulated in the box's frame from it.

The third look, at the version with the dice in pockets and spikes in the
stand-in stream: **not good.** It shook for too long. It did not have the
smoothness of real dice answering a sharp shake: it had a small, acute
vibration, the dice were not free to move about the box (a real one is a
volume, the dice move through it in three dimensions, fall back by gravity into
their slots, and sometimes want a tap to seat), and the stand-in data may have
been poor. What was wrong, found by measuring and not guessing: the dice
buzzed against walls only a fifth of a tile away, the stream had single-sample
spikes of up to 2 g that were each a shock, the tip was an underdamped spring
that rang, and the box itself shivered with every stroke. Hence the dice in a
box with room, contact as impulses, a hand with no spikes, a tip that does not
ring, and a still box.

![The weighted shake, 4x4, straight down: ten times, then the settled board with the 2D tile layer over it](shake-prototype-filmstrip.png)

Stills at fixed times of one board (`--auto --shots`), so they are the same on
every run. They were read from stills by whoever built the scene, not by a
child, and nobody has yet watched the weighted motion: that is the author's to
judge by running it. **Nothing plays `landed`, and sound is much of what makes
a die feel heavy:** Qt Multimedia is not in the shared tree, so the prototype
is silent, and the feel is judged without it.

- **Weighted, straight down (above).** The dice jostle in a box with room, in
  a stream of small slides, turns and knocks, with some of them off the floor
  (bigger, and with their shadows moved away from them) (350 to 1000 ms); then
  the lid goes, and each die hops from wherever it has got to to its own cell,
  the dice that had traded places crossing over the others on the way, and
  lands with a small rebound (1150 to 1650 ms). Height is shown only by size
  and by the shadow, because the camera is straight down.
- **Faults.** Dice are squares in the plan and never rolled over, so what shows
  of a tumble is a turn and a rock; the hop home is a clean arc and is the
  most scripted-looking part; two dice can cross for a moment in the air on the
  way home; the dead jewel is a cube with four facets painted on, which looks
  wrong from the side; and nothing is heard.

Staged, leaning 24 degrees, for comparison:

![The staged shake, leaning](shake-prototype-filmstrip-staged.png)

- **At rest it reads well**, at 3x3 and 4x4. Letters upright, cream face and
  dark edge, the dead jewels distinct, and the 2D layer's outlines sit on the
  dice's tops to the pixel.
- **Staged: the rattle reads as a box**, the drop as dropping, and straight
  down it was a grid shaking under a pane. It was also what the author found
  weightless.


## If a 2D implementation is swapped in

Everything the application knows is the table in [The interface](#the-interface).
What a 2D `BoardScene` has to keep:

1. **The same name and members**, from the same module. QML has no interface
   type, so the contract is the list and a test that loads either file and
   checks it. The build picks the file (two files, one type name, chosen in
   CMake), not a preprocessor test.
2. **The same script.** `ShakeScript.js` has no renderer in it: `create`,
   `advance` and `evaluate` give each die's position, three angles, opacity and
   shadow, and the box, lid and camera. A 2D scene draws that: `px`, `py` as
   `x`, `y`; `pz` as `scale` and the shadow's size and opacity; `rz` as
   `rotation`; `rx` and `ry` dropped, or a skew; the lid as a rectangle that
   fades. Then a replay is the same, the landing order and the clicks are the
   same, and `digest()` is the same string in both.
3. **The same geometry.** `boardRect`, `tileSize` and `tileGap` by the same
   rule, and a die at rest exactly on its tile, since the tile layer, the path
   and the panel are 2D in both and do not move.
4. **The same events.** One `settled()` for a play, after `busy` is true and
   before it is false; `landed(index, pitch)` from the script; `rattled()` at
   most 8 a second; `play()` while busy restarts; `show()` makes no motion and
   emits nothing.
5. **The same inputs, the same meaning.** The motion samples change how the
   dice move and how long the box rattles and never where a die lands; with
   none, a seeded stand-in shakes; `reducedMotion` takes the cross-fading path
   of every stage, 0.54 s in all.

What it does not carry: `lean`, `probeMode`, `cameraDistance`, `warmUp`, and
the 3D-only files (`Die.qml`, `FaceAtlas.qml`, the probe). It loses the lit
solid and the true tumble about the horizontal axes; the stages and their
durations are the same. The weighted script is a shared part, not a 3D one: a
2D scene draws the same positions, yaw, heights (as scale and shadow) and
events from it.

### What changed in the seam

The prototype suggested two changes to the seam before the application is
built on it:

- **Done: the scene draws past its item.** `overdraw` (half a tile) is how far,
  and the view is that much bigger on every side. The item is still the
  board's area and `boardRect` is still by the rule, so the layout is not
  changed; the application must leave the item unclipped, and the window needs
  room around the board (the harness gives it a margin above, and the stills
  and the video are 720 wide). 3D only: a 2D scene may report any `overdraw`,
  including 0.
- **A rule for the application, no code:** the first board's `show()` is where
  the one-time costs go, so the application creates the scene at start-up with
  the first board and not on the first press of Shake.

## What was not measured

- **Anything on an iPad, a tablet or a phone**, and the GPU time of a frame on
  any device. The next step is a build for iOS (the shared tree has no kit) and
  Qt's GPU timestamps on the device.
- Power and heat, memory beyond what is noted, and a second desktop GPU.
- A real accelerometer and gyroscope. The scene takes samples; what has been
  fed to it is the seeded stand-in and a mouse (a pointer path differentiated
  twice), and a person's hand on the drag is unjudged ([above](#the-drag)).
- **Sound**, the accelerometer and haptics. The scene emits `rattled` and
  `landed` and the harness shows a count; nothing plays them, and no run
  checked the counts against the script.
- Whether the tumble reads at a child's eye height. See
  [Unknowns](board.md#unknowns).
