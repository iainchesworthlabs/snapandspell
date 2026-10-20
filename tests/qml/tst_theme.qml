import QtQuick
import QtTest
import WordJewelsSolver

// The kit's tokens (src/app/qml/kit/Theme.qml): the values the design's placeholders
// pin, the three things the person or the system chooses, and the two the
// application multiplies or cuts - text scale and reduced motion. A screen writes
// no token of its own, so these are the whole of what a screen's colour and time
// come to.
TestCase {
    id: testCase
    name: "Theme"
    when: windowShown

    function init() {
        // A QML warning in a test function fails it, as one outside does the
        // whole run: a screen that warned is a screen that is wrong.
        failOnWarning(/.?/)
        // The two settings a test changes are the window's to set (Main.qml
        // binds them from the parent area's controller), so each starts here.
        Theme.colourScheme = "system"
        Theme.textScale = 1.0
        Theme.reducedMotion = false
        Theme.rightToLeft = false
    }

    function cleanup() {
        init()
    }

    function test_theLightTokens() {
        Theme.colourScheme = "light"

        compare(Theme.dark, false)
        compare(Theme.page.toString(), "#f6f1e7")
        compare(Theme.ink.toString(), "#1e2a30")
        compare(Theme.inkSecondary.toString(), "#5c6b70")
        compare(Theme.outline.toString(), "#6e7a78")
        compare(Theme.buttonFill.toString(), "#166b5c")
        compare(Theme.buttonInk.toString(), "#ffffff")
        compare(Theme.hoverWash.toString(), "#ede6d6")
        compare(Theme.pressWash.toString(), "#e0d8c4")
        compare(Theme.boardArea.toString(), "#efe9da")
        compare(Theme.panel.toString(), "#ede7da")
        compare(Theme.card.toString(), "#fffdf8")
    }

    function test_theDarkTokens() {
        Theme.colourScheme = "dark"

        compare(Theme.dark, true)
        compare(Theme.page.toString(), "#11181b")
        compare(Theme.ink.toString(), "#f2ecde")
        compare(Theme.inkSecondary.toString(), "#b3bdbb")
        compare(Theme.outline.toString(), "#8c9a98")
        compare(Theme.buttonFill.toString(), "#63c7b2")
        compare(Theme.buttonInk.toString(), "#0e1f1c")
        compare(Theme.hoverWash.toString(), "#243136")
        compare(Theme.pressWash.toString(), "#2e3d42")
        compare(Theme.boardArea.toString(), "#1a2428")
        compare(Theme.panel.toString(), "#1a2428")
        compare(Theme.card.toString(), "#1e2a2f")
    }

    function test_theBoardIsTheSameInBothThemes() {
        // The dice read the same whatever the chrome does, so these never flip.
        Theme.colourScheme = "light"
        const tray = Theme.tray.toString()
        const die = Theme.die.toString()
        const dieInk = Theme.dieInk.toString()
        const lit = Theme.dieLit.toString()
        const dead = Theme.jewelDead.toString()

        Theme.colourScheme = "dark"
        compare(Theme.tray.toString(), tray)
        compare(Theme.die.toString(), die)
        compare(Theme.dieInk.toString(), dieInk)
        compare(Theme.dieLit.toString(), lit)
        compare(Theme.jewelDead.toString(), dead)
    }

    function test_theColourSchemeTokenFlipsDark() {
        // System is the operating system's answer, whatever this machine's is,
        // so the comparison is against the live wrapper rather than a constant.
        Theme.colourScheme = "system"
        compare(Theme.dark, SystemColourScheme.dark)

        Theme.colourScheme = "light"
        compare(Theme.dark, false)
        Theme.colourScheme = "dark"
        compare(Theme.dark, true)
    }

    function test_theTextScaleMultipliesTheType() {
        Theme.textScale = 1.0
        compare(Theme.labelSize, 18.0)
        compare(Theme.bodySize, 16.0)
        compare(Theme.wordSize, 24.0)
        compare(Theme.smallSize, 14.0)
        compare(Theme.rem(1.0), 16.0)

        Theme.textScale = 1.25
        compare(Theme.labelSize, 22.5)
        compare(Theme.wordSize, 30.0)
        compare(Theme.smallSize, 17.5)

        Theme.textScale = 1.5
        compare(Theme.labelSize, 27.0)
        compare(Theme.wordSize, 36.0)
        compare(Theme.smallSize, 21.0)
    }

    function test_reducedMotionRemovesTheAnimationDurations() {
        Theme.reducedMotion = false
        compare(Theme.quick, 120)
        compare(Theme.base, 200)
        compare(Theme.screen, 280)
        compare(Theme.turn, 380)
        compare(Theme.restFade, 600)
        compare(Theme.wordHold, 450)
        compare(Theme.ms(200, 0), 200)
        compare(Theme.ms(280, 120), 280)

        Theme.reducedMotion = true
        // A cut where the value is 0; the shorter cross-fade where the design
        // keeps one (the screen's and the turn's 120 ms).
        compare(Theme.quick, 0)
        compare(Theme.base, 0)
        compare(Theme.screen, 120)
        compare(Theme.turn, 120)
        compare(Theme.restFade, 0)
        compare(Theme.wordHold, 150)
        compare(Theme.ms(200, 0), 0)
        compare(Theme.ms(280, 120), 120)
    }

    function test_theTargetTokensAreTheDesigns() {
        compare(Theme.targetMin, 48)
        compare(Theme.targetChild, 64)
        compare(Theme.targetPrimary, 96)
        compare(Theme.targetGap, 8)
        compare(Theme.rule, 1)
        compare(Theme.heavy, 2)
        compare(Theme.litEdge, 3)
        compare(Theme.litScale, 1.03)
        compare(Theme.barHeight, Theme.appBarHeight)
    }

    function test_theJewelSizesSayTheirTier() {
        compare(Theme.jewelSpark, 26)
        compare(Theme.jewelPower, 34)
        compare(Theme.jewelMega, 44)
        compare(Theme.jewelColors.length, 5)
    }
}
