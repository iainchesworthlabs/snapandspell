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
| The scene | `src/app/prototype/shake/qml/BoardScene.qml` | The component: a `View3D`, the box, the lid, one `Model` for each die sharing one six-sided cube mesh, a face atlas, a baked light probe. Draws the poses the script gives it |
| The script | `qml/ShakeScript.js` | The shake, lid and landing as a function of the board, the seed and the time, in two ways: prescribed curves (staged) or a small integrator of our own (weighted); and which face of each die is up and how it is turned over on the way. Plain JavaScript, no QML and no renderer |
| The faces | `qml/FaceAtlas.qml`, `qml/Die.qml` | One texture for the board's dice, a block of 3 x 2 cells for each, a cell to a face; each die's cube samples its own block. A dead jewel is a die of six cells of four facets, drawn once and shared. The scene holds two atlases, so that a board can be drawn ahead (`prepare`) |
| The harness | `qml/ShakeWindow.qml`, `frame_stats.*`, `dice_roller.*`, `main.cpp` | A window with a Shake button, a mouse drag standing in for the accelerometer, a reduced-motion switch, a 2D tile layer over the scene, the library's dice roll (`DiceRoller`, which links `wjs::lib` and embeds `data/dice`), and the timing and measurement runs |
| The probe | `tools/lightprobe`, `qml/lightprobe/sky.ktx` | Qt's procedural sky prefiltered in advance (128 x 128 texels a face, 1 MB), made by Cubeulator's tool over the same sky |

It is behind `-DWJS_BUILD_SHAKE_PROTOTYPE=ON`, off by default, and the default
build is unchanged: the option adds one program and finds one more Qt module.
The scene has no game rules in it: a face is an opaque string and `""` is a
jewel (a dead jewel is a die of six). Its script is also run by a ctest case
when the prototype is built (`tests/app/shake_script_test.cpp`).

### The interface

| Member | Kind | Meaning |
| --- | --- | --- |
| `play(board, seed)` | function | Shake, lid, drop, settle, for `board = { rows, columns, tiles, faces, up }`: for each tile, row-major, its letters, the six faces of the die on it, and which is up ([Real faces](#real-faces)). The dice are in the box at the first frame |
| `show(board)` | function | The board already down, with no motion: a resumed game, a rotated window |
| `prepare(board)` | function | Optional. Draws the board's faces ahead of `play` or `show` of that board, so that the frame in which it arrives has less to do ([the real faces](#the-real-faces)); nothing changes on screen. A board that was not prepared is drawn when it arrives, as before |
| `reducedMotion` | in | The cross-fading variant of every stage |
| `pushMotion(ax, ay, az, wx, wy, wz)` | function | One sample of the device's motion, about a hundred a second: the accelerometer's reading in g, gravity included (face up and still it reads 0, 0, 1; x right, y up the screen, z out of it), and the gyroscope's in rad/s. The dice answer to it sample for sample, and the rattle holds on, up to 3 s, while it reads as shaking. With none, a seeded stand-in shakes |
| `settled()` | signal | Once per play, when the dice have stopped |
| `landed(index, pitch)` | signal | A die has hit the floor: its place in the board, and a pitch for its click |
| `rattled()` | signal | The box rattles, at most 8 a second, for the haptic tap |
| `busy`, `rows`, `columns`, `tileSize`, `tileGap`, `boardRect` | read-only | The geometry by [the rule](board.md#the-rule), for the 2D layer that traces over the scene |
| `overdraw` | read-only | How far past its item the scene draws, on every side (half a tile), so a die tumbling at the edge and the tipping rim are not cut off. The item is still the board's area; the application must not `clip` it |

`lean`, `probeMode`, `cameraDistance`, `warmUp`, `weighted` and `mockBurstMs`
are settings of the 3D implementation and are not part of the seam.
`seek(board, seed, ms)`, `digest(board, seed)` and `faceReport(board, seed)` are
for tests and pictures.

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

## Real faces

The first prototype put one letter on all six sides of a die, because it was
given one string for each tile. A real die has six different letters and a shake
leaves one up, so the scene is now given the dice, as
[Board](board.md#what-the-scene-is-given) says: `faces[t]` is the six strings of
the die on tile `t` and `up[t]` is which is up, with `faces[t][up[t]]` the tile's
letters. The library supplies them: `wjs::roll_with_placements` returns the die
and the face for each tile of a roll, and the harness's `DiceRoller` turns a roll
of the shipped set (`data/dice`, embedded in the program) into that shape. The
scene reads no letter. The standard board of the harness is that roll; the
phonics board is still a stand-in (below).

| Part | How it is made |
| --- | --- |
| The mesh | One cube of six sides, built in QML by `ProceduralMesh` from the script's `sides` table (24 vertices, shared by every die), so the mesh and the orientations agree on which side is which face. It replaces `#Cube`, whose six sides all show the whole texture |
| The atlas | A block of 3 x 2 cells for each die, a cell to a face. A die's texture offset and scale pick its block, as they picked its cell; the mesh's coordinates pick the cell in it. The atlas is a layer, drawn once into one texture that every die's `Texture` reads, and its cells are items made as a board needs them and kept ([the cost](#the-real-faces)) |
| Which face is up | A quaternion for each die from the script, set on the cube under the Euler tips and yaw the body already had, so those are unchanged. At rest it is the face the board says, the letter upright |
| Weighted | A die starts on a random face with a random quarter turn of yaw, and turns a quarter turn about a horizontal axis, the way it was going, at each knock over 3.5 tiles/s (floor, lid, wall, neighbour; a half turn over 14). On its hop home it turns the short way to its up face, and lands on it |
| Staged | A die lies on a random face in the box and turns over as it falls, landing with its up face on top |
| Reduced motion | Nothing tumbles: the dice fade in on their up faces |
| A dead jewel | A die of six jewel cells, so every way up is the jewel. The jewel is drawn once into a texture of one cell, and each jewel cell shows that texture |

**The faces move nothing.** They are drawn from generators of their own and
nothing in the motion reads them. The poses of the previous script (positions
and the three angles, frame for frame at 60 Hz, for both scripts and both
sizes, checked against the branch's script in Node) are the same numbers, and the
landing is the same cells; what is new is the die's orientation, which the replay
checksum now covers. A ctest case holds the same against the file itself: with
another set of faces up on every die the positions and angles are equal at six
times through a play.

**What a viewer sees of the other faces.** `--auto --digest` plays a board and
counts, frame by frame at 60 Hz with the device still, the faces that were
uppermost at some frame (the tips of a die in flight, which show side faces
too, are not counted). Seed 7 of the library's dice, and of the stand-in, played
with seed 100:

| Board | Tiles that are their die's face up | Different letters on a die (mean) | Land face up, upright | Faces uppermost in a play (mean, most) |
| --- | --- | --- | --- | --- |
| Library dice 3x3 | 9 of 9 | 6.00 | 9 of 9 | 5.00, 6 |
| Library dice 4x4 | 16 of 16 | 6.00 | 16 of 16 | 4.50, 6 |
| Library dice 5x5 | 25 of 25 | 6.00 | 25 of 25 | 4.60, 6 |
| Library dice 6x6 | 36 of 36 | 6.00 | 36 of 36 | 4.97, 6 |
| Stand-in 3x3, 4x4, 5x5, 6x6 | all | 5.4 to 5.7 (a dead die has one) | all | 4.6 to 4.9, 6 |

Under reduced motion nothing tumbles (one face, 1.00) and every die lands as
asked. The staged script shows a die about two faces (the one it lay on and the
one it lands on, and what the turn passes between). The same checks, on six
boards (3x3 and 4x4, three seeds each), are the ctest case "the shake script
lands each die on the face the roll says and tumbles the others"; it fails if
the up face is ignored, and if the tumble is switched off.

**What it does not do.** The opposite faces of a die are the cube's arrangement,
not the die's (the library says which six letters a die has and nothing about
their neighbours). The dice that are shown tumbling are the dice of the final
board: the roll decided which die is on which tile, and the weighted script lets
the dice roam the box and trade places and then hops each to the tile it was
given, so the mixing of *which die is where* is drawn but decides nothing; in
the staged script and under reduced motion a die does not travel. The phonics
board's dice are a stand-in in the harness (a tile on one face and five others
drawn from the harness's pool, six different letters); what faces the phonics
board's dice have is an open choice that is not made here.

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
build\windows-msvc-release\bin\ShakePrototype.exe --auto --digest --out digest    # replay check, and the faces
build\windows-msvc-release\bin\ShakePrototype.exe --auto --change --sizes 3,4,5,6 --out change   # the cost of changing the board
build\windows-msvc-release\bin\ShakePrototype.exe --auto --change --prepare --sizes 3,4,5,6 --out prepare   # the same, with each board prepared a frame ahead (the frames of prepare() are the result's "prepare")
build\windows-msvc-release\bin\ShakePrototype.exe --boards phonics   # the stand-in board of the earlier tables (the default is the library's dice)
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

| Script, board (seed 7) | Seed 100, then again | Seed 101 | Reduced, seed 100, then again |
| --- | --- | --- | --- |
| Weighted, library dice 3x3 | `38cde292`, `38cde292` | `efd90ab4` | `c710afd9`, `c710afd9` |
| Weighted, library dice 4x4 | `d3fba177`, `d3fba177` | `f015d079` | `b226c069`, `b226c069` |
| Weighted, library dice 5x5 | `987c1268`, `987c1268` | `4dd96e54` | `fe1247e5`, `fe1247e5` |
| Weighted, library dice 6x6 | `b11deba2`, `b11deba2` | `a74de28b` | `709173dd`, `709173dd` |
| Weighted, stand-in 3x3 | `8aea8`, `8aea8` | `b5fd164b` | `d92b509d`, `d92b509d` |
| Weighted, stand-in 4x4 | `2af519a0`, `2af519a0` | `ac922da6` | `683142cd`, `683142cd` |
| Staged, library dice 3x3 | `e5396714`, `e5396714` | `50fbac07` | `c710afd9`, `c710afd9` |
| Staged, library dice 4x4 | `ed068f95`, `ed068f95` | `6b6fb2aa` | `b226c069`, `b226c069` |
| Staged, stand-in 3x3 | `53647a7e`, `53647a7e` | `16f63790` | `d92b509d`, `d92b509d` |
| Staged, stand-in 4x4 | `dc06b383`, `dc06b383` | `668a9f2` | `683142cd`, `683142cd` |

A checksum is of the poses in pixels (for the harness's window, 560 x 900) **and
now of each die's orientation**, so every earlier checksum is replaced, not only
by the new motion code: the board's faces are in it too, so the same seed on
another roll is another checksum. The reduced rows are the same script under
either name. What the checksum does not cover is the position and angles alone,
which the faces leave as they were: with the faces left out of the checksum the
poses of the previous script and of this one were equal, frame for frame, for
both scripts and both sizes (checked in Node against the branch's script).

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

### The real faces

What six cells to a die cost, against the branch as it was without them (its tip
when this was merged, `ac4e881`, built beside this one with the same compiler
and flags, with the same `--change` mode patched into its harness so the two
measure the same way). Three processes of each, one after the other on this
desktop, with nothing else drawing and no build running, which was checked every
second of every run (a run in which anything was seen was thrown away and made
again; an earlier series had been broken by another worktree's `ShakePrototype`).
The "after" is two boards: the stand-in the earlier tables used (two dead jewels,
each now a die of six jewel cells) and the library's dice.

**The atlas is six times the pixels.**

| Board | Before | Now | In memory (RGBA), before and now |
| --- | --- | --- | --- |
| 3x3 | 384 x 384, 9 cells | 1,152 x 768, 54 cells | 0.6 MB, 3.5 MB |
| 4x4 | 512 x 512, 16 cells | 1,536 x 1,024, 96 cells | 1.0 MB, 6.3 MB |
| 5x5 | 640 x 640, 25 cells | 1,920 x 1,280, 150 cells | 1.6 MB, 9.8 MB |
| 6x6 | 768 x 768, 36 cells | 2,304 x 1,536, 216 cells | 2.4 MB, 14.2 MB |

**A frame of a play costs about a tenth more.** Every play but the first of each
process, 30 plays a process, the stand-in's shake (the render thread's time to
record and submit a frame, as above; GPU time was not measured):

| | Before | After, stand-in board | After, library dice |
| --- | --- | --- | --- |
| Frames | 13,107 | 14,156 | 13,394 |
| Render thread, median | 0.44 ms | 0.49 ms | 0.47 ms |
| mean | 0.46 ms | 0.52 ms | 0.55 ms |
| p95 | 0.67 ms | 0.83 ms | 0.84 ms |
| p99 | 0.97 ms | 1.12 ms | 1.33 ms |
| largest | 2.5 ms | 2.4 ms | 59.1 ms |
| Intervals over 33 ms | 0 | 0 | 4 |

The median is 0.03 to 0.05 ms higher, in every kind of play (by 0.01 to 0.09 ms
in the mean): what the cube of six sides and an extra turn for each die add to
the frame. The one large number is one process of the nine, which had a stall (a
115.8 ms interval and a 59.1 ms frame, with 44 intervals over 20 ms) that its
two siblings and the other six processes did not; the cause was not found (an
earlier series of this measurement, against the branch's previous tip, had a
stall of the same kind in one process of the *before*).
The display ran at 100 Hz (10 ms) in all but the first process of the before,
which was a mix of the two cadences (the "two cadences" above), so the intervals
are not compared. Creating the view is unchanged within its spread: loading the
component 16, 16 and 17 ms (before, stand-in, library dice), creating the
objects 3.5, 3.9 and 4.0 ms, and the first frame after it 237, 251 and 240 ms
(228 to 238 ms of it the render thread's).

**The cost is in the frame in which the board changes.** `--auto --change` shows
a new board and records the frames after it, six boards of each size in each of
three processes (18 boards of each size for each kind). That frame draws the new
board's faces into the atlas. Means: the time from `show()` to the first frame on
screen, and what the render thread spent on that frame.

| Board | Before | After, stand-in board | After, library dice |
| --- | --- | --- | --- |
| 3x3 | 11.7 ms, 5.7 ms | 28.1 ms, 16.6 ms | 21.9 ms, 11.2 ms |
| 4x4 | 11.8 ms, 5.0 ms | 26.2 ms, 14.5 ms | 19.0 ms, 8.1 ms |
| 5x5 | 17.1 ms, 8.7 ms | 46.6 ms, 29.3 ms | 33.5 ms, 16.5 ms |
| 6x6 | 26.1 ms, 13.5 ms | 75.5 ms, 49.5 ms | 57.7 ms, 32.3 ms |

The frames after the first are nearly the same as before (the largest
render-thread time of the next four was under 2 ms everywhere but at 6x6, where
it was 1.5 ms before, and 9.1 ms with the stand-in and 4.5 ms with the library's
dice). So, as the faces were added, **a new board cost two to three and a half
times what it did**, and the frame in which it is drawn was over the
[33 ms budget](performance.md#budgets) at 6x6 and at 5x5 with the stand-in's
jewels, at the budget at 5x5 with the library's dice, and under it at 3x3 and
4x4 (the stand-in's means there, 28 and 26 ms, are close to it). `play()` sets
the board, so the first frame of a shake carries it. What made the stand-in
dearer than the library's board is the jewel: each dead die is six
`Shape`s of four facets, about 0.6 ms a cell by the difference at 4x4 (two dead
dice, 12 cells, 6.4 ms more; the boards' letters differ too). The next
subsection is what was done about it.

**Not measured:** the GPU's time for a frame with the bigger atlas, the atlas's
memory and upload on a device, and anything on an iPad, a tablet or a phone.

### The real faces, made cheaper

The frame in which a board changes is the cost above, and three things were
tried on it, in this order, keeping what measured better. Each is a commit of
the branch. The harness's `--change` mode is as it was, and it also records how
long the `show()` call itself held the GUI thread, which is the part of the
frame before the first frame is drawn.

| Step | What it is | Kept |
| --- | --- | --- |
| 1. The jewel once | The jewel is drawn into a texture of one cell, not live, and each of a dead die's six cells shows that texture (a `ShaderEffect`, whose own shaders show its `source`) instead of a `Shape` of four paths | Yes: the stand-in's frame 8 to 14 ms shorter, and nothing changes for the library's dice |
| 2. Prepare, and the atlas as a layer | `prepare(board)` draws a board's faces a frame or more before `play` or `show` of it ([the interface](#the-interface)). Making it work found the larger saving: an atlas that is only a `Texture`'s `sourceItem`, with no layer, seems to be drawn once for each `Texture`, and each die has one. The atlases are now layers, in the window's tree a long way off its left edge, so one texture is drawn and every die reads it | Yes: the render thread's time for the frame fell from 12 to 3 ms (3x3) and from 35 to 5 ms (6x6, library dice), and `prepare` then takes more than half off the shake's first frame (6x6: 31.2 to 13.3 ms) |
| 3. The cells kept | The atlas's cells (a cream face with its letter, or a jewel) are made as a board needs them and kept, and a smaller board hides the ones it does not use, where each board made them all. Each cell is two items, the face and its letter, with no `Loader`, and the places and letters of all of them are worked out in one pass | Yes: the `show()` call from 25 to 13 ms (6x6), and no cell is made for a board of a size that has been shown before |

**Tried and not kept.** The first form of `prepare` left Quick 3D drawing the
atlas, as it did, and put a cube a hundredth the size under the floor that used
the spare atlas's texture, so that it would be drawn a frame early. It was not:
the render thread's time was 4 ms for the frame of `prepare` at 6x6 and 43 ms
for the frame of `show` after it, against 39 ms for that frame without
`prepare` (the stand-in's board). Making the atlases layers (step 2) is what
makes them draw when they change.

**How it was measured.** The same harness (`ShakePrototype.exe --auto --change
--sizes 3,4,5,6`, with `--boards phonics` and `--boards standard`, and
`--prepare` for the frames of `prepare`), the same desktop and build flags as
the rest of this page, three processes of each build, six boards of each size in
each (18 of each cell). The builds were each copied to a name of their own and
run one after another in each round (before, step 1, step 2, step 3, and the
last two again with `--prepare`), so that anything that drifted touched all of
them. A run was thrown away and made again if another prototype was seen drawing,
or a build or ctest running, in any second of it (33 of 69 attempts: this desktop
is shared). "Before" is the previous subsection's "after" (`c5bb73b`), measured
again in this series: its means are 3 to 12 ms higher than in the table above
(at 6x6, library dice 61.8 ms against 57.7 ms and the stand-in 87.4 ms against
75.5 ms), which is the spread between series on this machine. **Nothing was measured on an iPad**, a tablet or a phone,
and not the GPU's time: these are times on a desktop with a fast GPU, and the
work that moved is mostly CPU.

Means, in ms, of the time from `show()` to the first frame on screen, with the
part of it that is the `show()` call (the GUI thread's) in brackets:

| Library dice | Before | 1. Jewel once | 2. One layer | 3. Cells kept (final) |
| --- | --- | --- | --- | --- |
| 3x3 | 27.9 (9.8) | 24.5 (9.6) | 13.5 (8.5) | **9.2 (5.4)** |
| 4x4 | 23.1 (10.7) | 21.8 (10.8) | 14.5 (10.7) | **9.7 (5.0)** |
| 5x5 | 36.8 (16.5) | 37.9 (17.4) | 21.4 (16.7) | **13.1 (9.1)** |
| 6x6 | 61.8 (24.9) | 63.9 (25.1) | 31.2 (24.7) | **19.0 (12.9)** |

| The stand-in (two dead dice) | Before | 1. Jewel once | 2. One layer | 3. Cells kept (final) |
| --- | --- | --- | --- | --- |
| 3x3 | 34.8 (10.1) | 27.0 (10.0) | 13.6 (9.0) | **9.9 (5.7)** |
| 4x4 | 33.9 (12.0) | 23.8 (11.1) | 15.6 (11.6) | **9.8 (5.3)** |
| 5x5 | 53.7 (17.9) | 42.4 (18.1) | 23.0 (18.0) | **12.9 (8.5)** |
| 6x6 | 87.4 (26.1) | 73.1 (27.3) | 33.0 (26.4) | **19.4 (13.8)** |

The render thread's own time for that frame (recording and submitting it, as
above), before and at the end: the library dice's 12.3, 9.1, 17.7 and 34.9 ms
become 2.6, 2.7, 3.2 and 5.0 ms at 3x3 to 6x6, and the stand-in's 19.4, 17.5,
31.2 and 57.1 ms become 2.7, 2.6, 3.4 and 4.7 ms. **Every size is now under the
[33 ms budget](performance.md#budgets) in its mean, and the stand-in costs what
the library's dice do** (the jewel's 7 to 26 ms is now 0 to 1).

**The first board of a size.** The cells are made the first time a board needs
them, so the first board at a size larger than any before it in the process
costs more than the rest. Means over the three processes for the final build
(the first board of each size is the first of the six, and the run goes 3, 4, 5,
6), library dice and then the stand-in:

| | 3x3 | 4x4 | 5x5 | 6x6 |
| --- | --- | --- | --- | --- |
| The first board of the size | 9.2, 8.9 ms | 9.6, 9.1 ms | 22.5, 20.6 ms | 35.5, 32.9 ms |
| The five that follow | 9.2, 10.1 ms | 9.7, 10.0 ms | 11.2, 11.4 ms | 15.8, 16.7 ms |

So a 6x6 board that is the first of its size is at the budget (33 to 36 ms),
and any later board of any size is well under it. The application could pay it
where one-time costs already go (the first board,
[above](#what-changed-in-the-seam)), by having the scene's first `show()` make a
6x6 board's worth of cells. That was not done here: it trades about 20 ms at
start-up for the same 20 ms the first time a larger board appears.

**With `prepare`.** Each board was given to `prepare()`, 300 ms went by, and
then it was shown (`show()` of the board made again, so that the scene knows it
by its faces). Means, in ms, of the frame of `prepare` (from the call to the
frame on screen, so including the call) and then the frame of `show`, final
build:

| | 3x3 | 4x4 | 5x5 | 6x6 |
| --- | --- | --- | --- | --- |
| Library dice: `prepare`, then `show` | 6.5, 8.3 | 6.8, 7.7 | 8.1, 9.1 | 10.3, 12.6 |
| The stand-in: `prepare`, then `show` | 8.3, 8.8 | 8.8, 7.7 | 9.5, 11.0 | 12.0, 15.4 |
| Without `prepare` (above): library dice, stand-in | 9.2, 9.9 | 9.7, 9.8 | 13.1, 12.9 | 19.0, 19.4 |

The first 6x6 board, which makes the cells, costs 19 to 23 ms in the frame of
`prepare` and 13 to 15 ms in the frame of `show` after it, so neither frame is
over the budget; without `prepare` it is one frame of 33 to 36 ms. What
`prepare` buys is the shake's first frame, by about a third at 6x6 (19.0 to
12.6 ms, library dice), for a second frame of 10 ms somewhere the application
chooses. It holds a second atlas (14 MB of texture at 6x6, at a pixel ratio of
1). On a display with a higher pixel ratio a layer is drawn at that ratio in each
direction by default, and what ratio Quick 3D's own layer used was not checked;
that, and the memory on a device, are not measured.

**What did not change.** The stills of the 4x4 and 6x6 boards, library dice and
stand-in (11 of each: the shake through the rattle, the lid, the drop and the
settle, and the board with its 2D tiles over it) are the same as those of the
branch before this (no channel differs by more than 1/255), and show every
letter upright and not mirrored, the digraphs and the Qu face, and the jewels.
The first frame after `show()`, grabbed before any other frame is drawn, equals
the settled frame (24 pairs, with and without `prepare`: no channel differs).
The poses, the replay checksums and the scene's seam are untouched: `prepare` is
an addition, and `play()` and `show()` mean what they did.

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

The fourth look, the author's, on seeing the prototype:
**every die shows the same letter on all six sides.** A real die has six
different letters, and a shake mixes the dice and the faces. Hence
[Real faces](#real-faces): the scene is given the dice, not only the board.

![The weighted shake, 4x4, straight down, the library's dice: ten times, then the settled board with the 2D tile layer over it](shake-prototype-filmstrip.png)

The library's roll of the 4x4 dice with seed 7 (`M D O O / C A R T / R N E U /
L D R P`). At 0 ms the dice sit in the box on other faces than the ones they
will land on, and the shake shows side faces (350 to 1000 ms); then each die
hops home, turning over in the air (1150 to 1450 ms), and lands on the rolled
face, the letter upright. The same with the stand-in board, whose two dead dice
are six jewels each (and whose decoy faces include `QU` and `TH`):

![The same with the stand-in board](shake-prototype-filmstrip-jewels.png)

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
- **Faults.** Dice are squares in the plan and turn over in quarter and half
  turns at a knock, so a tumble is steps and not a roll; the hop home is a
  clean arc and is the most scripted-looking part; two dice can cross for a
  moment in the air on the way home; the dead jewel is a cube with four facets
  painted on, which looks wrong from the side; and nothing is heard.
- **The faces.** Read from the stills by whoever built it: letters upright and
  not mirrored on every top face at rest; side faces read as letters during the
  tumble and the hop; a die turns over about a horizontal axis in a quarter turn
  at a time, so the turning is in steps. How it reads at 60 Hz, and to a child,
  is unjudged.

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
   fades; the die's `tw`, `tx`, `ty`, `tz` read for which side is up (it lands
   on the board's face) and, if the other faces are to be seen, as a letter that
   changes where the turn passes a quarter. Then a replay is the same, the
   landing order and the clicks are the same, and `digest()` is the same string
   in both.
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

The prototype suggested three changes to the seam before the application is
built on it:

- **Done: the scene is given the dice, not only the board.** `board` is
  `{ rows, columns, tiles, faces, up }`, from the library's
  `roll_with_placements`. The proposal was `play(board, faces, up, seed)`; the
  faces and the face that is up travel inside the one object because `show`,
  `seek`, `digest` and the 2D swap need them too. A board whose faces do not
  make its tiles is drawn as given and logged once.
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
- Power and heat, memory beyond what is noted, and a second desktop GPU. The
  face atlas is six times the pixels it was (below); its memory on a device,
  and its upload there, are not measured.
- **Whether the other faces read.** The stills show a die's side letters while
  it tumbles; nobody has watched it at 60 Hz, let alone at a child's eye height.
- A real accelerometer and gyroscope. The scene takes samples; what has been
  fed to it is the seeded stand-in and a mouse (a pointer path differentiated
  twice), and a person's hand on the drag is unjudged ([above](#the-drag)).
- **Sound**, the accelerometer and haptics. The scene emits `rattled` and
  `landed` and the harness shows a count; nothing plays them, and no run
  checked the counts against the script.
- Whether the tumble reads at a child's eye height. See
  [Unknowns](board.md#unknowns).
