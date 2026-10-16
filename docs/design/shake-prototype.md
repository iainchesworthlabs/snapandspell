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
| `agitation` | in | 0 to 1, how hard the device is being shaken now. Holds the rattle on past 0.7 s, up to 3 s |
| `tilt` | in | A vector, each axis -1 to 1. Leans the box within the 8 degrees |
| `settled()` | signal | Once per play, when the dice have stopped |
| `landed(index, pitch)` | signal | A die has hit the floor: its place in the board, and a pitch for its click |
| `rattled()` | signal | The box rattles, at most 8 a second, for the haptic tap |
| `busy`, `rows`, `columns`, `tileSize`, `tileGap`, `boardRect` | read-only | The geometry by [the rule](board.md#the-rule), for the 2D layer that traces over the scene |
| `overdraw` | read-only | How far past its item the scene draws, on every side (half a tile), so a die tumbling at the edge and the tipping rim are not cut off. The item is still the board's area; the application must not `clip` it |

`lean`, `probeMode`, `cameraDistance`, `warmUp` and `weighted` are settings of
the 3D implementation and are not part of the seam. `seek(board, seed, ms)` and
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
the device shakes, up to 3 s; the lid; the dice slot in, in the seeded order)
and the motion between them integrated, at a fixed 240 steps a second, from
forces:

- The box is accelerated (sideways and up and down) and the dice in it are
  pushed the other way. The vertical push is more than gravity, so dice leave
  the floor and hit the lid; the live `agitation` scales it.
- A die bounces off the floor (about a third of its speed back), the lid and
  the walls of a cavity a fifth of a tile roomier than the board, and off the
  dice next to it. It spins by a seeded amount, more the harder it hit, and
  flops flat on the floor within a tenth of a second.
- Once the lid is off, a spring funnels each die into its cell, the first dice
  of the seeded order first (30 ms apart), and the yaw springs back to upright.
  Everything is at rest when the springs and bounces have died away, with a
  hard limit 0.7 s after the last die began, and then it is exactly on its cell.
- Time is slowed to about a third of a real die's, so that a fall of a tile
  takes a fifth of a second and can be seen. The weight is in the shape of the
  motion: the acceleration, the stop, the rebound, the spin.

It is still seeded and replays: the same board, seed and (still) device give
the same checksum, because the integrator is a fixed step with a generator for
each die. It is not a physics engine and does not decide where a die lands;
that is its cell, always.

| Play (weighted) | Measured, play to `settled` | What it is |
| --- | --- | --- |
| 4x4, full motion | 1.7 to 1.8 s | 0.70 shake, 0.25 lid, the rest slotting in: 1.1 s after the shake, 1.5 s with the 380 ms layout stage that is the application's, not the scene's |
| 3x3, full motion | 1.9 s | The same |
| Either, reduced motion | 0.58 s | 0.30 rattle, 0.12 lid fade, 0.12 dice fade |
| Rattle held at full | 4.1 to 4.2 s | The shake held to its 3 s limit, then the lid and the slot |

(Staged: 1.97 s at 4x4 and 1.73 s at 3x3, with a 0.87 s drop.)

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
| Weighted | 3x3 | `f3f4f8ac`, `f3f4f8ac` | `3763cac3` | `fecee3b5`, `fecee3b5` |
| Weighted | 4x4 | `4bba074`, `4bba074` | `bb5ba6` | `6a5f4a5d`, `6a5f4a5d` |

A checksum is of the poses in pixels, so it is for one scene size: the staged
rows are from the first window and the weighted from the later one (which is
why the reduced rows, the same script in both, differ between them).

Moving the script out of the scene file into `ShakeScript.js` left the stills
identical, pixel for pixel but one pixel one level off. What the checksum does
not cover is the rattle's response to a live `agitation`, which is the one live
part by design.

### The drag

A mouse drag stands in for the accelerometer: where the pointer is in the scene
sets `tilt`, how fast it moves sets `agitation`, and the agitation decays when
it stops. One measured play sent the window a scripted back-and-forth pointer
path for two seconds (`dragBetween`, so the pointer path through the scene is
exercised without a hand): the peak agitation reached 1.0 and the tilt 0.7, the
box rattled for 2.3 s instead of 0.7 s (3.6 s play against 1.97 s), and no frame
was over 20 ms. The same hold with the agitation set directly is the "rattle
held" rows above. The first version read the speed from the handler's own
velocity, which a scripted event does not carry (peak 0.19); it is now computed
from where the pointer was and when.

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

Direct3D 11, four plays of each case after the first, the baked probe: 2,862
frames, the longest interval 20.9 ms, and the render thread's mean is 0.4 to
0.7 ms a frame, the same as staged. The integrator (4 steps a frame, 16 dice,
120 pair tests a step) does not show in the frame times. A shake driven
through the pointer was 3.4 s in all. New checksums for replay, for this script,
are recorded in [Replay](#replay) and agree between runs.

## What it looked like

The author's first look at the staged version, running it: **it had no weight.**
Everything moved on curves, the dice did not collide or slide into their cells
the way a real set does (curved corners, a tap to seat them), and there was no
sense of mass. Looking straight down was right for how the board is held and
shaken, and 3x3 was fine. Hence the weighted script, and top-down as the
default (`lean` 0).

![The weighted shake, 4x4, straight down: ten times, then the settled board with the 2D tile layer over it](shake-prototype-filmstrip.png)

Stills at fixed times of one board (`--auto --shots`), so they are the same on
every run. They were read from stills by whoever built the scene, not by a
child, and nobody has yet watched the weighted motion: that is the author's to
judge by running it. **Nothing plays `landed`, and sound is much of what makes
a die feel heavy:** Qt Multimedia is not in the shared tree, so the prototype
is silent, and the feel is judged without it.

- **Weighted, straight down (above).** Dice slide, turn and bump (350 to
  1000 ms) in a box with a visible rim, then slot into their cells in a seeded
  order with the last stragglers turning upright (1150 to 1650 ms). Height is
  shown only by size and shadow, because the camera is straight down.
- **Faults.** Dice are discs to each other and squares to the eye, so corners
  cross for a moment when two are turned; the dead jewel is a cube with four
  facets painted on, which looks wrong from the side; and the rebound is a
  fixed fraction, so every die lands about alike.

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
5. **The same inputs, the same meaning.** `agitation` and `tilt` change how
   hard and how long the box rattles and never where a die lands;
   `reducedMotion` takes the cross-fading path of every stage, 0.54 s in all.

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
- A person's hand on the drag. The runs set the agitation directly, and one
  sends the window a scripted back-and-forth pointer path ([above](#the-drag));
  how the drag *feels* is unjudged.
- **Sound**, the accelerometer and haptics. The scene emits `rattled` and
  `landed` and the harness shows a count; nothing plays them, and no run
  checked the counts against the script.
- Whether the tumble reads at a child's eye height. See
  [Unknowns](board.md#unknowns).
