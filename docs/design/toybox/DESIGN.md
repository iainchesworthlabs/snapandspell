# Toybox Pixel for Word Jewels Solver

Status: draft 1 for sign-off, 2026-10-24. Nothing here is built. It describes the restyle that Stage 5 builds, screen by screen, against the application as it is in `develop` (fa1159b) and the screens listed in section 8. The mockups are in `screens/` and `screenshots/`; `gallery.html` is the gallery. The values are in `tokens.json` (Snake, unchanged) and `wjs-tokens.json` (what this project adds).

## 1. What changes and what stays

Changes: the look of every screen. The page becomes sunshine, ink becomes night purple, panels become cream with 4 px outlines and hard shadows, selection becomes coral, the chrome type becomes Pixelify Sans, and the gems, the box and the icons become pixel art drawn in code.

Stays:

* The game, the solver, the word lists, the dice, the store and its versioned format. Nothing in this brief changes a rule.
* The privacy rules: no network, no recording, no export, no stored duration.
* The reading face for the dice and for the words a child reads: Andika. Section 4 shows why.
* The target sizes: 48 dp for a grown-up, 64 dp for a child, 96 dp for the main action. Toybox's 44 px is the lowest value anywhere.
* The Colour scheme, Motion and Text size settings. Toybox has no dark scheme, so section 5 defines Toybox Night from the same palette.
* The parent area as one gated screen with four tabs (Dashboard, Settings, About, Licences).
* The guard rails: `quick.lint`, `quick.contrast`, `quick.sizematrix`, `quick.theme`, `quick.kit`. Their expectations change with the tokens, in the same pull request.

## 2. Tokens: shared and project

| Group | Shared with Snake (unchanged, `tokens.json`) | Added here (`wjs-tokens.json`) |
|---|---|---|
| Colour | background (sunshine), ink, shadow, surface (cream), accent (coral), boardLight, boardDark, food (blueberry), foodHighlight, leaf, tongue (pink), eye, overlay | the five gem colours (all taken from the shared palette), die, tray, surfaceDim `#EBD9B4`, path, and the Night set (page, panel, text, outline, shadowDeep `#0B0618`, button pair) |
| Shape | outline 4 and 3, radii, hard drop shadows small, medium and large, the board frame (6 px outline, 12 px drop) used for the tray | die radius and gap, gem tiers, panel and dialog values, the 20-step progress bar, the pixel-unit rule |
| Type | Pixelify Sans and its roles (wordmark, title, buttons, label, caption) | Andika as the reading face, Silkscreen for upper-case labels only, the word, dieLetter and points roles, minimum 14 px, the three text scales |
| Touch | minTouchTarget 44 | 48, 64, 96, gap 8, tile minimum 48 |
| Motion | scorePop, reducedMotion | boxShake, diceDrop, gemBob, tileFlash |
| Not used | the snake, tilt indicator and medal tokens | |

Rule for changes: a value that Snake and Word Jewels both use is changed in `tokens.json` first and copied to both. A value only this project needs goes in `wjs-tokens.json`. How the two repositories share the file is the open decision in section 15.

## 3. The pixel unit

Outlines (3, 4 and 6 px), hard shadows (3, 4, 6 and 12 px) and pixel art are drawn in whole device pixels. On a display with a fractional scale factor they are rounded to the nearest whole device pixel and never blurred. Snake scales its whole board in integer steps and letterboxes it. Word Jewels is a text-heavy interface in eight languages with a parent text size and windows from 900 x 620, so the layout stays fluid: only the borders, shadows and sprites are snapped, and the board scales in whole tile steps. The board is the only object that is ever letterboxed.

## 4. Type, and why the dice keep Andika

`screenshots/_legibility.png` sets Pixelify Sans next to Andika at dice and word sizes. For a beginning reader, Pixelify Sans has these faults: its `O` and `0` match; its `G` reads as `6`; its `a` and `g` read as solid blobs; at 16 to 30 px its `2`, `3` and `5` read as `Z`, `8` and `S` (in the first render of the Settings screen, "125 %" read "128 %" and "5 words" read "S words"). Silkscreen's digits have the same fault to a smaller degree. So:

| Role | Face | Size |
|---|---|---|
| Wordmark | Pixelify Sans 700, coral hard shadow 3 px | 40 (hero 76) |
| Display, screen title, heading | Pixelify Sans 700 | 56, 34, 24 |
| Buttons | Pixelify Sans 700 | 26 primary, 18 secondary |
| Label | Pixelify Sans 500 | 16 |
| Eyebrow (upper-case label) | Silkscreen 400, letter-spaced | 16 |
| Body, caption, hints | Andika 400 | 18, 14 |
| Found word | Andika 400 | 24 |
| Die letter | Andika 700 | 0.55 x tile |
| Every numeral (points, counts, sizes, order badges, percentages) | Andika 700, tabular figures | 20 and up |

Snake uses Pixelify Sans for its score. The same fault applies there; this is a finding for the Snake work (section 15).

`screens/_legibility.html` is the specimen: the pairs I, l, 1, O, 0, Q, 6, 9, b, d, p, q, M and W in both faces, dice at 120 px, and words at 26 px. It is the start of the glyph check that `docs/design/application.md` (question 18) asks for; it still needs a child to look at it.

Minimum text size is 14 px (Toybox's caption). The project's old minimum was 12.

## 5. Colour and contrast

Ratios are computed from the hex values (`source/contrast.py`, exit code 0 when every stated ratio is right).

| Use | Pair | Ratio |
|---|---|---|
| All text on panels and dice | ink on cream | 13.43 |
| Text on the page; start and hint tiles | ink on sunshine | 9.76 |
| Selected choice, tile in a path | ink on coral | 5.32 |
| Valid tile | ink on leaf | 7.69 |
| Primary button text | cream on ink | 13.43 |
| Labels on the board well | cream on boardLight | 5.27 |
| Night: text on the page | cream on shadow | 16.08 |
| Night: selected edge on a panel | coral on ink (non-text) | 5.32 |
| Invalid outline | pink on cream (non-text) | 3.23 |

Forbidden, because the pair fails 4.5:1 for text or has no edge of its own:

* Ink on pink is 4.16:1. No text sits on pink. The danger button is cream with a pink outline and an x mark.
* Ink on blueberry is 3.47:1 and white on blueberry is 4.33:1. Blueberry is a gem colour only.
* Coral on sunshine is 1.83:1. A coral object on the page always has an ink outline.

State is never carried by colour alone (WCAG 1.4.1). Every state in section 9 differs by shape, mark or position as well.

Toybox Night (Colour scheme: Dark): page `#1A1035`, panels `#2B1D52`, text and outlines `#FFF1D6`, shadow `#0B0618`, primary button sunshine with ink text, selected coral with ink text. The dice, the tray's well and the gems are unchanged: they are the toy on the table. One known detail to settle in Stage 5: a gem's ink outline vanishes on an ink panel, so in Night the gem outline is drawn in cream.

## 6. Components

All sizes in px at 1x. Hard shadows have no blur.

| Component | Spec |
|---|---|
| App bar | 72 high, panel fill, 4 outline below. Back (ghost button, chevron), title (Pixelify 28), right-hand buttons. The wordmark replaces the title on Home and in the parent area |
| Button | 3 outline, radius 14, shadow small plus 1 (0 4px). Sizes 48 (grown-up), 64 (child), 96 (primary action, radius 14, shadow 0 6px). Variants: primary (ink fill, cream text; Night: sunshine fill, ink text), secondary (cream fill), ghost (no fill, no shadow), danger (cream, 4 px pink outline, x mark), disabled (surfaceDim fill, dashed outline, ink text, no shadow). Pressed: down 4 px, no shadow. Focus: 4 px dashed outline in the outline colour, 3 px gap, after a key press |
| Choice (segmented) | A row of buttons. The selected one is coral, ink text, 4 px outline and a check mark before the label |
| Panel | Cream, 4 outline, radius 18, shadow 0 4px (0 6px for cards) |
| Dialog and sheet | Cream, 4 outline, radius 20, shadow 0 6px, 560 wide at most, over a scrim of ink at 80 percent. Escape closes, Tab is trapped, focus returns to the opener (as now) |
| Chip | 32 high, 3 outline, radius 8, Andika 700 16. Sunshine for "Often found" |
| Progress bar | 20 steps, each flexible by 20 high with a 3 px gap, coral when filled and surfaceDim when empty. Used by the gate only |
| Tray | 14 px padding, 6 px outline, 12 px drop, radius 12, a teal checker well (boardLight, boardDark) with one cell behind each die |
| Die | Cream face, 3 px ink outline, radius 10, shadow 0 4px, Andika 700 at 0.55 x tile (0.42 for the "Qu" tile) |
| Gem | A pixel sprite with an ink outline, a highlight at the top-left and a shade at the bottom-right. Three shapes by tier: diamond (1 star), cut gem (2 stars), star (5 stars). Colour from the five-colour list in turn, so it carries no meaning. 3 px per cell in lists, 4 to 6 in dialogs |
| Icons | Pixel maps drawn in code from one table (check, back, x, play, person, camera, boards, retry, shake, hint). Icon-only buttons stay limited to back and close |
| Wordmark | "Word Jewels" in Pixelify 700 with a 3 px coral hard shadow, with the cut gem to its left |

## 7. Tile states

`screens/tile-states.html`. Letters are ink on every fill.

| State | Fill | Marks |
|---|---|---|
| Normal | cream | none |
| In path | coral | sinks 3 px, loses its shadow, shows its order number in a disc at the top-left |
| Start tile (help) | sunshine | dashed cream ring (the tray is ink, so an ink ring would vanish), a play mark at the top-left |
| Second help tile | sunshine | a square mark at the top-left |
| Valid word | leaf | check mark at the top-right for 600 ms, tiles sink |
| Not a word | cream | 4 px pink outline, x mark at the top-right, a two-step sideways shake (not with reduced motion) |
| Doubtful (scan) | cream | question mark in a sunshine disc at the top-right |
| Disabled | surfaceDim | dashed outline, no shadow |

The path joins the tiles with a dotted trail: square ink dashes with a cream centre, drawn over the dice so the letters stay readable. It replaces the project's solid ribbon, which hides the letters it crosses.

## 8. Screens

Every screen and dialog of the application with its mockup. Window 1160 x 740 unless stated; the same layouts hold from 900 x 620.

| # | Screen | Layout and components | Mockup |
|---|---|---|---|
| 1 | Home | Transparent bar with wordmark and three buttons (My boards, My jewels, For grown-ups). A hero line, "Shake. Find. Collect." in Pixelify 52. Two panels side by side: the child's card (box art, eyebrow, heading, one line, a 96 px primary Play button) and the grown-up card (board art, eyebrow, heading, line, a 64 px secondary Continue). The illustration gives way first when height is short or text is large (see `home-text150`) | `home`, `home-zh`, `home-de`, `home-text150` |
| 2 | Solo Play, the box | Left: the pixel box with its lid ajar, caption "Shake the box to start". Right panel: Choose a board (3, 4: only sizes that fit), How it works (three numbered steps in Andika), a 96 px Shake button | `solo-box` |
| 3 | Solo Play, playing | Left: tray with the board. Right panel (410): Words I found (count, six-digit points, the list with a gem, word and +points), the message line (the word in progress, or the result), Show me a start, Finish board, Clear and Done. Timer chip when the parent switches the clock on | `solo-playing`, `-empty`, `-path`, `-invalid`, `-hint`, `-night`, `-he`, `-tablet` |
| 4 | Summary card | A dialog over the dimmed screen: "You found all 5 words!", a bobbing gem and "5 stars", the points, the word list, Play again (primary) and Home | `summary` |
| 5 | My jewels | Three stat panels (stars, points, words in Andika at 36), best board for each size, the gem collection, earlier boards as thumbnail trays, a seven-bar week chart | `my-jewels` |
| 6 | Earlier boards (sheet) | The thumbnails of the last twenty boards in a dialog. Not mocked separately: it reuses the thumbnail tray from My jewels | (my-jewels) |
| 7 | Parent gate | A dialog: "For grown-ups", the instruction, a large hold button, the 20-step bar, the keyboard line | `parent-gate` |
| 8 | Dashboard | The parent shell (bar with wordmark, "Parent area" and four tabs), the "Play with a real board" card, the week's lines, the Sounds rows each with a chip | `parent-dashboard` |
| 9 | Settings | Rows of title, hint and choice, grouped under Play and the device settings. Language values show the language's own name | `parent-settings`, `settings-de` |
| 10 | About | The gem and name, tagline, the version block (monospace), Licence and source, Privacy. See section 12 | `parent-about` |
| 11 | Licences | The generated notices text in a scrolling panel (Andika 16, monospace for licence bodies). Not mocked: it is the existing text in the new panel | (parent-about) |
| 12 | Real board: set up | Three-step stepper, the camera frame (ink with corner marks) or the no-camera picture, Try camera again, Type in your board | `scan-setup` |
| 13 | Real board: check | The read board with doubtful tiles marked, board size choice, Turn, Scan again, Start playing (disabled until the letters are confirmed) | `scan-check` |
| 14 | Real board: assistant | The playing layout with the confirmed board; the child gets hints, not the list | `assist` |
| 15 | Kit components | The component sheet | `components`, `tile-states`, `gems` |
| 16 | Startup failure | A native message box. Not styled | none |

## 9. Layouts and platforms

* Desktop: as the mockups. Window 1160 x 740 by default, minimum 900 x 620, fluid.
* Tablet: portrait stacks the board above the panel (`solo-playing-tablet`, 834 x 1112 points). Safe areas pad the app bar and the bottom row. The tablet is the product's target (roadmap D1). Child targets stay 64.
* Phone: portrait, the same stack with the panel as a bottom sheet. Not built in 0.1; not mocked.
* Web: see section 11.

## 10. Motion

Toybox motion with these Word Jewels additions. With reduced motion (the system's, or the parent's setting) every row below is replaced by its still form.

| Effect | Full | Reduced |
|---|---|---|
| Shake of the closed box | 600 ms, eight stepped jumps of 6 px | the box stays still, the lid cross-fades |
| Dice drop | 380 ms, six flip-book frames per die, 40 ms stagger | the dice appear at once |
| Valid and invalid marks | 600 ms flash, at most 3 flashes a second | the mark appears, no flash |
| Invalid sideways shake | two steps | none |
| Summary gems | bob 3 px over 0.9 s, stops after 5 s | still |
| +points | rises 14 px over 1.6 s (shared scorePop) | appears in place |

No screen shake for anyone. Nothing loops for more than 5 seconds.

## 11. The web build

The product should also be playable in a browser. What that asks of the design:

* Layout: the desktop layout inside the page, resizable, with the same minimum. Touch and mouse both work.
* Fonts: a browser has no system fonts, so every face for every language is bundled and subset (section 13). The first load budget is set in Stage 5 once the sizes are measured.
* Shake: no accelerometer is assumed; the Shake button, Space and a drag are the ways in (as on a desktop).
* Camera: only if the browser grants it; the typed board is the way on (as with no camera today). Qt Multimedia's browser support is unverified.
* Speech: `docs/design/application.md` records that a search of Qt's TextToSpeech engines found none for WebAssembly, so spoken words are silent on the web unless recorded clips are added. Unverified.
* The 3D scene: Quick 3D runs on WebGL 2 but is large and heavy. See section 12.1, which recommends the 2D scene.
* Build: Qt for WebAssembly, single-thread kit (GitHub Pages cannot send the headers that multithreaded Qt needs), published under `/demo/` by the docs workflow (standard section 14).
* Licence: a static WebAssembly build delivers the application and Qt to the user, so the same GPL terms and source offer apply as to a download.

## 12. Decisions for Iain

### 12.1 The 3D board scene

The shake and drop use Qt Quick 3D with rigid-body dice. Flat pixel art with hard shadows does not sit with a lit 3D scene, Quick 3D is GPL-3.0 only (so the binary is), and it is heavy for the web.

* A. Keep Quick 3D and flatten its materials (toon shading, ink outlines). The look stays close to the 3D scene's physical feel. The web build and the licence note stay as they are.
* B. Replace the scene with the existing 2D scene, restyled as pixel art: the box shakes in stepped jumps and each die flips through six frames. **Recommended.** It matches Toybox, runs on the web, and removes the GPL-3.0-only effect of Quick 3D (the binary can be GPL-3.0-or-later again, which also eases the App Store question in standard D8). It costs the 3D feel. The 2D scene and its script already exist (`src/app/qml/scene/fallback`).
* C. Both, chosen by a setting or by the platform (3D on desktop with a GPU, 2D elsewhere). Doubles the art and the tests.

### 12.2 Dice and words in Andika

Recommended and mocked. The alternative is Pixelify Sans on the dice, which is closer to Snake and harder for a five-year-old to read (section 4).

### 12.3 Night scheme

Keep the Colour scheme setting with a Toybox Night set (section 5). The alternative is to drop the dark scheme, which removes a setting that exists today.

### 12.4 Words in the CJK and Hebrew scripts

See section 13.

### 12.5 Touch targets

48, 64 and 96 stay above Toybox's 44. Recommended.

## 13. Languages and fonts

Pixelify Sans, Silkscreen and Andika cover Latin. Hebrew, Yiddish, Chinese and Japanese need other faces, and the web needs them bundled.

| Script | Strategy | Notes |
|---|---|---|
| French, Spanish, German | Andika and Pixelify Sans. The Latin subset of Pixelify covers the accents; check ß and œ in Stage 5 | `home-de` shows German, the longest strings, fitting |
| Hebrew and Yiddish | Noto Sans Hebrew (SIL OFL), subset | Pixelify has no Hebrew. RTL mirrors the chrome; the board never mirrors |
| Simplified Chinese, Japanese | **Recommended:** Noto Sans SC and Noto Sans JP (SIL OFL), subset by `pyftsubset` to the glyphs in the catalogues. The words on the board are English, so the catalogues are the only CJK text | Clean, small after subsetting (measure in Stage 5), works on the web. Snake's roadmap makes the same choice |
| Pixel-style CJK (option) | Fusion Pixel Font, zh-Hans and ja (SIL OFL 1.1 per its repository, release 2026.10.16), for headings only | Drawn on a 12 px grid, so crisp at 12, 24, 36 and 48 px. Its upstream parts include fonts with other terms (the repository names Misaki as compatible with OFL), so the licence file of the exact release must be read before it ships. **Zpix is excluded**: its licence is paid for commercial use and forbids modification and subsetting |

The Chinese and Hebrew mockups (`home-zh`, `solo-playing-he`) are rendered with the Windows system fonts, because the bundled faces are not chosen yet. They show that the layouts fit; they do not show the final glyphs.

Strings that are missing from a catalogue appear in English in the mockups (the German and Hebrew pages show a few). The translations are mechanical and unreviewed, as `docs/i18n.md` will say.

## 14. About and Settings

Snake left both open. Word Jewels has both, so the pattern is proven here:

* One parent area behind the gate, four tabs: Dashboard, Settings, About, Licences. The settings and About are not child-facing, so they live behind the same hold gate.
* Settings (built): Help, Board, Phase, Focus sounds, Board size, Sound, Celebration sounds, Sound it out, Vibration, Shake, Motion, Colour scheme, Language, Text size, Board clock, Reset (two steps). The Toybox restyle changes the controls, not the list.
* About (built, one block added): name and tagline, version block, **Licence and source** (new: the licence of the source files, the note about the Quick 3D binary if scene option A is kept, the source address with tag and commit, a button to the licences tab), and Privacy.
* Licences (built): the generated notices.
* Snake could copy the pattern with a smaller list: inverted axes, reduced motion, sound and music toggles, and reset leaderboard in Settings; the same About block. Whether Snake's Settings sits behind a gate is Snake's decision (its players are not necessarily children).

## 15. Shared theme: options

Snake and Word Jewels both use Toybox. Snake has the tokens and the mockups but no theme code yet; its roadmap plans a QML theme singleton filled from `tokens.json` by a CMake step, with a test that fails when they differ. Word Jewels already has a token source (`kit/Theme.qml`) with guard rails, and eight languages and a CJK strategy to solve.

| Option | What it is | For | Against |
|---|---|---|---|
| A. A copy in each repository | Each project holds `tokens.json` and its own Theme | Nothing new to maintain; each repository stands alone | The files drift; a fix is made twice |
| B. Tokens as data, one implementation per repository (**recommended**) | `tokens.json` (and the font files with their licences and the subsetting script) live in one place, `icl-common-infra/design/toybox/`, versioned by tag and vendored with a `SOURCE` file like `cmake/icl`; each project generates its own Theme from it with a CMake step and tests that the generated file matches | One source of truth for values and fonts; no shared C++ or QML API to version; fits the existing vendoring and repin tools | Each project still writes its own components |
| C. A shared Qt theme library | A repository with the Theme singleton, the styled controls and the drawing helpers (hard shadow, pixel sprites), consumed as a CMake package | Components shared too | A shared Qt API to version across a Qt Quick desktop app and Snake's second front end on LVGL (which cannot use QML); a new repository and its CI; the first two consumers are different enough that the common API would be guessed |

Recommendation: B now, and move to C only when a third Qt Quick application needs the same components. Which repository leads: Word Jewels is ahead on the Qt side (a kit, guard rails, eight languages), Snake on the design package. Iain decides; I recommend Word Jewels writes the first generated Theme and the font subsetting step, and Snake adopts them.

## 16. Art, audio and distribution

* Art: drawn in code in the Toybox style (gems, box, icons, wordmark). No sprite files, so the game does not wait for assets. The application icon is a pixel gem on sunshine with an ink outline; the 1024 px source is generated by `tools/package/make_*_icon.py` from a small script.
* Sound: the eight placeholder cues stay; the same synthesis script gains a chiptune voice for them. No music in 0.1 (a calm game for young children; the parent's Sound setting stays). The phoneme clips remain a recording job.
* Distribution: the roadmap's first release is the three desktops and an ad-hoc iOS build; the web build is added to that list. Public store builds are a later licence and signing decision (standard D8, D17).
* Name: keep "Word Jewels Solver" for the repository, About and CLI, and "Word Jewels" as the wordmark, as the app already does.
* Screenshots: the README and release notes carry screenshots taken by CI from the offscreen app, one set for each platform leg, so the look can be checked without building.

## 17. Mapping to the current code

`src/app/qml/kit/Theme.qml` holds the tokens today. The restyle changes values and adds properties; component files keep their names.

| Theme property now | Toybox value |
|---|---|
| `page` | `background` (day) or `#1A1035` (night) |
| `surface`, `card`, `panel` | `surface` `#FFF1D6` (day) or `#2B1D52` (night) |
| `ink` | `ink` `#2B1D52` (day) or `#FFF1D6` (night) |
| `accent`, `buttonFill` | primary button: ink (day), sunshine (night); selection: coral |
| `gold`, `goldSoft` | retired; the hint is sunshine |
| `tray`, `trayEdge`, `well` | ink frame, teal checker |
| `die`, `dieInk`, `dieEdge` | `surface`, `ink`, `ink` (unchanged in Night) |
| `jewelColors` | the five palette colours |
| `uiFont`, `fontFamily`, `monoFont` | Pixelify Sans, Andika, Silkscreen (labels only) |
| `radius*` | 4, 8, 10, 14, 18, 20 as in `tokens.json` |
| `targetMin`, `targetChild`, `targetPrimary` | 48, 64, 96 (unchanged) |

The guard rails then check the new values: `quick.contrast` holds the pairs in section 5, `quick.lint` rejects a hex value outside `Theme`, and `quick.sizematrix` holds the target sizes at 900x620, 900x1200, 1024x768, 2400x1200 and 150 % text.
