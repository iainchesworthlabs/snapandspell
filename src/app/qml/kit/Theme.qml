pragma Singleton
import QtQuick
import WordJewelsSolver

// Every colour, size, time and type size a screen uses is a token here; a screen
// writes none of its own. The values are the placeholders of
// docs/design/application.md ("Colour and contrast", "Touch targets", "Motion",
// "Type"), which are the gate a final palette has to meet. A school font, a
// world (7.2) or a dark theme is a swap of these tokens and changes no rule.
//
// The board is dark in both themes, so the dice read the same; only the chrome
// around it follows the system's light or dark setting.
QtObject {
    id: root

    // --- what the person or the system chooses ------------------------------
    // Set by the parent area's Motion setting once it exists: System follows the
    // operating system, Reduced forces it. Nothing here is a child's choice.
    property bool reducedMotion: false
    // The system's text size times the app's own (1, 1.25 or 1.5), clamped to
    // 0.85 to 2.0.
    property real textScale: 1.0

    readonly property bool dark: SystemColourScheme.dark

    // --- colour: the chrome (light / dark) ----------------------------------
    readonly property color page: dark ? "#12161C" : "#FBF8F1"
    readonly property color ink: dark ? "#F4EFE3" : "#1F2A37"
    readonly property color inkSecondary: dark ? "#B4BDC9" : "#4A5666"
    readonly property color outline: dark ? "#8C98A8" : "#5B6879"
    readonly property color buttonFill: dark ? "#7DB4EE" : "#1F5FA8"
    readonly property color buttonInk: dark ? "#0E1620" : "#FFFFFF"
    readonly property color hoverWash: dark ? "#27303B" : "#EDE6D6"
    readonly property color pressWash: dark ? "#323D4A" : "#E0D8C4"
    readonly property color boardArea: dark ? "#1A1F26" : "#EFE9DA"
    readonly property color panel: dark ? "#1D2733" : "#DCE6F2"
    readonly property color card: dark ? "#263140" : "#FFFFFF"
    readonly property color hintCard: "#FFD54F"
    readonly property color hintCardInk: "#1F2A37"
    readonly property color scrim: Qt.rgba(0.07, 0.09, 0.11, 0.6)

    // The resting screen is meant to be looked at without wanting anything: low
    // contrast, nothing to chase.
    readonly property color restPage: dark ? "#161B22" : "#F1ECDF"
    readonly property color restShape: dark ? "#2B3542" : "#D9D2BF"
    readonly property color restDetail: dark ? "#3A4757" : "#C7BFA9"

    // --- colour: the board (the same in both themes) ---------------------------
    readonly property color tray: "#1F2A37"
    readonly property color trayEdge: dark ? "#8C98A8" : "#1F2A37"
    readonly property color lid: "#DCE6F2"
    readonly property color well: "#41536B"
    readonly property color die: "#FFF8E1"
    readonly property color dieInk: "#1F2A37"
    readonly property color dieEdge: "#1F2A37"
    readonly property color dieLit: "#FFD54F"
    readonly property color dieLitInk: "#1F2A37"
    readonly property color pathCasing: "#FFFFFF"
    readonly property color pathCore: "#1F2A37"
    readonly property color jewelDead: "#8FA1B8"

    // A jewel's colour is decoration: its size says its tier (game.md). These are
    // taken in turn by the order a word was found.
    readonly property var jewelColors: ["#E0556B", "#3F9E8F", "#7B6FD6", "#E39A2D", "#4B8BD3"]

    // --- sizes, in dp ----------------------------------------------------------
    // Touch targets (application.md, "Touch targets").
    readonly property real targetMin: 48       // anything an adult touches
    readonly property real targetChild: 64     // anything a child taps that is not primary
    readonly property real targetPrimary: 96   // Shake, and the like: this high, at least a third wide
    readonly property real targetGap: 8        // between neighbouring child targets

    readonly property real space1: 4
    readonly property real space2: 8
    readonly property real space3: 12
    readonly property real space4: 16
    readonly property real space6: 24
    readonly property real space8: 32
    readonly property real space12: 48
    readonly property real space16: 64

    readonly property real hair: 1
    readonly property real rule: 2
    readonly property real heavy: 3
    readonly property real litEdge: 4          // a lit tile's edge
    readonly property real litScale: 1.06      // a lit tile's scale
    readonly property real pathCoreWidth: 6
    readonly property real pathCasingWidth: 12

    // Pictures and panels (a screen takes its sizes from here).
    readonly property real pictureMax: 480     // the largest side of a picture on home
    readonly property real roomyPanel: 300     // a panel this big on both sides has room for more
    readonly property real thumbInline: 96     // a history thumbnail in the panel
    readonly property real thumbSheet: 112     // a history thumbnail in the sheet
    readonly property real thumbSheetCell: 136
    readonly property real sheetMaxWidth: 720
    readonly property real sheetMaxHeight: 560
    readonly property real hintDisplay: 96     // the hint's picture of the board
    readonly property real doneWidth: 160      // the Done jewel's button
    readonly property real iconMax: 48
    readonly property real startRing: 6        // the start tile's ring, outside its tile

    // The bar above every Solo Play screen holds the way back and the quiet corner
    // buttons, so neither lies over the board.
    readonly property real barHeight: targetChild + space2

    // The letter on a die is this share of the tile (Qu: this share of that).
    readonly property real letterShare: 0.55
    readonly property real quShare: 0.8

    // The size of a jewel in the tray, by tier.
    readonly property real jewelSpark: 28
    readonly property real jewelPower: 38
    readonly property real jewelMega: 50

    // --- motion, in ms (application.md, "Motion") ----------------------------------
    readonly property int quick: ms(120, 0)
    readonly property int base: ms(200, 0)
    readonly property int screen: ms(280, 120)
    readonly property int turn: ms(380, 120)
    readonly property int restFade: ms(600, 0)        // a cut when reduced
    readonly property int pathFade: 300               // a path that is not a word
    readonly property int tileFlashStep: 60           // the found word's tiles, in order
    readonly property int tileFlashLength: 240        // each of them stays lit this long
    readonly property int ringPulse: 500              // the start tile's ring, five pulses (under 5 s)
    readonly property int glowRise: 600               // the one slow glow when the board clears
    readonly property int glowFall: 1200
    readonly property real glowPeak: 0.35
    readonly property int wordHold: ms(450, 150)      // the word stays lit this long
    readonly property int clearedHold: 3000           // solo.cleared lasts about this long
    readonly property int holdToPlay: 1000            // the resting screen's Again

    function ms(full, reduced) {
        return reducedMotion ? reduced : full;
    }

    // --- type ----------------------------------------------------------------------
    // Andika (SIL OFL) is the recommendation for beginning readers. It is not
    // bundled: its licence file has to be read first and the glyph check of
    // docs/design/application.md ("Type") done on a device. Until then Qt falls
    // back to the platform's face.
    readonly property string fontFamily: "Andika"

    // Sizes are in rem: 1 rem is 16 px times the text scale.
    function rem(amount) {
        return amount * 16 * textScale;
    }
    readonly property real labelSize: rem(1.0)
    readonly property real bodySize: rem(1.0)
    readonly property real wordSize: rem(1.5)         // a word in the tray
    readonly property real smallSize: rem(0.875)
}
