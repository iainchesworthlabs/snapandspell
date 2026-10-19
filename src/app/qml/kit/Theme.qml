pragma Singleton
import QtQuick
import WordJewelsSolver

// The one design system of the application: the Jewel Box. Warm paper, a deep
// tray, cream dice, an emerald action colour and a gold reward. Archivo is the
// house UI face, IBM Plex Mono the label face, Andika the learning face (dice
// letters and words). Every screen, the parent area included, reads these
// tokens; a screen writes no colour, size or type value of its own.
//
// Light and dark are chosen by the parent's Colour scheme setting, or by the
// system when that is System. The board is the one object that stays warm in
// both: its tray, dice and gold rim are the physical toy on the table.
QtObject {
    id: root

    // --- what the person or the system chooses ------------------------------
    property bool reducedMotion: false
    property real textScale: 1.0
    property string colourScheme: "system"
    property bool rightToLeft: false

    readonly property bool dark: colourScheme === "dark"
                                 || (colourScheme !== "light" && SystemColourScheme.dark)

    // --- colour: the chrome (light / dark) ----------------------------------
    readonly property color page: dark ? "#11181B" : "#F6F1E7"
    readonly property color surface: dark ? "#1A2428" : "#FFFDF8"
    readonly property color card: dark ? "#1E2A2F" : "#FFFDF8"
    readonly property color panel: dark ? "#1A2428" : "#EDE7DA"
    readonly property color boardArea: dark ? "#1A2428" : "#EFE9DA"
    readonly property color ink: dark ? "#F2ECDE" : "#1E2A30"
    readonly property color inkSecondary: dark ? "#B3BDBB" : "#5C6B70"
    readonly property color line: dark ? "#3A474B" : "#D9D1C2"
    readonly property color outline: dark ? "#8C9A98" : "#6E7A78"
    readonly property color accent: dark ? "#63C7B2" : "#166B5C"
    readonly property color accentSoft: dark ? "#1E3A36" : "#DDEDE8"
    readonly property color buttonFill: accent
    readonly property color buttonInk: dark ? "#0E1F1C" : "#FFFFFF"
    readonly property color gold: "#C98A1E"
    readonly property color goldInk: dark ? "#F0C879" : "#7A4E08"
    readonly property color goldSoft: dark ? "#3A2E1A" : "#F7E8C6"
    readonly property color hoverWash: dark ? "#243136" : "#EDE6D6"
    readonly property color pressWash: dark ? "#2E3D42" : "#E0D8C4"
    readonly property color danger: dark ? "#F08A78" : "#A33A2B"
    readonly property color focusRing: accent
    readonly property color scrim: Qt.rgba(0.067, 0.094, 0.106, 0.64)

    // The hint's own card. Gold is the reward and the hint, never the chrome.
    readonly property color hintCard: goldSoft
    readonly property color hintCardInk: ink

    // The resting screen is meant to be looked at without wanting anything.
    readonly property color restPage: dark ? "#161B22" : "#F1ECDF"
    readonly property color restShape: dark ? "#2B3542" : "#D9D2BF"
    readonly property color restDetail: dark ? "#3A4757" : "#C7BFA9"

    // --- colour: the board (the same warm object in both themes) ------------
    readonly property color tray: "#17323A"
    readonly property color trayEdge: gold
    readonly property color lid: "#F7E8C6"
    readonly property color well: "#0D2229"
    readonly property color die: "#FFF7E3"
    readonly property color dieInk: "#1F2A30"
    readonly property color dieEdge: "#20323A"
    readonly property color dieLit: "#F7E8C6"
    readonly property color dieLitInk: "#1F2A30"
    readonly property color pathCasing: gold
    readonly property color pathCore: "#17323A"
    readonly property color jewelDead: "#8FA1B8"

    // A jewel's colour is decoration: its size says its tier. These are taken
    // in turn by the order a word was found.
    readonly property var jewelColors: ["#E0556B", "#3F9E8F", "#7B6FD6", "#E39A2D", "#4B8BD3"]

    // --- type --------------------------------------------------------------
    readonly property string uiFont: "Archivo"
    readonly property string fontFamily: "Andika"
    readonly property string monoFont: "IBM Plex Mono"

    function rem(amount) {
        return amount * 16 * textScale;
    }

    readonly property real displaySize: rem(2.75)
    readonly property real titleSize: rem(1.875)
    readonly property real headingSize: rem(1.25)
    readonly property real labelSize: rem(1.125)
    readonly property real bodySize: rem(1.0)
    readonly property real wordSize: rem(1.5)
    readonly property real childLabelSize: rem(1.375)
    readonly property real smallSize: rem(0.875)
    readonly property real monoSize: rem(0.75)

    // --- sizes, in dp ------------------------------------------------------
    readonly property real targetMin: 48
    readonly property real targetChild: 64
    readonly property real targetPrimary: 96
    readonly property real targetGap: 8

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
    readonly property real pathCoreWidth: 5
    readonly property real pathCasingWidth: 9

    readonly property real radiusSmall: 8
    readonly property real radius: 12
    readonly property real radiusLarge: 20

    readonly property real appBarHeight: 72
    readonly property real barHeight: appBarHeight
    // Tall enough for the 96 dp Done target plus a little breathing room; the
    // old 88 clipped the bottom of the primary action.
    readonly property real actionBarHeight: 104

    // Pictures and panels.
    readonly property real pictureMax: 520
    readonly property real roomyPanel: 320
    readonly property real thumbInline: 96
    readonly property real thumbSheet: 112
    readonly property real thumbSheetCell: 136
    readonly property real sheetMaxWidth: 720
    readonly property real sheetMaxHeight: 560
    readonly property real hintDisplay: 96
    readonly property real doneWidth: 160
    readonly property real iconMax: 32
    readonly property real startRing: 8

    // The letter on a die is this share of the tile (Qu: this share of that).
    readonly property real letterShare: 0.55
    readonly property real quShare: 0.78

    // The size of a jewel in the tray, by tier.
    readonly property real jewelSpark: 26
    readonly property real jewelPower: 34
    readonly property real jewelMega: 44

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
