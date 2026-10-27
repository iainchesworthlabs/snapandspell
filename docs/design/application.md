# Application design

The screens and how they connect, the sizes of things a child touches, type,
the interface's languages and right-to-left layouts, the Jewel Box design
system, colour, accessibility, sound and touch feedback, how the privacy rules
of the MVP are built in and checked, and what the application needs from Qt and
the operating system. Written from the
[product requirements](../product-requirements.md) sections 2, 7, 10 and 12,
and kept in line with the code at 0.1: where a sentence says **As built** it
describes what the application does; where it says **Not built** or **Open** it
does not. The rest is design intent.

The privacy architecture (section 12.2) is frozen before visuals or layouts:
**no audio recording, no network capability, no export**, and, until
2026-10-21, **no timers or limits**. The last rule is amended
([Decisions](#decisions)): a grown-up can switch on a clock for the board, off by
default. No screen needs any of the first three.

## Platform and scope

**MVP (question 64, assumed):** an iPad, as a private ad-hoc build for the
author's own devices; the platform list of section 2 is the product's scope.

**0.1 (decided 2026-10-21, [D11](../roadmap.md#4-decisions-that-gate-the-plan)).**
The first release is built, tested and packaged on the three **desktops**
(Windows, Linux, macOS), plus an **ad-hoc iOS build** for the author's own
devices. The iOS fragment, preset, signing job and install page exist and have
**never run on a device** ([CI](../ci.md#ios)). **Android is in progress in a
separate stream and is not built**: do not read the Android column below as a
claim that it works. Web and phone layouts come after 0.1. The tablet-first
table below remains the product's target.

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
Gui, Qml, Quick, QuickControls2, Quick3D (no longer used), ShaderTools, Svg, **Multimedia,
Sensors and TextToSpeech** for the feedback engines; the module archives for the
last three were fetched from the Qt repository and merged into the kit
(docs/building.md, "The Qt Quick window"). The iOS kit is installed by the iOS
CI jobs only. The application finds the three **optionally**: a contributor's kit without them
builds the silent variants of the sound, speech and motion backends
(`apps/app/src/feedback`), and the game is complete, only quieter.

| Module | For | In the tree | Offered under |
| --- | --- | --- | --- |
| Quick, Controls 2 | The UI | Yes | Commercial, LGPL-3.0, GPL-2.0 or GPL-3.0 (**checked**, SBOM) |
| QuickShapes, QuickParticles | Paths, confetti | Yes | The same (**checked**) |
| ShaderTools | Multimedia's build dependency | Yes | The same as Quick (**checked**). Quick3D, which the first build's 3D board used, is Commercial or GPL-3.0-only and is no longer linked |
| Multimedia | The short feedback assets; the camera on some targets | **Yes**, optional | Commercial, LGPL-3.0, GPL-2.0 or GPL-3.0 (**checked**, the kit's `sbom/qtmultimedia-6.12.0.spdx`) |
| Sensors | The accelerometer and gyroscope | **Yes**, optional | The same (**checked**, `qtsensors`) |
| TextToSpeech | Whole words and sentences, on the platform's engine | **Yes**, optional | The same (**checked**, `qtspeech`) |

The rule in [Licensing](../licensing.md#qt) holds: check how Qt offers a module
before adding it. All three are offered under the LGPL-3.0 as well as the GPL
and commercial options, so none changes the application binary's licence; it is
GPL-3.0-or-later like the sources (Quick 3D, which made it GPL-3.0-only, is gone).

## Screens and navigation

```text
                      ┌──────── home ────────┐
                      │                      │
        ┌─────────────┴────┐          ┌──────┴───────────┐
        │  Mode B: solo    │          │ Mode A: scanner  │       parent gate
        │  solo.box        │          │ scan.ready       │            │
        │   ▼ shake        │          │   ▼ capture      │       ┌────┴────────┐
        │  solo.playing    │          │ scan.check       │       │ dashboard   │
        │   ▼ summary      │          │   ▼              │       │ settings    │
        │                  │          │ assist.playing   │       │ about       │
        └──────────────────┘          └──────────────────┘       └─────────────┘
              My jewels, earlier boards: from home or the bar
      (Mode A is reached through the gate and the dashboard, not from home)
```

| id | Screen | Notes |
| --- | --- | --- |
| `home` | Two large pictures: the box (Solo Play) and the tablet on a stand (the family game, marked for grown-ups). A parent button, and a way to My jewels and earlier boards | **As built** the cards also carry a title and a line of text, and the family card opens the gate. A picture speaks its name when pressed |
| `solo.*` | Mode B | [Game design](game.md#mode-b-solo-play) |
| `scan.*`, `assist.*` | Mode A | [Game design](game.md#mode-a-the-family-assistant-and-scanner) |
| `jewels` | **My jewels**, the child's collection (the design's vault; [Game](game.md#progress-without-a-pull)) | From home; never offered after a board; no gate ([D10](../roadmap.md#4-decisions-that-gate-the-plan)) |
| `history` | Earlier boards, a sheet | From the bar in Solo Play, or from home |
| `parent.gate` | The gate | [Parent area](parent-area.md#the-gate) |
| `parent.*` | Dashboard, settings, about, licences, privacy | [Dashboard](dashboard.md) |

Rules that hold everywhere, taken from Cubeulator's journey:

1. **A way back and a way on.** Every state has a visible control that goes
   back. System back does the same and is never the only way.
2. **Back goes where the person came from**, with one exception: from
   `solo.playing` (and a word being shown, the shake or the drop) it goes
   `home` and **leaves the round as it is**; the next visit to Solo Play finds it
   there, because the game is saved on every find. The design had Back put the
   lid on and finish the board; the code does the other, and a test pins it
   (`tst_box`, "test_backFromPlayLeavesTheRoundInProgress").
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
  its place ("A, row 2, column 3"); the panel's words are a list, each named
  "pain, found". **Not built:** the polite live announcement when a word is found
  (WCAG 4.1.3): the list changes and the word is spoken aloud, and nothing is
  sent to a screen reader. The scene is a picture of the shake, so the
  tiles' accessible items are the flat board's, over it.

## Type

**The requirement (7.1):** large, child-friendly sans-serif type in which upper
and lower case are very clear and "I", "l" and "1" are not confused. Question
18 asks that this be checked by looking at the glyphs.

| Face | Licence | What is known | What is not |
| --- | --- | --- | --- |
| **Andika** (SIL) | SIL Open Font Licence (**read** from several secondary sources, not from the licence file) | Designed for beginning readers; single-storey a and g by default; its design notes list "capital I, lowercase l and numeral 1 that don't look alike" among its aims (**read**) | **As built:** bundled (Regular and Bold, `assets/fonts`). Its glyphs at this app's sizes with a child: **not looked at** |
| **OpenDyslexic** | OFL; Wikipedia gives version 1.1; the project says only the newest release moved to it, so older releases may differ (**read**) | Named in the brief | The same, and its design aims for the "I, l, 1" shapes were not checked |

**Recommendation.** Andika for the dice, the panel and every label, because it
was designed for beginning readers and for these distinctions; **a
side-by-side with a child decides it**, and both are named in the brief. Each
face's licence file must be read before it is bundled. Bundle it on every
platform: a browser has no system fonts (Cubeulator's web notes), and a
tablet cannot be assumed to have it.

**As built.** Three typefaces are bundled and compiled into the application,
all under the SIL Open Font License (`notices/fragments/fonts.txt`): **Andika**
(Regular, Bold) for the reading text, the dice, the words a child reads and every
numeral; **Pixelify Sans** (Medium, Bold) for the wordmark, titles, headings,
buttons and labels; and **Silkscreen** (Regular) for small upper-case labels
([Toybox](toybox/DESIGN.md), section 4).
OpenDyslexic is not bundled. The five-step glyph check below has not been done
with a child, and no specimen sheet is committed.

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
`zh_CN`, `ja`). The Language row in the parent area offers those eight and
**System**, which is the device's own language when it is one of them and
English otherwise. The setting is kept in the store with the others, so it
survives a restart, and the language is in force before the window is built:
`LanguageManager` (`apps/app/src/language_manager.*`) installs the catalogue and sets
`QGuiApplication::setLayoutDirection` before the engine loads. A change in the
parent area installs the new catalogue, turns the layout and calls
`QQmlEngine::retranslate()`, so every `qsTr()` sentence on screen changes at
once without a restart.

**The catalogues.** One Qt Linguist `.ts` file per language is in `apps/app/translations/`,
generated from the QML sources with the kit's `lupdate`
(`cmake --build <build> --target sns_update_translations`), and `lrelease`
compiles them at build time into `:/i18n/sns_<code>.qm`, embedded in the
application (docs/building.md, "The interface's languages"). English is the
source language and has a catalogue too, so the eight are checked alike. Every
user-facing string is `qsTr()`; the sentences of an area live in that area's
wording file (`ParentWording.qml`, `ScanWording.qml`, `SoloWording.qml`,
`HomeWording.qml`), and only the shell's window title is inline. `qWarning()`
and error text a developer reads stay English.

**What is translated.** The chrome: every label, sentence, accessible name and
message. **Not** the board: the dice letters, the words in the tray, the ENABLE
word list and the phonics content are English, because the game is an English
word game. The translations are **mechanical and unreviewed**; they are listed
with the phonics content for the literacy/reviewer pass, which decides the
wording a child and a parent see. The author's list of eight languages was
stated on 2026-10-19, with a full linguistic review as a later task.

**How complete the catalogues are.** Each of the eight has the same 279
messages and none is marked `unfinished`. That is not the same as translated:
**90 to 96 of the 279 messages in each non-English catalogue are identical to
their English source**. Some of those are right (`Qu`, numbers, a brand name),
the rest are untranslated or cognates; no test counts them
([Testing](../testing.md#the-interfaces-languages)). The Chinese, Japanese,
Hebrew and Yiddish catalogues are drafts.

**Right to left.** Hebrew and Yiddish read right to left. The window sets
`LayoutMirroring.enabled` with `childrenInherit` from the language, and the
screens that need the direction for something other than anchors read
`Theme.rightToLeft`: the navigation bars put the way back on the leading edge,
the parent area's rows and choices order from it, the gate's words follow it,
and the two-thirds/one-third
split puts the panel on the trailing side (the left) and the board on the
leading side (the right). The **board never mirrors**: the dice, the tiles, the
trace path and the board and its scene keep their reading order and their
coordinates in every language (`LayoutMirroring.enabled: false` on the board
containers). Numbers and dates use the locale.

**Fonts.** Andika (SIL OFL) is bundled and is the face for beginning readers,
the dice, the reading text and every numeral; Pixelify Sans carries the
interface's headings, buttons and labels, and Silkscreen the small upper-case
labels. Those three are compiled into the application's resources, with **Noto Sans SC, Noto Sans JP
and Noto Sans Hebrew** (SIL OFL), each a subset of the upstream variable font cut by
`tools/design/make_noto_subsets.py` to the characters of the catalogues and the languages' own names, in
two static weights (Regular, Bold): 0.86 MB in all, where the full CJK families would be about 27 MB.
The web has no system fonts, so a face for every script is bundled and no platform fallback is relied on.
In Chinese, Japanese, Hebrew and Yiddish the chrome and the sentences are set in the Noto face of the
language (`Theme.nativeFont`: Pixelify Sans, Silkscreen and Andika carry Latin only), and a
Han character is Chinese or Japanese by the language it is read in; the wordmark keeps the house faces.
The language picker names each language in its own script, in every interface language, and Qt finds those
glyphs among the registered fonts. The words on the board are English and are
drawn in Andika, so no CJK text is ever drawn into a picture. A test reads every catalogue and checks that
the bundled faces carry each character (`language_test.cpp`); the older glyph check of eight sample
sentences still skips itself on a platform whose fonts lack the script (docs/testing.md). **Recorded:** run on Windows on
2026-10-19, the chain carried every glyph of all eight samples (Latin with
accents, Hebrew, Yiddish, Chinese and Japanese).

**Checked by.** A catalogue-completeness and direction test
(`apps/app/tests/unit/language_test.cpp`: every language has a catalogue with the same
messages and none unfinished or vanished, Hebrew and Yiddish are right to
left and the rest left to right, a catalogue is really installed, the font chain
carries each language's sample); a QML right-to-left suite
(`apps/app/ui-tests/tst_rtl.qml`: the chrome mirrors and the board does not); the
`i18n.catalogues` drift check, which regenerates the catalogues with `lupdate`
and fails on any difference (skipped where `lupdate` is absent); and the screens
checks of the parent and scan areas in all eight languages. **Not checked:** a
Solo Play or Home smoke run in each language, text overflow in the longest
translations, or number and date formatting by locale.
## The Jewel Box design system

> **Being replaced by Toybox Pixel.** The restyle ([Toybox](toybox/DESIGN.md), signed off 2026-10-24) is
> built in steps. The fonts, the palette, the type roles and the contrast gates below are already Toybox's
> (`Theme.qml`, `Label.qml`); the colour jobs, the component shapes and the board art described here
> are the Jewel Box's until the pull requests that restyle them, and this section is rewritten with
> the last of them.

**As built.** One design system, the Jewel Box, serves every screen, the parent
area included: warm paper, a deep tray, cream dice, an emerald action colour and
a gold reward. It replaced three hand-built looks (the child screens, the parent
area's own theme and the scan screens) after the October 2026 human-factors
review of the prototype, whose findings are kept in git history rather than in
this tree (the last commit that holds it is `b1aeb44`, in
`docs/design/ux-audit-2026-10/`). The values live
in `apps/app/qml/kit/Theme.qml` and are **not copied here**: this page says what
the system is made of and which rules the tests hold it to.

- **One token source.** A screen writes no colour, size, radius or type value of
  its own; it reads `Theme`. Light and dark are chosen by the parent's Colour
  scheme setting, or by the system. The board is the one object that stays the
  same in both: the tray, the dice and the gold rim are the physical toy on the
  table.
- **Roles, not sizes, for text.** A `Label` takes a role (display, title,
  heading, child, label, body, word, numeral, eyebrow, caption) and never a pixel
  size. The scale has ten steps (14, 16, 18, 20, 24, 26, 34, 36, 40, 56) and
  nothing is smaller than 14 at any text scale. The chrome is Pixelify Sans and
  what a child reads, and every numeral, is Andika. Source strings are never
  upper-cased; a small label is an `Eyebrow`, which the platform capitalises.
- **Colour has jobs.** Emerald is the one action colour; selection is a soft
  emerald fill, an emerald edge and a check mark. **Gold is reward only**: the
  hint, the lit die, the tray rim, a jewel's rim; never a selection. A disabled
  control has its own fill, ink and edge, never reduced opacity, so its label
  keeps a readable contrast. A container is told from its surround by a line
  with a measured separation.
- **Components** (`apps/app/qml/kit`): `AppButton` (variants and three sizes,
  48, 64 and 96 dp, that grow with their label and wrap rather than truncate;
  a 2 px focus ring after a key press), `SegmentedChoice` (a titled group of
  radio buttons), `Card` (a title and an empty state), `Sheet` (a modal over its
  area with a scrim, Escape, a Tab trap and focus given back to the opener),
  `ActionBar` (one height ladder, child 96 and 64, parent 64 and 48),
  `AppBar` (way back, title, tabs), `BoardTray`, `JewelBox`, `ModeCard`,
  `Wordmark`, `Icon`, `Label`, `Eyebrow`, `FocusRing`. Icon-only buttons are
  allowed for back and close only.
- **Guard rails.** Four tests in `apps/app/ui-tests` keep it so: `quick.lint` rejects a
  pixel size, a letter spacing, a hex colour, a bare `Text`, a disabled opacity,
  gold used for selection and an icon-only button outside back and close, against
  an allow-list that may only shrink; `quick.contrast` measures about sixty
  foreground and background token pairs in both schemes against the gates below;
  `quick.sizematrix` lays the screens out at the sizes of "Window sizes" below
  (desktop, tablet and phone, both ways round) and at 150 % text and checks that
  every focusable item is at least 48 dp, inside its window, not truncated and
  not overlapping another control, that no page scrolls sideways and that no text
  runs off its card or the window; and
  `quick.kit` and `quick.theme` pin the components and tokens.
- **Window sizes.** A screen is laid out for the size it is given, by class and not by
  device (`Theme.isNarrow`, `isShort`, `isWide`): **narrow** below 600 dp wide (a phone
  held upright), **short** below 500 dp high (a phone on its side), **wide** from 900 dp
  (a tablet on its side, a desktop window). A tablet held upright is none of the three.
  Only a desktop window has a floor (900 x 620, `Theme.desktopMinWidth/Height`; the platform fragment's `SNS_PLATFORM_DESKTOP` says which, through `main.cpp`); a phone,
  a tablet and a browser page are as big as the screen is, and `Main.qml` lays the
  screens inside the device's safe area (`SafeArea`), so nothing sits under the notch, the
  rounded corners or the home indicator. The matrix holds each screen to 900x620,
  900x1200, 1024x768 and 2400x1200 (desktop), 820x1180 and 1180x820 (an iPad both ways),
  390x844, 375x667 and 360x640 (phones upright), 844x390 and 667x375 (phones on their
  side), and 150 % text at 900x620, 1024x768, 390x844 and 844x390. A suite whose screen
  is not yet laid out for the phones and the tablets names `SizeMatrix.desktopSizes`
  instead, and says which stage fixes it. On a narrow bar the boards and the jewels are
  pictures alone (they keep their names for a screen reader); the grown-ups' entry keeps
  its words, and a bar with a Back button leaves the product's name to the title.
- **Where it is not finished.** Solo Play and the scanner and assistant are not yet laid
  out for the phone and the tablet (their matrix suites run the desktop sizes only). The parent area's bare `Text` items are on the lint
  allow-list; at 150 % text on the smallest window the playing panel scrolls
  (Clear and Done are reachable below the fold).

## Colour and contrast

The gates are WCAG 2.x relative-luminance ratios, and they are **enforced by
`apps/app/ui-tests/tst_contrast.qml` from the live tokens in both schemes**, so this page
carries no table of colours (the palette it once tabulated was replaced, and a
table would be stale again with the next token change):

| What | Gate |
| --- | --- |
| Text: ink and secondary ink on every surface they sit on, including hovered and pressed | 4.5:1 |
| Large text, and a disabled label (exempt from WCAG, held to the house floor) | 3:1 |
| Non-text: a control's edge, the focus ring, the selected edge, a track, the gold of the path and of the hint against the die | 3:1 |
| Separation of a container from its surround, measured on the pair named: a line on a card, the tray on its stage, the panel on the stage | 1.1 to 1.6 |

The path is a pale line in a gold edge with a thin dark line, so it shows on the
dice and on the tray. And:

- **State is never colour alone.** Lit is a heavier edge; a grapheme link is a
  bar and a shared edge; the start tile is a ring; a doubtful tile is a ring
  and a `?`; a dead jewel has a shape; a selected choice has a check mark. This
  is Cubeulator's rule.
- **A jewel's colour is decoration.** Its size says its tier; nothing is "the
  red one". A world (7.2) swaps the board's look and background tokens and no
  rule.
- Nothing here has been looked at on a device.

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

**What is built and what is not.** The shake, lid and drop of the pixel scene, the
page and sheet transitions, and the reduced-motion behaviour of all of them are
built. **Not built:** the sparkle and rainbow trail on a word, the Mega burst
and bounce, confetti and the particle layer, the "single slow glow when the board
clears", and the resting screen (deleted on 2026-10-21; [Game](game.md#the-end-of-a-board)).
The rows above are the design for them and the limits any such effect must
keep.

**Who sets it.** The system's reduced-motion setting is the default and the
parent area can override it (Motion: System, Reduced). It is a parent's
decision, not a child's, and it is not behind the game.

**Motion as input, as built.** `MotionSource` (`apps/app/src/feedback`, over Qt
Sensors) reads the accelerometer and the gyroscope at the highest rate the
platform lists, feeds the samples to the scene's `pushMotion(ax, ay, az, wx,
wy, wz)` and to the pure `sns::MotionProcessor`, and reports a real shake once
per shake; the box then rolls as the Shake button does. It runs only while Solo
Play's box or the falling dice are on screen, only while Shake is on, and stops
when the screen is left. On the three desktops there is no accelerometer: the
source stays unavailable and the drag that stands in for one remains
(`SoloScreen.qml`). The processing is tested on synthetic traces
(docs/testing.md), not on hardware: the detector's starting values still have
to be tuned on a tablet.

**The shake and the drop are the pixel board scene**
([Board](board.md#the-shake-and-the-drop)). With reduced motion the scene keeps
the box still, cross-fades the lid and fades the dice in; nothing hops or falls, and the board appears and plays as it does otherwise.
The scene draws no particles of its own.

## Accessibility

| WCAG 2.2 criterion | How it is met |
| --- | --- |
| 1.1.1 Non-text content | Tiles, jewels, the start tile and stickers have names; the 3D scene's alternative is the 2D tile layer |
| 1.4.1 Use of colour, 1.4.11 Non-text contrast | The gates above, measured by `quick.contrast` |
| 1.4.4 Resize text | The parent's Text size is 100, 125 or 150 % **of the design size** (the app does not read the system's text size: Qt 6.12 does not expose it, [parent area](parent-area.md#integration)); tokens have a floor of 12 and the size matrix checks 150 % at two window sizes. 200 % is not tested |
| 2.2.1 Timing, 2.2.2 Pause, stop, hide | **No timer unless a grown-up chooses one** (off by default; D8); nothing loops over 5 seconds. A count-down ends the board and keeps everything found, which is a time limit a parent set deliberately |
| 2.3.3 Animation from interactions | Reduced motion (above) |
| 2.5.1 Pointer gestures | Tap in order is always on |
| 2.5.4 Motion actuation | The **Shake** button, and shake detection can be switched off |
| 2.5.8 Target size | 48 / 64 / 96 dp |
| 4.1.2 Name, role, value | Every control has a name and a role by convention; no test reads an accessible name or role yet |
| 4.1.3 Status messages | **Not built**: a found word is spoken aloud, and no live announcement goes to a screen reader |
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
| Resting (not built; [Game](game.md#the-end-of-a-board)) | Nothing | Nothing |

An error is never scolded (Cubeulator: "An error is not scolded"). Sounds are
under 300 ms except the sequence and the rattle. No music.

**As built.** The seam is still `Feedback.cue(kind, detail)` in the QML
(`apps/app/qml/kit/Feedback.qml`); Main.qml installs one `FeedbackEngine` on it
(`apps/app/src/feedback`), and the engine asks the pure policy
`sns::plan_cue` (`libs/core/include/sns/feedback.hpp`) what the cue and the
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
  anywhere else in the application (section 12.2). **The sound-it-out sequence
  is built and has no sounds to play.** The library makes the steps for a word
  of the phonics board (`sns::sound_it_out_sequence`: a sound for each block, a
  blend, the word, a cheer; a tricky word is said whole), and the application's
  `SoundSequencePlayer` walks them through a `ClipPlayer`. No phoneme clip
  exists, so the application has the player that answers "no such clip" and the
  word is said at once, as before: the isolated phonemes are the reviewer's
  recording job, and the platform's voice must not substitute letter names for
  them. The standard board's words are always said whole.
- **The assets** are placeholders made by `tools/sounds/make_sounds.ps1` and
  owned by the project (`assets/sounds/README.md`, notices). The real phoneme
  clips and recorded cheers replace them after the literacy review; the file
  names are the seam. The three Qt modules are optional at build time: their
  silent variants are chosen by CMake, so a kit without them still builds.
- **Touch feedback.** `libs/platforms/interfaces/sns/platform/haptics.hpp` is the
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
| **No network capability** | Android: no `INTERNET` permission in the merged manifest. All targets: first-party code does not use the network classes. **Not built:** a QML network manager that refuses everything but local and resource URLs (the application installs none), and the web page's content-security policy (there is no web build). The rule is kept by the source scan, not by a runtime refusal | A scan for the network classes (`privacy.sources`). **Planned, not built:** on Android, a dump of the built package's permissions (it must not list `INTERNET`; whether Qt's Android build adds it for a module is **unverified**, which is why it would be checked on the built package); a test that loads a remote URL in QML and fails; on the web, a header check. **Qt Network cannot be left unlinked**: Cubeulator's licensing paper notes that Qml and Quick depend on it, so the rule is "no capability and no use", not "no module" |
| **Zero export** | No share sheet, print, file-save or mail code; no URL scheme handler | A scan for the share, print and file-dialog classes. **Backups are an open question**: an iOS app's data is included in a device backup by default (**general knowledge, not checked**), which is data leaving the app under the user's own account; whether to exclude it is the author's decision (question 50) |
| **Local store** | A small versioned file, written atomically, in the app's private directory, with only what the dashboard needs (below). SQLite through Qt SQL is possible but the data is a few kilobytes and has no query that needs it | A test that round-trips and migrates a version |
| **No profiling in what ships** | Tracy listens on a TCP port (8086) when built in, and the Debug and profiling presets build it in, so a Debug build or its CI artefact is **not** a privacy-clean build. The release presets keep it off | `profiling.shipped_presets` reads the presets; `cpack` refuses to package a Tracy build, and `tools/package/check_package.ps1` rejects `TracyClient`, the AddressSanitizer runtime and `.pdb` files in a release package (the `Package (Windows, Release)` CI job runs it on the real package) |

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

## Open accessibility items

What the October 2026 review found and the build has not yet closed. Tick one off
only when a test or a recorded check shows it.

- [ ] **Live announcements.** Announce a found word politely and the board's
  summary when it opens (WCAG 4.1.3). Confirm with NVDA, VoiceOver and TalkBack.
- [ ] **Accessible names and roles are asserted nowhere.** A Quick Test that
  walks each screen and fails on an interactive item with an empty
  `Accessible.name` or no role, and one that checks the tiles are named "A, row 2,
  column 3".
- [ ] **A spoken label on press for every child control** (the kit makes icon-only
  buttons impossible outside back and close; the spoken part is not asserted).
- [ ] **The parent gate by keyboard and screen reader.** Space and Enter hold the
  button (`GateScreen.qml`); no test drives it with a key, and no screen reader
  has been tried on it.
- [ ] **The letter picker and the sheets as dialogs.** `Sheet` traps Tab and
  returns focus (`quick.kit`); confirm the Mode A letter picker does the same
  with a keyboard.
- [ ] **200 % text and a 320 px width.** The size matrix covers 150 % at two
  window sizes and a 900 px minimum window; the app claims nothing narrower.
- [ ] **Targets.** Anything the matrix does not reach: the "Show me a start"
  button is 64 dp and not 96, and word rows in the tray are shorter than 64 dp.
- [ ] **Keyboard play of the board** (arrows, Space, Enter, Escape, Backspace),
  including under right-to-left, has no test.
- [ ] **Screens never run on a device.** TalkBack and VoiceOver reading the
  flat board over the scene; the camera screens by a screen reader; reduced motion
  on a screen that is open when the system setting changes.

## User study plan (open)

Nothing in this design has been put in front of a child or a parent. This is the
plan for the first round; its results would amend the touch-target sizes, the
"real, not today's" cue and the gate. It is **not done**.

**Questions.** Can a five- or six-year-old start a game without help? Do they
know what **Shake** will do before they press it? Can they make a word and know it
was accepted? Do they understand the jewels and the tray? Can they stop and go
home? Would they play again without a pull? Can a parent find the gate, the
settings and the dashboard unaided, set up a board (camera or typed) and say what
the app keeps?

**Who.** Five children of five or six with a parent present, mixed reading
confidence, at least one not yet reading; five parents of such children; and, for
access, a keyboard-only adult, a screen-reader user and, if possible, a child with
a motor or visual access need (with a specialist's advice). A quiet room, the
release build, no network.

**Child session (20 to 30 minutes, play, not a test).** Show me how you would
start. What will this button do? (before Shake). Find a word. What now? (Done).
What happened, and how do you know? What are these? (jewels, tray). Show me how
you would stop. Play again? Five minutes of free play, watched without prompting.
Do not correct, point or explain.

**Parent session (30 to 40 minutes).** Get to the grown-up area. Turn the sound
off. Make the text bigger. Start a game with a real board. What did your child do
this week? What does the app keep and what does it never do? Where would you clear
progress, and what would you expect?

**Success criteria (targets, not guarantees).**

| Measure | Target |
| --- | --- |
| Time to first action, a child | median under 15 s, no help for 4 of 5 |
| Shake comprehension before pressing | 4 of 5 say or show shake, open or roll |
| A word made | 3 of 5 unprompted, 4 of 5 with one prompt |
| Found-word understanding | 4 of 5 point to the word or jewel |
| Leaving the board | 4 of 5 |
| Parent gate | unlocked in under 15 s by 4 of 5 parents |
| Parent settings (sound, text size) | found in under 60 s by 4 of 5 |
| Privacy understanding | 4 of 5 parents say "on this device" and "no recording or network" |
| Engagement | 4 of 5 children say yes to playing again; no distress |
| Parent questionnaire | SUS at least 75; single ease question at least 5.5 of 7 |

**Ethics.** Written parental consent and the child's assent; the child can stop at
any time; no audio or video without separate consent (the app records nothing and
the study follows it); codes, not names; notes kept on the local device and
deleted after; a familiar adult in the room; a thank-you that creates no pressure
to finish. **What would change the plan:** if the product is mainly a parent tool,
the home hierarchy flips; if the shake cannot be made reliable, play moves to the flat
board and the shake stays optional; if the phonics content is not ready, the
app is a word-finding game with a start-tile hint, not a phonics tutor.

## Decisions

Decided by the author and recorded with the date; the roadmap's
[decision table](../roadmap.md#4-decisions-that-gate-the-plan) holds the same
entries (D8 to D12) with the pages they amend. D8 to D11 are the author's; D12
and the two entries after it are recorded from the build and wait for the
author's confirmation.

- **2026-10-21, D8: a parent-set board clock stays.** The settings have a **Time
  on the board** row: off (the default), count up, or count down for 1 to 30
  minutes, which ends the board and keeps everything found. It amends the frozen
  "no timers or limits" (PRD 12.3). *Weakness:* the clock is visible to the child
  when set, and a count-down is a limit on a board; it is one the parent sets
  behind the gate and the app never starts. No duration is stored.
- **2026-10-21, D9: the parental gate is one press-and-hold.** A single button held
  for two seconds puts both of the library gate's virtual touches down; Space or
  Enter holds it by keyboard. It amends "two fingers held on two spots". *Weakness:*
  a determined child passes it with one finger, which the design's own review called
  too weak alone; it is a brake against accidents, not security. Nothing the gate
  protects is dangerous (no purchase, no network, no export); what it guards is the
  settings, the dashboard and the scanner.
- **2026-10-21, D10: "My jewels" stays ungated and shows the best board for each
  size, the points and the week.** It amends "shown to the parent only".
  *Weakness:* it is a number a child can try to beat. The records are the device's
  own and nothing is lost by a poor board.
- **2026-10-21, D11: 0.1 is the three desktops plus an ad-hoc iOS build; Android
  is in progress** and not built.
- **Recorded from the build, 2026-10-21, to confirm (D12): help is the parent's
  three-step setting** (off, the start tile, the start tile then two letters;
  default the last). The MVP was one level (PRD 12.5). It is built and tested, and
  the author has not yet said it should stay.
- **Recorded from the build, to confirm: the resting screen is not built**
  ([Game](game.md#the-end-of-a-board)); Back from play leaves the round in
  progress instead of putting the lid on; and the board size a child picks on the
  box becomes the next default, overwriting the parent's.

## Unknowns

- Whether the board and its scene run well on the author's iPad, and which iPad it is.
- Whether VoiceOver reads the flat board over the scene well.
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
