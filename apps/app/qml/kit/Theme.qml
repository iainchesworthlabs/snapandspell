// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick
import SnapAndSpell

// The one design system of the application: Toybox Pixel (docs/design/toybox/DESIGN.md). A sunshine
// page, night-purple ink, cream panels with hard outlines and hard shadows, a coral selection and five
// gem colours. Pixelify Sans is the chrome (wordmark, titles, headings, buttons, labels), Silkscreen the
// upper-case eyebrow, and Andika the face a child reads: the dice, the words, the body text and every
// numeral. Every screen, the parent area included, reads these tokens; a screen writes no colour, size
// or type value of its own. The lint test in apps/app/ui-tests keeps it so (tst files `lint` and
// `contrast`).
//
// Light and dark are chosen by the parent's Colour scheme setting, or by the system when that is
// System. Toybox has no dark scheme, so Toybox Night is made from the same palette (section 5 of the
// design): a deep-purple page, ink panels, cream text and outlines. The dice, the tray's well and the
// gems are the same in both: they are the physical toy on the table.
//
// Colour rules, held by apps/app/ui-tests/tst_contrast.qml (text 4.5, large text 3, non-text 3):
//   * Ink is the one text colour on the page and on panels, and the fill of the primary button (Night:
//     sunshine, with ink text). Selection is `selectedFill` (coral) with ink text and an ink edge, and
//     a check mark - never colour alone.
//   * No text sits on pink or on blueberry (ink on pink is 4.16:1, on blueberry 3.47:1). Coral and
//     sunshine objects on the page always have an ink outline (coral on sunshine is 1.83:1).
//   * A disabled control is `disabledFill` with a dashed edge and no shadow, never `opacity`.
//   * Every state also differs by shape, mark or position, not by colour alone (WCAG 1.4.1).
//
// Type rules: the scale is 14, 16, 18, 20, 24, 26, 34, 36, 40, 56 and nothing is smaller than 14. A
// screen picks a role through kit/Label.qml and never a size.
QtObject {
    id: root

    // --- what the person or the system chooses ------------------------------
    property bool reducedMotion: false
    property real textScale: 1.0
    property string colourScheme: "system"
    property bool rightToLeft: false
    // The interface's language code ("en", "fr", "es", "de", "yi", "he", "zh_CN", "ja"), set by the shell
    // from the LanguageManager; it decides the face of the chrome and the sentences (a Han character is
    // Chinese or Japanese by the language it is read in).
    property string language: "en"

    readonly property bool dark: colourScheme === "dark"
                                 || (colourScheme !== "light" && SystemColourScheme.dark)

    // --- colour: the palette (the same in both schemes) ---------------------
    // The named colours of Toybox (tokens.json). A component that needs one of these in either
    // scheme names it here; the scheme's own roles follow.
    readonly property color sunshine: "#FFC93C"
    readonly property color cream: "#FFF1D6"
    readonly property color coral: "#FF6B4A"
    readonly property color pink: "#FF2E63"
    readonly property color leaf: "#5BD16A"
    readonly property color blueberry: "#4C6BFF"
    readonly property color blueberryShine: "#DDE3FF"
    readonly property color nightPurple: "#2B1D52"
    readonly property color deepPurple: "#1A1035"
    readonly property color tealLight: "#1F6E7A"
    readonly property color tealDark: "#1A6170"

    // --- colour: the chrome (light / dark) ----------------------------------
    // Light: a sunshine page, cream panels. Night: a deep-purple page, night-purple panels.
    readonly property color page: dark ? deepPurple : sunshine
    readonly property color surface: dark ? nightPurple : cream
    readonly property color card: surface
    readonly property color panel: surface
    // A darker cream (Night: a lighter purple): the empty well of a panel, a disabled fill.
    readonly property color surfaceDim: dark ? "#3A2B66" : "#EBD9B4"
    // The stage the board stands on is the page itself: the tray has its own frame.
    readonly property color boardArea: page
    readonly property color ink: dark ? cream : nightPurple
    // Secondary text: 6.25:1 or better on every light surface, 5.46:1 on the Night ones.
    readonly property color inkSecondary: dark ? "#C4B7DD" : "#4A3A7C"
    // The edge of a container and of a control is the ink itself.
    readonly property color line: ink
    readonly property color outline: ink
    // The hard drop shadow: no blur, always darker than what it falls on.
    readonly property color shadow: dark ? "#0B0618" : deepPurple
    // Selection, a path, a highlight: always with an ink outline on the page (1.83:1 on sunshine).
    readonly property color accent: coral
    // Text that stands out from the rest: ink on light, sunshine on Night (never coral).
    readonly property color accentText: dark ? sunshine : nightPurple
    // A chip's fill (Often found): sunshine in both schemes; its text is `chipInk`.
    readonly property color accentSoft: sunshine
    readonly property color chipInk: nightPurple
    readonly property color buttonFill: dark ? sunshine : nightPurple
    readonly property color buttonInk: dark ? nightPurple : cream
    // The focus ring is ink on light and cream on Night, drawn outside the control with a gap.
    readonly property color focusRing: ink
    readonly property color scrim: Qt.rgba(0.169, 0.114, 0.322, 0.8)
    // Reward and hint: sunshine. `goldInk` is emphasised text on a panel (the points, a low clock), so it
    // follows the scheme like `accentText`; text on the hint's own sunshine card is `hintCardInk`.
    readonly property color gold: sunshine
    readonly property color goldInk: accentText
    readonly property color goldSoft: sunshine
    readonly property color hoverWash: dark ? "#3A2B66" : "#FFE7B8"
    readonly property color pressWash: dark ? "#473577" : "#FFDA8F"

    // A chosen option: coral, ink text, an ink edge and a check mark.
    readonly property color selectedFill: coral
    readonly property color selectedEdge: ink
    readonly property color selectedInk: nightPurple

    // A control that cannot be used now: a dimmer fill, a dashed edge and no shadow. Its label stays
    // at full strength on the fill, so nothing under it needs to show.
    readonly property color disabledFill: surfaceDim
    readonly property color disabledInk: ink
    readonly property color disabledEdge: ink

    // A destructive action: a cream (Night: night-purple) button with a pink outline and an x mark, the
    // text in ink. Pink is never a text colour.
    readonly property color danger: pink
    readonly property color dangerFill: surface
    readonly property color dangerInk: ink

    // An empty progress track is the dimmer surface; a filled step is coral.
    readonly property color track: surfaceDim

    // The hint's own card.
    readonly property color hintCard: sunshine
    readonly property color hintCardInk: nightPurple

    // The resting screen is meant to be looked at without wanting anything.
    readonly property color restPage: dark ? "#241848" : "#FFD76B"
    readonly property color restShape: dark ? "#33265E" : "#EBD9B4"
    readonly property color restDetail: dark ? "#3F3070" : "#E0CA9A"

    // --- colour: the board (the same toy in both schemes) -------------------
    // The tray is an ink frame round a teal checker well; in Night the frame is cream, as the outlines are.
    readonly property color tray: nightPurple
    readonly property color trayEdge: dark ? cream : nightPurple
    readonly property color trayLine: tealDark
    readonly property color trayShadow: shadow
    readonly property color lid: cream
    readonly property color well: tealDark
    readonly property color wellCell: tealLight
    readonly property color die: cream
    readonly property color dieInk: nightPurple
    readonly property color dieEdge: nightPurple
    readonly property color dieLit: coral
    readonly property color dieLitInk: nightPurple
    readonly property color bevelLight: "#FFFFFF"
    readonly property color bevelShade: deepPurple
    // The path is a dotted trail drawn over the dice: square ink dashes with a cream centre.
    readonly property color pathCasing: nightPurple
    readonly property color pathCore: cream
    readonly property color ribbonEdge: nightPurple
    readonly property color ribbonCore: cream
    readonly property color ribbonLine: nightPurple
    // A jewel not yet found: 3.2:1 on cream.
    readonly property color jewelDead: "#9C8450"

    // A jewel's colour is decoration: its shape says its tier. These are taken in turn by the order a
    // word was found.
    readonly property var jewelColors: [coral, blueberry, leaf, pink, sunshine]

    // --- type --------------------------------------------------------------
    // Pixelify Sans: the chrome. Silkscreen: the upper-case eyebrow only. Andika: the face a child
    // reads - the dice, the words, the body and caption text and every numeral.
    readonly property string uiFont: nativeFont !== "" ? nativeFont : "Pixelify Sans"
    readonly property string fontFamily: "Andika"
    readonly property string eyebrowFont: nativeFont !== "" ? nativeFont : "Silkscreen"
    // The one monospace: the About version block and the licence bodies. Courier New is on Windows and
    // macOS (so Qt has no missing family to warn about); elsewhere fontconfig substitutes its metric twin.
    readonly property string monoFont: "Courier New"
    // Chinese, Japanese, Hebrew and Yiddish are set in a bundled Noto Sans face of their own (subsets,
    // assets/fonts, tools/design/make_noto_subsets.py), for the chrome and for the sentences: Pixelify Sans,
    // Silkscreen and Andika carry Latin only, and the web has no system fonts to fall back on. The board's
    // dice, the English words and every numeral stay Andika (`fontFamily`). A glyph from another script in
    // a Label (the language picker names each language in its own) is found by Qt among the registered
    // fonts.
    readonly property string nativeFont: language === "zh_CN" ? "Noto Sans SC"
                                       : language === "ja" ? "Noto Sans JP"
                                       : (language === "he" || language === "yi") ? "Noto Sans Hebrew" : ""
    // The face of a sentence the interface says: Andika, or the language's own Noto face.
    readonly property string textFont: nativeFont !== "" ? nativeFont : fontFamily

    // Nothing is set smaller than this, at any text scale (Toybox's caption).
    readonly property real minTextSize: 14
    // The scale, for tests and for anyone who needs the list.
    readonly property var typeScale: [14, 16, 18, 20, 24, 26, 34, 36, 40, 56]

    function rem(amount) {
        return Math.max(minTextSize, amount * 16 * textScale);
    }

    readonly property real monoSize: rem(1.0)           // 16, the eyebrow
    readonly property real microSize: smallSize
    readonly property real smallSize: rem(0.875)        // 14
    readonly property real captionSize: smallSize
    readonly property real bodySize: rem(1.125)         // 18
    readonly property real labelSize: rem(1.0)          // 16, a parent's button label
    readonly property real headingSize: rem(1.5)        // 24
    readonly property real wordSize: rem(1.5)           // 24
    readonly property real childLabelSize: rem(1.625)   // 26, the primary button
    readonly property real titleSize: rem(2.125)        // 34
    readonly property real wordmarkSize: rem(2.5)       // 40
    readonly property real numeralSize: rem(2.25)       // 36, a statistic
    readonly property real displaySize: rem(3.5)        // 56

    // Line height, as a multiple of the size (the brief: 1.4-1.5 for text,
    // 1.15-1.25 for display).
    readonly property real lineDisplay: 1.15
    readonly property real lineTitle: 1.2
    readonly property real lineHeading: 1.3
    readonly property real lineBody: 1.5
    readonly property real lineChild: 1.3
    readonly property real lineWord: 1.35
    readonly property real lineMono: 1.35
    readonly property real lineCaption: 1.4

    // Letter spacing, in em: Label turns it into pixels. Positive tracking is
    // not applied in a right-to-left language (it breaks Hebrew's letter forms).
    readonly property real trackMono: 0.08
    readonly property real trackDisplay: 0
    readonly property real trackNone: 0

    // --- sizes, in dp ------------------------------------------------------
    readonly property real targetMin: 48
    readonly property real targetChild: 64
    readonly property real targetPrimary: 96
    readonly property real targetGap: 8
    readonly property real rowHeight: targetMin

    readonly property real space1: 4
    readonly property real space2: 8
    readonly property real space3: 12
    readonly property real space4: 16
    readonly property real space6: 24
    readonly property real space8: 32
    readonly property real space12: 48
    readonly property real space16: 64

    readonly property real hair: 1
    readonly property real rule: 1
    readonly property real heavy: 2
    readonly property real litEdge: 3
    readonly property real litScale: 1.03
    readonly property real focusWidth: 4
    readonly property real focusGap: 3
    readonly property real pathCoreWidth: 5
    readonly property real pathCasingWidth: 9
    // The path as a share of the tile, for a screen that scales it: the core
    // and casing are never thinner than the two widths above.
    readonly property real pathCoreShare: 0.07
    readonly property real pathCasingShare: 0.11
    // The ribbon: 14 and 22 px on the 200 px die of a roomy 3 x 3, scaled with the
    // die and never thinner than these on a small one (a 6 x 6 on a small window).
    readonly property real ribbonCoreMin: 8
    readonly property real ribbonEdgeMin: 14
    function ribbonCoreWidth(tile) {
        return Math.max(ribbonCoreMin, tile * pathCoreShare);
    }
    function ribbonEdgeWidth(tile) {
        return Math.max(ribbonEdgeMin, tile * pathCasingShare);
    }
    // The trail's dashes: square on the casing (`on` of its width, then the gap), and the same lengths
    // on the narrower core, so the cream centre sits in the ink dash.
    readonly property real trailOn: 0.9
    readonly property real trailOff: 0.7
    function trailDashes(casing) {
        return [trailOn, trailOff];
    }
    function trailCoreDashes(casing, core) {
        return [trailOn * casing / core, trailOff * casing / core];
    }
    function ribbonLineWidth(tile) {
        return ribbonEdgeWidth(tile) + 2 * heavy;
    }
    // The hint's ring round a die: a gold band with a thin dark line on its outer
    // edge, so it holds 3:1 on a cream die as well as on the tray, and two rings
    // side by side stay two.
    readonly property real ringWidth: 4
    readonly property real ringWidthSecond: 3

    // Shape (DESIGN.md, sections 3 and 6): outlines and hard shadows are whole pixels, no blur.
    readonly property real outlineControl: 3
    readonly property real outlinePanel: 4
    readonly property real outlineFrame: 6
    readonly property real shadowSmall: 3
    readonly property real shadowMedium: 4
    readonly property real shadowLarge: 6
    readonly property real shadowFrame: 12
    // A pressed button sinks onto its own shadow; a dashed edge is this many outline widths on, as many off.
    readonly property real pressDepth: 4
    readonly property real dashLength: 4

    // Radii: a segment, a chip, a control and a die, a button, a panel, a dialog.
    readonly property real radiusXs: 4
    readonly property real radiusSmall: 8
    readonly property real radiusControl: 10
    readonly property real radius: 14
    readonly property real radiusPanel: 18
    readonly property real radiusLarge: 20

    readonly property real appBarHeight: 72
    readonly property real barHeight: appBarHeight
    // The least of a screen's title a narrow bar keeps before its trailing group drops to a second row.
    readonly property real barTitleMin: 88

    // --- window sizes, in dp -----------------------------------------------
    // A screen is laid out for the size it is given, by class and not by device:
    // narrow below `compactWidth` (a phone held upright), short below
    // `compactHeight` (a phone on its side), wide from `expandedWidth` (a tablet
    // on its side, a desktop window: two columns). A tablet held upright is none
    // of the three. A screen asks its own width and height, so a test can give it
    // any size. Only a desktop window has a floor (`desktopMinWidth` x
    // `desktopMinHeight`); a phone, a tablet and a browser page are as big as
    // the screen is.
    readonly property real compactWidth: 600
    readonly property real expandedWidth: 900
    readonly property real compactHeight: 500
    readonly property real desktopMinWidth: 900
    readonly property real desktopMinHeight: 620
    function isNarrow(w) {
        return w < compactWidth;
    }
    function isWide(w) {
        return w >= expandedWidth;
    }
    function isShort(h) {
        return h < compactHeight;
    }

    // Pictures and panels.
    readonly property real pictureMax: 520
    readonly property real roomyPanel: 320
    readonly property real thumbInline: 96
    readonly property real thumbSheet: 112
    readonly property real thumbSheetCell: 136
    readonly property real sheetMaxWidth: 720
    readonly property real sheetMaxHeight: 560
    readonly property real modalMax: 560
    readonly property real contentWidth: 860
    // Home's column and the design's window (a screen larger than this is
    // zoomed to 1.5 at most and centred), the widest comfortable line of a
    // sentence, and a home card's picture: what it is given, the least it may
    // be given, and the tallest the card gets.
    readonly property real pageWidth: 1180
    readonly property real pageHeight: 740
    readonly property real measure: 720
    readonly property real cardArt: 240
    readonly property real cardArtMin: 160
    // Where the type is larger than the design's, the words need the room first: the picture gives way
    // down to this (none: a card shows its picture only from `cardArtCompact` up), and the card is not
    // pushed off the window.
    readonly property real cardArtLeast: 0
    // The picture's least height on a phone upright, where the card is a page of its own.
    readonly property real cardArtCompact: 96
    // A tall Solo window's least panel (the words and the actions) and least board, in dp.
    readonly property real minTallPanel: 300
    readonly property real minTallBoard: 180
    // The width of Solo's panel from which it puts the words beside the actions (a phone on its side).
    readonly property real twoColumnsWidth: 330
    // The height of Solo's box panel from which it shows the three steps under the sizes.
    readonly property real howItWorksRoom: 520
    readonly property real cardMax: 420
    readonly property real hintDisplay: 96
    readonly property real doneWidth: 160
    readonly property real startRing: 8
    // A board-size choice (3 x 3 ... 6 x 6) is at least this wide.
    readonly property real choiceMin: 64
    readonly property real choiceCell: 88

    // Icons, on the 24 x 24 grid scaled to these.
    readonly property real iconXs: 16
    readonly property real iconSm: 20
    readonly property real iconMd: 28
    readonly property real iconLg: 32
    readonly property real iconMax: iconLg

    // The letter on a die is this share of the tile (Qu: this share of that).
    readonly property real letterShare: 0.55
    readonly property real quShare: 0.78

    // The size of a jewel in the tray, by tier, and the sizes the rest of the
    // application draws it at.
    readonly property real jewelSpark: 26
    readonly property real jewelPower: 34
    readonly property real jewelMega: 44
    readonly property real jewelXs: 16
    readonly property real jewelSm: 24
    readonly property real jewelMd: 30
    readonly property real jewelXl: 72

    // The tray around the board (kit/BoardTray.qml): its rim is `heavy`, its
    // outset beyond the tiles is a share of the tile with a floor, and it must
    // fit inside the board area's inset (apps/app/src/solo/board_layout.cpp:
    // small_inset 12, roomy_inset 24), which it does only when `limit` is
    // passed - see trayOutset().
    readonly property real trayInsetMin: 10
    readonly property real trayInsetShare: 0.08
    readonly property real trayRim: outlineFrame
    readonly property real trayWellInset: 6
    function trayOutset(tile, limit) {
        const wanted = Math.max(trayInsetMin, tile * trayInsetShare);
        return limit === undefined ? wanted : Math.max(0, Math.min(wanted, limit - trayRim));
    }

    // --- motion, in ms -----------------------------------------------------
    readonly property int quick: ms(120, 0)
    readonly property int base: ms(200, 0)
    readonly property int screen: ms(280, 120)
    readonly property int turn: ms(380, 120)
    readonly property int restFade: ms(600, 0)
    readonly property int pathFade: 300
    readonly property int tileFlashStep: 60
    readonly property int tileFlashLength: 240
    readonly property int ringPulse: 500
    readonly property int glowRise: 600
    readonly property int glowFall: 1200
    readonly property real glowPeak: 0.28
    readonly property int wordHold: ms(450, 150)
    readonly property int clearedHold: 3000
    readonly property int holdToPlay: 1000

    function ms(full, reduced) {
        return reducedMotion ? reduced : full;
    }
}
