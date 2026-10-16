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
| The scene | `src/app/prototype/shake/qml/BoardScene.qml` | The component: a `View3D`, the box, the lid, one `Model` for each die, a letter atlas, a baked light probe. Draws the poses the script gives it |
| The script | `qml/ShakeScript.js` | The shake, lid and landing as a function of the board, the seed and the time, in two ways: prescribed curves (staged) or the simulated dice. Keeps the clock, the stages, the box, the lid and the camera. Plain JavaScript, no QML and no renderer |
| The dice | `sim/` (the `wjs_dice_world` library), `dice_sim.*`, `die_geometry.*` | Rigid cubes in a box, simulated from first principles by sequential impulses, in about 1,400 lines of plain C++ with no Qt, and two thin objects over it for QML: the simulation, and the mesh of a bevelled die whose six letters each turn a quarter turn at a time. Tested without a window by `tests/prototype/dice_world_test.cpp` |
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
| `landed(index, pitch)` | signal | A die has come down on its cell: its place in the board, and a pitch for its click |
| `rattled()` | signal | The box rattles, at most 8 a second, for the haptic tap |
| `busy`, `rows`, `columns`, `tileSize`, `tileGap`, `boardRect` | read-only | The geometry by [the rule](board.md#the-rule), for the 2D layer that traces over the scene |
| `overdraw` | read-only | How far past its item the scene draws, on every side (0.8 of a tile), so a die tumbling at the edge and the tipping rim are not cut off. The item is still the board's area; the application must not `clip` it |

`lean`, `probeMode`, `cameraDistance`, `warmUp`, `weighted` and `mockBurstMs`
are settings of the 3D implementation and are not part of the seam. `seek(board, seed, ms)` and
`digest(board, seed)` are for tests and pictures.

### The two scripts

**Simulated** (the default; the property is still called `weighted`) and
**staged** are two ways of making the same play, chosen by `weighted`; reduced
motion is the same cross-fade in both. The harness has a switch for it so they
can be compared in one window.

**Staged** is [the stages of the board page](board.md#the-stages) as prescribed
curves: dice hover a tile above their cells and jitter on sine waves, then fall
in a seeded order on a parabola with one rebound, and the camera comes upright
over the fall.

**Simulated** is the same play with the stages kept (the shake for as long as
the device shakes, 0.4 s at least and 3 s at most; the lid; the dice go home)
and the dice in between simulated, from first principles, as rigid cubes in a
box. This is the fourth version of the motion, and the first that is a
physical model (see [What it looked like](#what-it-looked-like)).

- **Rigid cubes with mass.** A die is a cube of one tile's edge and one unit of
  mass, so its moment of inertia is 1/6 about every axis through its centre (a
  cube has no gyroscopic term). A tile is taken as 20 mm, so 1 g is 490.5 tiles
  a second squared and time is real: a die falls a tile in 64 ms. Position,
  orientation (a quaternion), velocity and angular velocity are integrated at a
  fixed 1,200 steps a second in double precision; the scene sees 20 steps a
  frame.
- **The input is the device's own signal.** In the box's frame a die is
  accelerated by *minus what the accelerometer reads*: gravity, a tilt, a shove
  and a jolt are one thing, and a spike in the reading is a spike in the dice.
  The gyroscope adds the centrifugal, Coriolis and Euler terms of the box's
  turning, to the position and to the die's own spin. That is six axes, about a
  hundred samples a second, each taken as the reading for its share of a
  frame, so a jerky hand is a jerky shake.
- **A box with room in it.** A floor, four walls 1.4 tiles past the board on
  every side, and a lid 3 tiles up, so that a die can leave the floor, roll
  over, stack on another and slide off, hit the lid, and tumble. (At 0.6 of a
  tile past the board and a lid at 2.4 the sixteen dice were a packed grid that
  could only slide as one piece, and 0 or 1 of them rolled over in a shake.)
  The camera stands back while the box is shaken, so that all of it is in the
  item, and comes in as the dice go home, to be square by the time the last is
  down; the board is at its own scale at rest, and the 2D layer over it is
  unchanged.
- **Contact.** Two dice are tested on all fifteen separating axes; a face
  against a face is clipped to up to four contact points and an edge across an
  edge gives one. A die's eight corners are tested against each plane of the
  box. Points within 0.03 of a tile of touching are held too, so a die cannot
  close on a surface in one step. Each contact is solved by sequential
  impulses: an impulse along the normal that can only push, and two friction
  impulses held inside the cone of the normal impulse, accumulated and carried
  to the next step for the contacts that persist (warm starting), six passes a
  step, with what overlap is left pushed out in position alone so it adds no
  energy. A fast impact bounces by the contact's restitution, and the closing
  speed it is taken from is read before any carried impulse is applied. Floor
  0.52 restitution, 0.5 friction; walls 0.45 and 0.2; lid 0.38 and 0.2; dice
  against dice 0.60 and 0.3 (hard plastic dice on a hard board; the first
  values, 0.25 to 0.40, were dead: they did not bounce as the author expects,
  and it was the bounce, more than anything, that made dice roll over).
  **These are starting values, not measured from real dice.** Each die's grip and bounce differ from the next's by up to 15%,
  seeded, as no two real ones are alike.
- **Home (the guide).** The physics decides how a play looks and never where a
  die ends, because the board is decided before the play. When the reading goes
  still, each die is given a cell (the assignment that makes the dice travel
  least in the plan, by the Hungarian method) and shows that cell's tile from
  then on, the lid lifts, and from a moment of its own in the seeded order each
  die is kicked up (a hop of 0.3 to 1.6 tiles, by how far it has to go; the
  dice that have furthest to go start first, so that they cross the box while
  it is still open), pulled toward its cell by a spring in the plan and turned
  flat and square to the board by a spring on its orientation, its friction
  nearly off. The dice still collide on the way. A die that is on the floor
  and not getting closer (not by 0.05 of a tile in 0.07 to 0.19 s, a time of
  its own, so that two that are stuck against one another do not hop in step)
  hops again, higher, over the others. When every die has been on its cell,
  flat and still, for 40 ms, they are put exactly there and the play is over.
  If that has not happened 1.8 s after the last die started, they are put
  there anyway; in 900 shakes of 3x3, 4x4 and 5x5 in the simulation alone, with
  the scene's stand-in, none needed it (the slowest took 2.4 s from the end of
  the shake, at 5x5).
- **The letters.** Every face of a die carries the tile of the cell it is going
  to, so nothing shows when the assignment is made under the lid. A die is a
  `DieGeometry`, a cube with a small bevel whose six faces each have a
  quarter-turn setting for the letter. Once the lid is going, the face that is
  up has its letter turned to read upright for the way the die would lie if it
  were squared to the board, so by the time a die is down its letter reads
  upright, and a die that lands on a side shows the letter on that side.

With no device there is a stand-in, `mockSampleAt(seed, k)`, a function of the
seed and the sample number. It is meant to be a hand, not a signal generator:
a burst of 0.8 to 1.2 s of strokes at three to five hertz that wanders, back
and forth along an axis that drifts, reaching 2.4 g at the turns, with the up
and down of the arm (to 2.4 g) so that dice leave the floor, a little ripple
from the box, small knocks now and then, noise on every sample, a slow tip of
the device, and a twist on the gyroscope that goes with the strokes. The Shake
button, a replay and the tests use it, so they are the same shake every time;
a real device, or the drag in the harness, is live.

It is seeded and replays: the same board, seed and stream give the same
checksum, because the step is fixed, the arithmetic is double precision, and
the only random numbers are a generator seeded from the seed. That holds for
one build on one machine; it is **not** promised across compilers or
processors, whose floating-point rounding (fused multiply-add, for one) can
differ, and a replay that has to be exact across them would need the dice
stored, or an integer simulation.

| Play (simulated) | Measured, play to `settled` | What it is |
| --- | --- | --- |
| 4x4, full motion | 1.72 to 2.52 s | A 1.05 to 1.28 s shake (a burst of 0.8 to 1.2 s and the time for the reading to go still), the 0.25 s of the lid, and 0.47 to 0.88 s for the dice to go home (the lid lifts while the first dice go) |
| 3x3, full motion | 1.44 to 2.34 s | The same |
| Either, reduced motion | 0.55 to 0.57 s | 0.30 rattle, 0.12 lid fade, 0.12 dice fade |
| Rattle held at full | 3.5 to 4.6 s | A 4 s burst, cut at the 3 s limit, then the lid and the hops |
| The mouse drag, 2 s of it | 1.9 to 2.4 s | A scripted back-and-forth pointer path, with the arm's up and down added |

(Staged: 1.97 s at 4x4 and 1.73 s at 3x3, with a 0.87 s drop.)

How it moves, from the script run by itself, four seeds on the 4x4, as the
screen at 60 Hz sees it (a tile is the die's edge):

| | |
| --- | --- |
| Shake, then the lid, then home | 1.05 to 1.28 s, then 0.25 s, then 0.47 to 0.88 s |
| Speed of a die, peak; 95th percentile | 64 to 78 tiles a second; 44 to 52 (1.3 to 1.6 m/s for a 20 mm die: a hand's stroke) |
| Distance a die moves in a frame | median 0.10 to 0.16 of a tile, 95th percentile 0.73 to 0.86, none over 1.3 |
| Turn in a frame | 95th percentile 17 to 22 rad/s; none over 1.0 rad a frame |
| Acceleration seen at 60 Hz (second difference), 99th percentile | 5.4 to 6.2 g |
| Dice off the floor by more than 0.15 of a tile | 36 to 45% of die-frames; all sixteen at once at some frame in every play |
| Highest a die's centre gets | 2.5 to 4.2 tiles (the lid is at 3; the highest are thrown past it as it lifts) |
| What an average die does in a play | 24 to 33 tiles of path; 395 to 632 degrees of turning |
| Dice that end the shake on a different face than they began on, over 300 shakes of 4x4 with 2.4 g strokes in the simulation alone | 10.7 of 16, turning 627 degrees each (3x3: 6.7 of 9, 402 degrees; 5x5: 14.7 of 25, 890). At 0.6 of a tile of room, 0.3 of 16, 110 degrees |
| Deepest overlap between two dice, or past a wall, over 300 plays of 4x4 and 100 of 3x3 with the tests' gentler stand-in (every 50 ms) | 0.05 of a tile between dice or into a wall; 0.03 of a tile outside the box |
| Shakes in which the dice did not all reach their cells, in the simulation alone | 0 of 300 each of 3x3, 4x4 and 5x5 with the scene's stand-in (the slowest took 1.2, 1.6 and 2.4 s from the end of the shake), and 0 of 300 (4x4) and 0 of 100 (3x3) with the tests' gentler one (0.9 to 1.1 s) |

### The dice, checked against paper

The simulation is validated by what can be worked out on paper, in
`tests/prototype/dice_world_test.cpp` (20 cases, 1,746 checks, 2.5 s, run by
`ctest` with the rest when the option is on), and the numbers below are what
those cases measure, with identical dice (the 15% variety off) and no air:

| Case | Paper says | The simulation gives |
| --- | --- | --- |
| Free fall, 0.1 s | s = g t²/2, 17.5475 tiles from 20 | 17.5271; the 0.8% of the fall is the first-order step, half of g h t |
| The box accelerates (0.5, -0.25) g | the die goes the other way by half of 0.5 g t² | -1.2365 and 0.6182 against -1.2263 and 0.6131 |
| A box turning at 2 rad/s, a die 3 tiles out | centrifugal: ω² r t = 0.1200 in 10 ms | 0.1200; the Coriolis term's sign and the Euler term's size are checked too |
| A free die spinning at 3.74 rad/s for 1 s | the same spin, turned by 3.74 rad | off by less than 0.00001 rad |
| A die dropped flat, from 2 tiles, restitution 0.52 | rebounds e² h = 0.5408 | 0.5518 (+2.0%) |
| A die sliding from 20 tiles/s, friction 0.5 | stops after v²/(2 μ g) = 0.8155 | 0.8071 (-1.0%) |
| A slope of 20° (tan 0.36, under the friction 0.5) | holds | moved 0.00000 in 1.5 s |
| A slope of 35° (tan 0.70, over it) | slides g (sin a − μ cos a) t²/2 = 3.6199 in 0.3 s | 3.6302 (+0.3%) |
| Two dice head on, 10 tiles/s, restitution 0.6 | 2.000 and 8.000 afterwards | 2.001 and 7.999; momentum 10.0000 |
| An off-centre hit | momentum conserved, energy not made | (12.00000, 0, 0) of (12, 0, 0); 72.0 to 53.1 |
| A stack of three dice, 2 s | stands | drift 0.0002, height error 0.0002 |
| A die 0.3 off the centre of the one under it | stands (the centre of mass is over the support) | 0.30025, no turn |
| A die tipped 20°, 40°, 50°, 70° on an edge, 2 s | falls flat, onto its own face below 45° and onto another above | flat (an axis within 0.03 rad of vertical), as predicted |
| A die thrown at the lid and the walls | stays in the box | never more than 0.05 of a tile out (the check's limit) |
| Sixteen dice shaken hard | stay in the box, out of one another | 0.05 of a tile at the deepest |
| A replay | is the same replay | the same checksum, a different one for another shake |
| The least-cost assignment | is the least cost | equals the best of all 720 permutations, 30 random matrices |
| The die that is up, and its letter's turns | the letter reads upright | 60 random orientations |

What this is **not**: it is not compared with a real die, a real box, or a
real hand. It does not model a die's rounded corners (the bevel is drawn, not
simulated), felt or board compliance, air, a shaker's walls being anything but
rigid, or sound. The friction and bounce are plausible starting values and
the author is the judge of whether it *feels* right.

**Cost.** In plain C++ in Release on this desktop, a 4x4 play costs 1.24 ms
of a frame on average (20 steps), 2.9 ms at the 99th percentile and 4.7 ms at
worst, which is 62 µs a step; a 3x3 costs 0.59 ms on average. This is on the
GUI thread, with the script, and a tablet's core is slower, so it is the first
thing to measure on one. Four passes instead of six cost about a fifth less in
the same test, with the dice a little less exact (a die at rest flat to 0.011
rad and the deepest overlap 0.037 of a tile), and 600 steps a second would
halve it again; neither was tried at the scene.

## How it was measured

| | |
| --- | --- |
| Machine | Windows 11 Pro for Workstations 10.0.26200; AMD Ryzen 9 5950X (16 cores, 32 threads); NVIDIA GeForce RTX 4090 (driver 32.0.16.1742). Shared with other work, not quiesced |
| Renderer | Qt RHI on Direct3D 11, threaded render loop, flip-model swap chain, MSAA Medium. Qt reports the window's display at 60 Hz (a vsync of 16.68 ms) |
| Software | Qt 6.12.0 (`msvc2022_64` of the shared tree), MSVC 19.51, Release (`/O2`), QML compiled to C++ ahead of time |
| Window | 560 x 900 logical pixels, the scene in the top two thirds. The tables are from the **staged** script, which was the only one when they were taken; the simulated dice are under [the simulated dice in the same harness](#the-simulated-dice-in-the-same-harness) |
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
build\windows-msvc-release\bin\ShakePrototype.exe --auto --digest --out digest    # replay check, and a trace of every pose
build\windows-msvc-release\bin\wjs_dice_tests.exe                                  # the dice's physics against paper, no window
ctest --preset windows-msvc-release                                                 # everything, with the option on
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

`--auto --digest` checksums every pose of a play (the quaternions too, when the
dice are simulated), frame by frame at 60 Hz with the device still, and writes
the pose of every die at every frame (`trace-100.txt` and three more) for
looking at the motion. The same board and seed gave the same checksum twice, a
different seed a different one, and reduced motion its own:

| Script | Board | Seed 100, then again | Seed 101 | Reduced, seed 100, then again |
| --- | --- | --- | --- | --- |
| Staged | 3x3 | `7cfdfd74`, `7cfdfd74` | `c5dc0064` | `4a2172c5`, `4a2172c5` |
| Staged | 4x4 | `ea3b8ceb`, `ea3b8ceb` | `cd5e72f7` | `e5250505`, `e5250505` |
| Simulated, on the stand-in stream | 3x3 | `e47beae6`, `e47beae6` | `daedff55` | `25436ec5`, `25436ec5` |
| Simulated, on the stand-in stream | 4x4 | `1fd23609`, `1fd23609` | `f75cbf8` | `856e1c45`, `856e1c45` |

A checksum is of the poses in pixels, so it is for one scene size: the staged
rows are from the first window and the simulated from the later one (which is
why the reduced rows, the same script in both, differ between them). The
simulated rows are for this build on this machine, as [above](#the-two-scripts)
says.

Moving the script out of the scene file into `ShakeScript.js` left the stills
identical, pixel for pixel but one pixel one level off. What the checksum does
not cover is a live device: the same board with a different shake is a
different play, by design, and ends on the same cells.

### The drag

A mouse drag stands in for the device. The pointer's position at each event is
differentiated twice into an acceleration (the board is taken as 16 cm across),
so the stream is as jerky as the hand: a spike at every start and stop. Where
the pointer is in the scene tips the device up to 8 degrees, and its speed
twists it. A mouse cannot lift the device, and with no lift the dice have no reason
to leave the floor and slide as one block (the author's report), so the drag
adds the up and down of an arm: the vertical reading follows the sideways
acceleration (0.8 of it, to 4 g). Each sample goes to `pushMotion`, and to a
stand-in for the shake detector, which starts a play when the hand reads 0.8 g or more of shaking: so
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

### The simulated dice in the same harness

Direct3D 11, the baked probe, three processes of six plays of each case, each
process's first play left out: 12,445 frames. The longest interval was 20.4 ms
(none was over 25 ms), and the render thread's mean is 0.40 to 0.63 ms a
frame, with a worst frame of 4.8 ms: the same as staged. (The display ran at
12 to 13 ms in these runs and 16.7 ms in the tables above: the
[two cadences](#two-cadences).) Making the scene took 3.3 to 3.9 ms and its
first frame 228 to 232 ms after that, as before.

The simulation does not show in those numbers, because it runs on the GUI
thread, ahead of the frame, and they are the render thread's. Its own cost is
under [the two scripts](#the-two-scripts): 1.24 ms of a frame on average for a
4x4 play, and the whole prototype, playing, uses 8 to 18% of one core and 1 to
2% of the GPU's 3D engine on this machine, and idle it uses under 1% of a core
and none of the GPU. Checksums for replay are in [Replay](#replay).

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

The fourth look, at the dice in a box with room: **it still did not model
the dice rolling about in the volume.** The author's words: the dice should
roll about in the box when it is shaken, and the answer is to model it from
first principles, with the weights, the mass and the inertia of real dice. The
squares in the plan, however well their contacts were handled, could slide and
turn and never roll, tip over an edge, land on a side or tumble in three
dimensions, and the hop home was a scripted arc. Hence the rigid cubes of
[the two scripts](#the-two-scripts), checked against what can be worked out on
paper before they were put in the scene.

(The author also said the desktop became laggy while this was being tested and
demonstrated. The scene is not what did it: playing, it uses 8 to 18% of one
core and 1 to 2% of the GPU's 3D engine, and idle it uses nothing. It was the
builds, the test runs and the stills and videos, which used every core. They
now run at below-normal priority and with at most eight jobs.)

The fifth look, at the rigid dice, shaken with the mouse and with the button:
**they shook, and did not roll.** They did not bounce as a real set does, and
they moved "stuck together", as one piece. What was wrong, found by measuring
and not by guessing (the simulation alone, 120 shakes of the 4x4, with the
number of dice, of 16, that end a shake on a different face than they began,
and the turning each did):

- **The drag had no up and down.** A mouse moves the device sideways only, so
  nothing lifted a die off the floor and the pack slid as one piece against a
  wall: 0 of 16 rolled over, and each die turned 23 degrees in a shake.
- **The box was a packed grid.** With 0.6 of a tile of room the dice could not
  get an edge up even with 2 g of up and down: 0.3 of 16 rolled over, and each
  turned 110 degrees.
- **The bounce was dead.** Restitution of 0.25 to 0.40. At 1.5 times that, in a
  box with room, 9.3 of 16 rolled over, against 3.6 without it; friction
  mattered the other way (halving it, 1.4).
- **The stand-in was gentle**, 1.7 g.

Hence the box with 1.4 tiles of room and a lid at 3, the camera that stands back
for the shake, a bounce of 0.38 to 0.60, a stand-in of 2.4 g, and a drag with an
up and down in it. In the simulation alone that gives 10.7 of 16 dice rolled
over in a shake and 630 degrees of turning each (3x3: 6.7 of 9; 5x5: 14.7 of
25). It also broke the way home: dice that had to cross the box found the cells
filled, and in 2 shakes of 120 two dice had swapped sides and gear-turned
against one another for ever. The guide now sends the dice that have furthest
to go first, has a stiffer spring, and hops again a die that is not getting
closer, each after a time of its own.

![The simulated shake, 4x4, straight down: ten times, then the settled board with the 2D tile layer over it](shake-prototype-filmstrip.png)

Stills at fixed times of one board (`--auto --shots`), so they are the same on
every run. They were read from stills by whoever built the scene, not by a
child, and nobody has yet watched the simulated motion: that is the author's to
judge by running it. **Nothing plays `landed`, and sound is much of what makes
a die feel heavy:** Qt Multimedia is not in the shared tree, so the prototype
is silent, and the feel is judged without it.

- **Simulated, straight down (above).** The dice start in their cells. At
  350 ms they have been thrown apart, turned and tipped, and the camera has
  stood back to show the whole box, with its rim and the translucent lid
  (the pale frame). At 800 ms the box tips with the device and the pack has
  slid and stacked against the low side, a die thrown up toward the lid
  (bigger, with its shadow away from it). From 1000 to 1450 ms they roll
  across the box, show their sides and their other faces, and bounce off one
  another and the walls; then the lid has gone, and each die in turn is
  kicked up and goes to its own cell over and between the others, turning
  flat as it goes (1300 to 1900 ms), while the camera comes in so that the
  board is at its own scale when the last is down. Height is shown by size and
  by the shadow.
- **Faults.** The hops home are the most scripted-looking part: the guide is a
  spring and a kick, and a die hopping over others can look like a spring
  toy; all sixteen rise together in a vertical stroke, because in the box's
  frame the same acceleration pushes every die the same way, and the balance
  of up and sideways in the stand-in is a guess; the letters on the sides of
  a tumbling die are not turned to read (only the face that is up is, once the
  lid goes); a die's bevel is drawn, not simulated; the dead jewel is a cube
  with four facets painted on, which looks wrong from the side; a die at the
  lid is a third bigger and can overlap the controls in the harness; the camera
  standing back makes the board small for the shake; and nothing is heard.

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
2. **The same script, and the same dice.** `ShakeScript.js` has no renderer
   in it: `create`, `advance` and `evaluate` give each die's position and
   orientation (three angles when staged, a quaternion when simulated), its
   opacity and shadow, which cell's tile it shows (`cell`) and which faces are
   to be turned (`turnChanges`, for the 3D mesh), and the box, lid and camera.
   The simulated dice are plain C++ with no Qt (`wjs_dice_world`), so a 2D scene
   uses the same simulation. It draws `px`, `py` as `x`, `y`; `pz` as `scale`
   and the shadow's size and opacity; the yaw (`rz` when staged, the heading of
   the quaternion when simulated) as `rotation`; the tilt dropped, or a skew;
   the tile of `cell`; the lid as a rectangle that fades. Then a replay is the
   same, the landing order and the clicks are the same, and `digest()` is the
   same string in both.
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
the 3D-only files (`Die.qml`, `DieGeometry`, `FaceAtlas.qml`, the probe). It
loses the lit solid and the sight of a die rolling over; the stages and their
durations are the same. The simulation is a shared part, not a 3D one: a 2D
scene draws the same positions, yaw, heights (as scale and shadow) and events
from it, and its dice still collide, stack and tumble where nobody can see them
turn over, which is a strange thing to pay for, so a 2D scene may well prefer
the staged script and give up the physics too.

### What changed in the seam

The prototype suggested two changes to the seam before the application is
built on it:

- **Done: the scene draws past its item.** `overdraw` (0.8 of a tile) is how far,
  and the view is that much bigger on every side. The item is still the
  board's area and `boardRect` is still by the rule, so the layout is not
  changed; the application must leave the item unclipped, and the window needs
  room around the board (the harness gives it a margin above, and the stills
  and the video are 720 wide). 3D only: a 2D scene may report any `overdraw`,
  including 0.
- **The scene's box is bigger than the board while it is shaken**, 1.4 tiles
  past it on every side, and the camera stands back so that all of it is inside
  the item, then comes in as the dice go home. Nothing in the interface
  changes, but a 2D layer over the scene has to be the one the settled board
  needs (it is drawn once the dice are down) and the board is smaller on
  screen for the shake. 3D only.
- **A rule for the application, no code:** the first board's `show()` is where
  the one-time costs go, so the application creates the scene at start-up with
  the first board and not on the first press of Shake.

## What was not measured

- **Anything on an iPad, a tablet or a phone**, and the GPU time of a frame on
  any device. The next step is a build for iOS (the shared tree has no kit) and
  Qt's GPU timestamps on the device.
- Power and heat, memory beyond what is noted, and a second desktop GPU.
- Whether the simulated dice *feel* right. The friction and the bounce are
  plausible starting values and were not compared with real dice, a real box
  or a real hand; the physics was checked against what can be worked out on
  paper ([above](#the-dice-checked-against-paper)) and nothing else.
- The simulation's cost on a tablet's core (1.24 ms of a frame on average on
  this desktop, 2.9 ms at the 99th percentile), and its replays on another
  compiler or processor, which are not promised to match.
- A real accelerometer and gyroscope. The scene takes samples; what has been
  fed to it is the seeded stand-in and a mouse (a pointer path differentiated
  twice), and a person's hand on the drag is unjudged ([above](#the-drag)).
- **Sound**, the accelerometer and haptics. The scene emits `rattled` and
  `landed` and the harness shows a count; nothing plays them, and no run
  checked the counts against the script.
- Whether the tumble reads at a child's eye height. See
  [Unknowns](board.md#unknowns).
