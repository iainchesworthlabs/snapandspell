# Application design

The screens and how they connect, the sizes of things a child touches, type,
the interface's languages and right-to-left layouts, colour, accessibility,
sound and touch feedback, how the privacy rules of the
MVP are built in and checked, and what the application needs from Qt and the
operating system. Written from the
[product requirements](../product-requirements.md) sections 2, 7, 10 and 12.
Proposals unless marked **Fact**.

The privacy architecture (section 12.2) is frozen before visuals or layouts:
**no audio recording, no network capability, no export, no timers or limits**.
No screen on this page needs any of them.

## Platform and scope

**MVP (question 64, assumed):** an iPad, as a private ad-hoc build for the
author's own devices; the platform list of section 2 is the product's scope.

| | iPad (MVP) | Android tablet | Phone | Web (WebAssembly) | Desktop |
| --- | --- | --- | --- | --- | --- |
| Shake by the device | Accelerometer | Accelerometer | Accelerometer | Not assumed (see below) | No: a button, Space or a drag |
| Haptic feedback | **Probably none**: see [Audio and touch](#audio-and-touch-feedback) | Yes | Yes | No | No |
| Camera (Mode A) | Yes | Yes | Yes | If the browser has one and allows it | If one exists |
| Speech for whole words | The platform's own | The platform's own | Same | Not known | Platform's own |
| Back | On-screen control; swipe from the leading edge | System back | | Browser back | Esc |
| Orientation | Both | Both | Both | Follows the window | |

- **Shake in the browser.** A page can read motion only with a permission on
  some systems; Qt Sensors on WebAssembly is **unverified**. The browser build
  uses the **Shake** button.
- **Speech in the browser** is **unverified** (a search of Qt's TextToSpeech
  engine list found none for WebAssembly), so words there are pre-rendered
  clips or silent.
- A camera is a runtime condition, not a platform: Cubeulator's browser build
  reads one with `getUserMedia`. See [Game design](game.md#no-camera).

### Qt modules

**As built, checked in the shared Qt 6.12.0 tree:** the Windows kit has Core,
Gui, Qml, Quick, QuickControls2, Quick3D, ShaderTools, Svg, **Multimedia,
Sensors and TextToSpeech** for the feedback engines; the module archives for the
last three were fetched from the Qt repository and merged into the kit
(docs/building.md, "The Qt Quick window"). There is still no iOS kit. The
application finds the three **optionally**: a contributor's kit without them
builds the silent variants of the sound, speech and motion backends
(`src/app/feedback`), and the game is complete, only quieter.

| Module | For | In the tree | Offered under |
| --- | --- | --- | --- |
| Quick, Controls 2 | The UI | Yes | Commercial, LGPL-3.0, GPL-2.0 or GPL-3.0 (**checked**, SBOM) |
| QuickShapes, QuickParticles | Paths, confetti | Yes | The same (**checked**) |
| Quick3D, ShaderTools | The 3D board | Yes | Quick3D: Commercial or GPL-3.0-only (**checked**) |
| Multimedia | The short feedback assets; the camera on some targets | **Yes**, optional | Commercial, LGPL-3.0, GPL-2.0 or GPL-3.0 (**checked**, the kit's `sbom/qtmultimedia-6.12.0.spdx`) |
| Sensors | The accelerometer and gyroscope | **Yes**, optional | The same (**checked**, `qtsensors`) |
| TextToSpeech | Whole words and sentences, on the platform's engine | **Yes**, optional | The same (**checked**, `qtspeech`) |

The rule in [Licensing](../licensing.md#qt) holds: check how Qt offers a module
before adding it. All three are offered under the LGPL-3.0 as well as the GPL
and commercial options, so none changes the application binary's licence; it is
GPL-3.0-only because of Quick 3D, as before.

## Screens and navigation

```text
                      ┌──────── home ────────┐
                      │                      │
        ┌─────────────┴────┐          ┌──────┴───────────┐
        │  Mode B: solo    │          │ Mode A: scanner  │       parent gate
        │  solo.box        │          │ scan.ready       │            │
        │   ▼ shake        │          │   ▼ capture      │       ┌────┴────────┐
        │  solo.playing    │          │ scan.check       │       │ dashboard   │
        │   ▼ cleared      │          │   ▼              │       │ settings    │
        │  solo.resting    │          │ assist.playing   │       │ about       │
        └──────────────────┘          └──────────────────┘       └─────────────┘
              vault, history: from the panel or from home
```

| id | Screen | Notes |
| --- | --- | --- |
| `home` | Two large pictures: the box (Solo Play) and the tablet on a stand (the family game). A small corner button for the parent | A picture speaks its name when pressed and acts on release inside it; slide off to cancel |
| `solo.*` | Mode B | [Game design](game.md#mode-b-solo-play) |
| `scan.*`, `assist.*` | Mode A | [Game design](game.md#mode-a-the-family-assistant-and-scanner) |
| `vault` | The album | From home; never offered after a board |
| `history` | Earlier boards | From the panel or from home |
| `parent.gate` | The gate | [Dashboard](dashboard.md#the-gate) |
| `parent.*` | Dashboard, settings, about, licences, privacy | [Dashboard](dashboard.md) |

Rules that hold everywhere, taken from Cubeulator's journey:

1. **A way back and a way on.** Every state has a visible control that goes
   back. System back does the same and is never the only way.
2. **Back goes where the person came from**, with two exceptions: from
   `solo.resting` it goes to `home`, so leaving does not show the shake button;
   from `solo.playing` it puts the lid on (`solo.box`) and saves the board.
3. **Nothing is lost.** Leaving never asks "are you sure"; the board is saved
   to history, as Cubeulator's solve is kept. The only things that ask are in
   the parent area (clear history, reset).
4. **The app starts without a camera.** No state needs one except
   `scan.ready`'s capture.
5. **No first-run carousel.** Home is the first screen. There is no text to
   read; the pictures are the instruction.

## Touch targets

| Token | Size | Where |
| --- | --- | --- |
| `target.min` | 48 x 48 dp | Parent controls and anything an adult touches. Cubeulator's floor everywhere |
| `target.child` | 64 x 64 dp | Anything a child taps that is not primary: the size choice, the corner buttons, a jewel in the tray |
| `target.primary` | 96 dp high, at least a third of the width | **Shake**, **Capture**, **Yes, play**, **Show me a start** |
| Board tile | 48 dp is the floor; 56 or more is comfortable | See [Board design](board.md#which-sizes-are-offered) |

Rules, from Cubeulator's system: targets that touch share an edge and never
overlap; a target smaller than its visual is a defect; a visual smaller than its
target is normal. Neighbouring child targets are at least 8 dp apart (a
proposal).

**Why larger than Cubeulator's 48, 56 and 96.** A search for a published
minimum for five- and six-year-olds on tablets found none. What it did find is
that children miss targets more often than adults and that the youngest in the
studies (seven to ten) miss the most (Anthony and colleagues, 2013, and Brown and
Anthony, 2012). So 64 dp for a child's tap is a **starting value** for a test
with real children, not a finding.

## Input

- **Trace and tap in order** are both always on (see
  [Game design](game.md#tracing-and-tapping)); no gesture is required
  (WCAG 2.5.1). Only the first touch draws.
- **Keyboard and switch.** Arrow keys move focus across tiles, Space adds the
  focused tile, Enter submits, Escape clears. A switch device that sends those
  keys can play.
- **Screen readers.** The board is a grid; each tile is named by its letter and
  its place ("A, row 2, column 3"); the panel's words are a list; a found word
  is announced politely ("pain, found"). The 3D scene is opaque, so the tiles'
  accessible items are a 2D layer over it, as Cubeulator's instruction block is
  the alternative to its cube.

## Type

**The requirement (7.1):** large, child-friendly sans-serif type in which upper
and lower case are very clear and "I", "l" and "1" are not confused. Question
18 asks that this be checked by looking at the glyphs.

| Face | Licence | What is known | What is not |
| --- | --- | --- | --- |
| **Andika** (SIL) | SIL Open Font Licence (**read** from several secondary sources, not from the licence file) | Designed for beginning readers; single-storey a and g by default; its design notes list "capital I, lowercase l and numeral 1 that don't look alike" among its aims (**read**) | Its glyphs at this app's sizes: **not looked at**, the face is not installed here; its file size |
| **OpenDyslexic** | OFL; Wikipedia gives version 1.1; the project says only the newest release moved to it, so older releases may differ (**read**) | Named in the brief | The same, and its design aims for the "I, l, 1" shapes were not checked |

**Recommendation.** Andika for the dice, the panel and every label, because it
was designed for beginning readers and for these distinctions; **a
side-by-side with a child decides it**, and both are named in the brief. Each
face's licence file must be read before it is bundled. Bundle it on every
platform: a browser has no system fonts (Cubeulator's web notes), and a
tablet cannot be assumed to have it.

**The glyph check (question 18), as a procedure** because it cannot be done
from a description:

1. Set `I l 1 O 0 Q 6 9 b d p q M W` in each candidate at the sizes in use:
   the letter on a 56, 108 and 245 dp tile (about 31, 59 and 135 dp), and the
   lower-case words in the tray at 24 dp, on a real iPad at its pixel ratio.
2. Set the real alphabet as the dice show it, upper case, upright.
3. Set the same on the replica of a physical board, since the check screen is
   compared with real dice.
4. Record which pairs a person in the target age band can tell apart.
5. Record the result. A specimen sheet has no child in it and can be
   committed; a recording of a child's attempts cannot be (see
   [Licensing](../licensing.md#camera-input-and-letter-recognition)).

**Case.** Upper case on the dice (the real board is upper case, and the
replica must match it), lower case for words in the tray and vault (as in
books), no sentences in capitals.

### School fonts (7.1; questions 38 to 40)

- **Which belongs to which state.** The brief names Vic Modern Cursive for
  Victoria, WA and the NT and the NSW Foundation font for NSW; the others are
  unconfirmed. **Read:** a site of Australian school fonts has a NSW/ACT page
  and a VIC/WA/NT page, and a font-manager listing has a family named *Edu NSW
  ACT Foundation*; families for the other states were not checked.
- **May each be bundled?** **Not established.** A vendor describes the NSW
  fonts as copyright of the NSW Department of Education and sold under a
  per-computer licence; a packaging entry gives the Victorian font as CC-BY-4.0
  (**unofficial**; confirm on the Department's site). The *Edu* families are
  separate designs that imitate the school styles and are under the OFL
  (**read**; the font files' own licences are the evidence, not checked). So
  the setting can offer **only fonts whose licence file permits bundling**; the
  official ones are not bundled until a written permission or a clear licence is
  found. The roadmap already keeps the setting out of M1's exit until then.
- **What the setting changes (39).** The words a child reads in lower case: the
  tray, the vault, captions. **Not** the dice, which stay print capitals in the
  literacy face: a joined face on a die harms legibility and does not match the
  physical board.
- **Which requirement wins (40).** Clarity. A school face is offered only where
  it passes the same glyph check as above for the places it applies.

## Languages, right-to-left and fonts

**As built:** the window is offered in **English, French, Spanish, German,
Yiddish, Hebrew, Chinese and Japanese** (`en`, `fr`, `es`, `de`, `yi`, `he`,
`zh`, `ja`). The Language row in the parent area offers those eight and
**System**, which is the device's own language when it is one of them and
English otherwise. The setting is kept in the store with the others, so it
survives a restart, and the language is in force before the window is built:
`LanguageManager` (`src/app/language_manager.*`) installs the catalogue and sets
`QGuiApplication::setLayoutDirection` before the engine loads. A change in the
parent area installs the new catalogue, turns the layout and calls
`QQmlEngine::retranslate()`, so every `qsTr()` sentence on screen changes at
once without a restart.

**The catalogues.** One Qt Linguist `.ts` file per language is in `i18n/`,
generated from the QML sources with the kit's `lupdate`
(`cmake --build <build> --target wjs_update_translations`), and `lrelease`
compiles them at build time into `:/i18n/wjs_<code>.qm`, embedded in the
application (docs/building.md, "The interface's languages"). English is the
source language and has a catalogue too, so the eight are checked alike. Every
user-facing string is `qsTr()`; the sentences of an area live in that area's
wording file (`ParentWording.qml`, `ScanWording.qml`, `SoloWording.qml`,
`HomeWording.qml`), and only the shell's window title is inline. `qWarning()`
and error text a developer reads stay English.

**What is translated.** The chrome: every label, sentence, accessible name and
message. **Not** the board: the dice letters, the words in the tray, the ENABLE
word list and the phonics content are English, because the game is an English
word game (audit-0.1.md, "Languages, RTL and i18n"). The translations are
**mechanical and unreviewed**; they are listed with the phonics content for the
literacy/reviewer pass, which decides the wording a child and a parent see.

**Right to left.** Hebrew and Yiddish read right to left. The window sets
`LayoutMirroring.enabled` with `childrenInherit` from the language, and the
screens that need the direction for something other than anchors read
`Theme.rightToLeft`/`ParentTheme.rightToLeft`: the navigation bars put the way
back on the leading edge, the parent area's rows and choices order from it, the
gate's words and two symmetric spots follow it, and the two-thirds/one-third
split puts the panel on the trailing side (the left) and the board on the
leading side (the right). The **board never mirrors**: the dice, the tiles, the
trace path and the 2D and 3D scenes keep their reading order and their
coordinates in every language (`LayoutMirroring.enabled: false` on the board
containers). Numbers and dates use the locale.

**Fonts.** Andika (SIL OFL) is bundled and is the face for beginning readers,
the dice and the reading text; Archivo carries the interface's headings, buttons
and labels, and IBM Plex Mono the small uppercase labels. All three are
compiled into the application's resources. Hebrew, Yiddish and CJK still fall
back per glyph through the platform's fonts: Segoe UI/Arial and Microsoft YaHei
and Yu Gothic on Windows, PingFang and Hiragino on the iPad. CJK faces are
**not bundled** (about 16 MB); the system fallback is documented and accepted,
and no CJK text is ever drawn into a picture. If a device's chain turned out to
lack a Hebrew or Yiddish glyph, Noto Sans Hebrew (OFL, also not bundled) would
be added to the asset directory, the resource and the font family list, and
recorded in [Licensing](../licensing.md). A test lays out a sample
sentence in each of the eight languages with the app's font chain and skips
itself, with a message, on a platform whose fonts lack the script; it is a
Windows check today (docs/testing.md). **Recorded:** run on Windows on
2026-10-19, the chain carried every glyph of all eight samples (Latin with
accents, Hebrew, Yiddish, Chinese and Japanese).

## Colour and contrast

The brief gives no palette. These are **placeholders** chosen to be measured,
not designs; the numbers are WCAG 2.x contrast ratios computed from the
values, and are the gate a final palette has to meet. Cubeulator holds its
tokens to the same gate with a script (`docs/design/tools/check_tokens.py`).
The board is dark in both themes, so the dice read the same; the chrome around
it follows the **Colour scheme** setting: System (the default) follows the
operating system's light or dark setting, and Light or Dark is the parent's
choice for the whole program (the house standard of the sibling projects,
decided 2026-10-18).

| Pair | Values | Ratio | Target |
| --- | --- | --- | --- |
| Letter ink on a die | `#1F2A37` on `#FFF8E1` | 13.68 | 7:1 (the letters are the content) |
| Die against the tray | `#FFF8E1` on `#1F2A37` | 13.68 | 3:1 |
| A lit die against the tray | `#FFD54F` on `#1F2A37` | 10.30 | 3:1 |
| Ink on a lit die | `#1F2A37` on `#FFD54F` | 10.30 | 4.5:1 |
| **A lit die against a resting die** | `#FFD54F` on `#FFF8E1` | **1.33** | not met: so lit is shape, not colour |
| Path casing on the tray | `#FFFFFF` on `#1F2A37` | 14.54 | 3:1 |
| Path core on a die | `#1F2A37` on `#FFF8E1` | 13.68 | 3:1 |
| Path core on a lit die | `#1F2A37` on `#FFD54F` | 10.30 | 3:1 |
| Body text on the page | `#1F2A37` on `#FBF8F1` | 13.71 | 4.5:1 |
| Secondary text | `#4A5666` on `#FBF8F1` | 7.04 | 4.5:1 |
| Button label on the button | `#FFFFFF` on `#1F5FA8` | 6.44 | 4.5:1 |
| The button on the page | `#1F5FA8` on `#FBF8F1` | 6.07 | 3:1 |

The path is therefore two-tone: a pale casing seen against the tray and a dark
core seen against the dice. And:

- **State is never colour alone.** Lit is a thicker edge and a scale; a grapheme
  link is a bar and a shared edge; the start tile is a pulsing ring; a doubtful
  tile is a ring and a `?`; a dead jewel has a shape. This is Cubeulator's rule.
- **A jewel's colour is decoration.** Its size says its tier; nothing is "the
  red one". A world (7.2) swaps the board's look and background tokens and no
  rule.
- **Dark theme values** are in `src/app/qml/kit/Theme.qml`, with the light ones, and
  were computed against the same gate: the page `#12161C` and ink `#F4EFE3`
  (15.82), secondary ink `#B4BDC9` (9.56), a control's edge `#8C98A8` (6.20 on the
  page, 4.49 on a card, 5.16 on the panel), the button `#7DB4EE` under `#0E1620`
  (8.35, and 8.33 on the page). The board is the same in both, so its edge, which
  the dark tray would otherwise lose against a dark page, is `#8C98A8` (5.66 on
  the board area). The parent area's tokens (`ParentTheme.qml`) are the same
  pairs under its own names, so the ratios carry over. Nothing here has been
  looked at on a device.

## Motion

Cubeulator's motion tokens are reused: 120 ms for quick feedback, 200 ms for a
panel, 280 ms (120 reduced) for a screen, 380 ms for the board's own move. The
brief's strong effects are limited, because they are strong for a young child
(question 25).

| Effect | Full | Reduced |
| --- | --- | --- |
| Shake of the box, lid, drop | [Board design](board.md#the-shake-and-the-drop) | A still box, a cross-fade |
| Spark and Power effect | A sparkle or a trail on the word | The word and its jewel appear |
| **Mega Word screen shake** | **Not shown for anyone**; the jewel bounces once | The jewel appears |
| **Screen-wide confetti** | Confined to the board and the panel; at most 3 flashes a second | Stars fade in; no particles |
| Voiceover | Spoken at the device's volume | Spoken; a parent can set celebration sounds to quiet or off |
| Resting screen | Fades in over 600 ms, then still | A cut |
| Anything that loops | Never for more than 5 seconds (WCAG 2.2.2) | Nothing loops |

**Who sets it.** The system's reduced-motion setting is the default and the
parent area can override it (Motion: System, Reduced). It is a parent's
decision, not a child's, and it is not behind the game.

**Motion as input, as built.** `MotionSource` (`src/app/feedback`, over Qt
Sensors) reads the accelerometer and the gyroscope at the highest rate the
platform lists, feeds the samples to the scene's `pushMotion(ax, ay, az, wx,
wy, wz)` and to the pure `wjs::MotionProcessor`, and reports a real shake once
per shake; the box then rolls as the Shake button does. It runs only while Solo
Play's box or the falling dice are on screen, only while Shake is on, and stops
when the screen is left. On the three desktops there is no accelerometer: the
source stays unavailable and the drag that stands in for one remains
(`SoloScreen.qml`). The processing is tested on synthetic traces
(docs/testing.md), not on hardware: the detector's starting values still have
to be tuned on a tablet.

**The shake and the drop are the 3D board scene**
([Board](board.md#the-shake-and-the-drop)). With reduced motion the scene keeps
the box still, cross-fades the lid and fades the dice in on the faces the roll
left up; nothing tumbles, and the board appears and plays as it does otherwise.
The scene draws no particles of its own; the single slow glow when the board
clears is the 2D layer's and is switched off with reduced motion too.

## Accessibility

| WCAG 2.2 criterion | How it is met |
| --- | --- |
| 1.1.1 Non-text content | Tiles, jewels, the start tile and stickers have names; the 3D scene's alternative is the 2D tile layer |
| 1.4.1 Use of colour, 1.4.11 Non-text contrast | The table above |
| 1.4.4 Resize text | Text scale 1, 1.25 and 1.5 times the system's, clamped to 0.85 to 2.0; layouts hold at 2.0 (tray scrolls, buttons grow with their labels). The system's size is not read yet: Qt 6.12 does not expose it ([parent area](parent-area.md#integration)) |
| 2.2.1 Timing, 2.2.2 Pause, stop, hide | There are no timers; nothing loops over 5 seconds |
| 2.3.3 Animation from interactions | Reduced motion (above) |
| 2.5.1 Pointer gestures | Tap in order is always on |
| 2.5.4 Motion actuation | The **Shake** button, and shake detection can be switched off |
| 2.5.8 Target size | 48 / 64 / 96 dp |
| 4.1.2 Name, role, value | Every control has a name and a role |
| Photosensitivity | At most three flashes a second; no full-screen flash |
| Colour vision | State is never colour alone (above) |
| Hearing | Everything that is announced by sound is also seen: a jewel, a lit block |

Cubeulator's own accessibility paper lists what it does **not** know ("Whether
`Accessible` on custom Qt Quick items is read well by TalkBack and VoiceOver on
phones"). The same holds here and is the first thing to try on the iPad.

## Audio and touch feedback

All sound is **output**. Nothing is captured from the child (section 12.2).

| Event | Sound | Touch feedback |
| --- | --- | --- |
| Shake | A rattle, looped, then a settle | Light taps, 8 a second at most |
| Each die lands | A soft click, pitched a little differently | None |
| A tile joins the path | Its letter's sound, 300 ms or less | A very light tick |
| A viable word | The sound-it-out sequence, the word, a cheer | One medium tap |
| A real word, not today's | A soft tick | None |
| Not a word | Nothing | Nothing |
| A new best | One chime | One tap |
| Board cleared | A quiet three notes | A soft double tap |
| Resting | Nothing | Nothing |

An error is never scolded (Cubeulator: "An error is not scolded"). Sounds are
under 300 ms except the sequence and the rattle. No music.

**As built.** The seam is still `Feedback.cue(kind, detail)` in the QML
(`src/app/qml/kit/Feedback.qml`); Main.qml installs one `FeedbackEngine` on it
(`src/app/feedback`), and the engine asks the pure policy
`wjs::plan_cue` (`src/lib/include/wjs/feedback.hpp`) what the cue and the
parent's settings come to. The policy is tested without a speaker: a cue, a
settings value and a detail in, and the asset, its volume, whether the word is
spoken and which tap of the hand is asked for.

| Cue | Asset (`assets/sounds`) | Spoken | Haptic |
| --- | --- | --- | --- |
| `rattle` | `rattle` | - | light |
| `tile` | `letter` | - | light |
| `tile-landed` | `land` | - | none |
| `word` | `cheer` (Celebration sounds) | the word (Sound it out) | medium |
| `tick` | `tick` | - | none |
| `chime` | `chime` (Celebration sounds) | - | medium |
| `cleared` | `cleared` (Celebration sounds) | - | double tap |
| `speak` | - | the name or word | none |
| `tap` | `tap` | - | none |

The rules are the parent's: **Sound** off silences every asset and every spoken
word; **Celebration sounds** off drops the cheer, the chime and the cleared
notes (the word is still spoken if Sound it out says so) and **quiet** plays
them at half volume; **Sound it out** full and word both say the word, and off
does not; **Vibration** off drops every haptic and changes no sound. Nothing is
played for "not a word".

- **The sound engine** is Qt Multimedia's `QSoundEffect` over the eight
  placeholder WAVs, one effect per asset. A device with no audio output, or its
  silent switch on, plays nothing and the game carries on; the engine counts
  requests, not sounds heard. The scene's `tile-landed` cue carries a pitch for
  its click; the placeholders are one soft click and ignore it until the
  recorded set exists.
- **The voice** is the device's own Qt TextToSpeech engine, whole words and
  sentences only. It is output only: there is no audio input in the engine or
  anywhere else in the application (section 12.2). `say_letter_sequence()` is
  the seam for the sound-it-out sequence and is deliberately silent: the
  isolated phonemes are the reviewer's recording job, and the platform's voice
  must not substitute letter names for them.
- **The assets** are placeholders made by `tools/sounds/make_sounds.ps1` and
  owned by the project (`assets/sounds/README.md`, notices). The real phoneme
  clips and recorded cheers replace them after the literacy review; the file
  names are the seam. The three Qt modules are optional at build time: their
  silent variants are chosen by CMake, so a kit without them still builds.
- **Touch feedback.** `src/platforms/interfaces/wjs/platform/haptics.hpp` is the
  platform interface (`light`, `medium`, `double_tap`). All three desktops
  answer false through `detail/haptics_none.cpp`, and the game is unchanged:
  every haptic is paired with a sound and a picture. The Android
  (`Vibrator`/`VibratorManager` through JNI) and iOS (`UIImpactFeedbackGenerator`)
  implementations wait for those targets. Whether an iPad has a haptic engine
  is still **from general knowledge, not checked**; the first device test
  settles it.

## Privacy, built in and checked

Section 12.2 (frozen) and question 65 ask how each rule is kept true and
checked on each target.

| Rule | In the build | Checked by |
| --- | --- | --- |
| **No audio recording** | No microphone usage string in the iOS property list; no audio input in the camera session, which is video only; no `QAudioSource` or audio capture in first-party code; the sound and speech engines only play and say | A scan of first-party code for the audio-capture classes, in the style of `check_platform_isolation.ps1` (**built for the scanner and its camera files:** `scan.privacy`, [Scanner](scanner.md#privacy-built-in-and-checked), which also scans them for network classes and for any file output, so a frame is never written); a check on the built property list |
| **No network capability** | Android: no `INTERNET` permission in the merged manifest. All targets: first-party code does not use the network classes; the QML engine is given a network manager that refuses everything but local and resource URLs. Web: the page makes no request beyond its own files (a content-security policy of `connect-src 'self'`) | On Android, a dump of the built package's permissions (it must not list `INTERNET`; whether Qt's Android build adds it for a module is **unverified**, which is why it is checked on the built package); a scan for the network classes; a test that loads a remote URL in QML and fails; on the web, a header check. **Qt Network cannot be left unlinked**: Cubeulator's licensing paper notes that Qml and Quick depend on it, so the rule is "no capability and no use", not "no module" |
| **Zero export** | No share sheet, print, file-save or mail code; no URL scheme handler | A scan for the share, print and file-dialog classes. **Backups are an open question**: an iOS app's data is included in a device backup by default (**general knowledge, not checked**), which is data leaving the app under the user's own account; whether to exclude it is the author's decision (question 50) |
| **Local store** | A small versioned file, written atomically, in the app's private directory, with only what the dashboard needs (below). SQLite through Qt SQL is possible but the data is a few kilobytes and has no query that needs it | A test that round-trips and migrates a version |
| **No profiling in what ships** | Tracy listens on a TCP port when built in (Cubeulator's profiling notes) | The shipped presets must not enable it; a check that a release artefact has no Tracy symbols |

**What the store holds**, which is also what the dashboard may say: for each
find, the word, the board size, the day it was found, whether the start
tile was used for it, and the phase; the stars; the best; the stickers; the
last twenty boards. **It does not hold** a duration of play, an identity, a
location, an image or a sound. (An earlier draft of this page said the time of
day; the build keeps the day only, because a time on every find would be
a figure for time on screen, which [Game](game.md#what-this-design-does-not-do)
rules out. See [The parent area as built](parent-area.md#the-store).)

**What is checked now.** The scans for audio capture, the network classes and
modules, and the export routes, the store's round trip, migration, damage and size
cap, and the parent area's wording are tests that run in every CI leg
(`privacy.sources`, the `[store]` cases, `parent.wording`; the table in
[The parent area as built](parent-area.md#privacy-checks-ci)). The checks on a built
Android package, an iOS property list and a web page wait for those targets.

## Unknowns

- Whether the 3D board runs well on the author's iPad, and which iPad it is.
- Whether VoiceOver reads the 2D tile layer over a 3D scene well.
- Whether an iPad can give any haptic feedback (see above).
- Whether the tablet's Qt Sensors backend reports an accelerometer and
  gyroscope the way the scene expects, and the detector's starting values on
  it: nothing here has been run on a tablet, and the desktops have no sensors.
- The size of Andika and its glyphs at these sizes.
- The licences of the school fonts (the three Qt modules are checked).
- **The recordings**: the phoneme clips and the sound-it-out wording are the
  reviewer's, and the placeholder tones are not them.
- **The translations**: all eight catalogues are mechanical and unreviewed. The
  reviewer's decisions are the wording a child and a parent read, what each
  language calls the sounds and the tiers, and whether a translated UI over an
  English board is enough for a family whose language is not English.
- **The fonts on a device**: the glyph check is a Windows check today. Whether
  the iPad's fallback covers every Hebrew and Yiddish sentence, and whether the
  Chinese and Japanese sentences read well at the sizes in use, is looked at on
  a device, not here.
