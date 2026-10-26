// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import SnapAndSpell

// The kit's tokens (apps/app/qml/kit/Theme.qml): the values the design's placeholders
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

    // Toybox Pixel (docs/design/toybox/DESIGN.md, section 5). The named palette is tied to tokens.json by
    // tests/design/check_tokens.cmake; these are the roles it plays in each scheme.
    function test_theLightTokens() {
        Theme.colourScheme = "light"

        compare(Theme.dark, false)
        compare(Theme.page.toString(), "#ffc93c")
        compare(Theme.ink.toString(), "#2b1d52")
        compare(Theme.inkSecondary.toString(), "#4a3a7c")
        compare(Theme.outline.toString(), "#2b1d52")
        compare(Theme.buttonFill.toString(), "#2b1d52")
        compare(Theme.buttonInk.toString(), "#fff1d6")
        compare(Theme.hoverWash.toString(), "#ffe7b8")
        compare(Theme.pressWash.toString(), "#ffda8f")
        compare(Theme.boardArea.toString(), "#ffc93c")
        compare(Theme.panel.toString(), "#fff1d6")
        compare(Theme.card.toString(), "#fff1d6")
        compare(Theme.surfaceDim.toString(), "#ebd9b4")
        compare(Theme.shadow.toString(), "#1a1035")
        compare(Theme.accentText.toString(), "#2b1d52")
        compare(Theme.selectedFill.toString(), "#ff6b4a")
    }

    // Toybox Night: a deep-purple page, night-purple panels, cream text and outlines, a sunshine primary
    // button with ink text and coral selection with ink text.
    function test_theDarkTokens() {
        Theme.colourScheme = "dark"

        compare(Theme.dark, true)
        compare(Theme.page.toString(), "#1a1035")
        compare(Theme.ink.toString(), "#fff1d6")
        compare(Theme.inkSecondary.toString(), "#c4b7dd")
        compare(Theme.outline.toString(), "#fff1d6")
        compare(Theme.buttonFill.toString(), "#ffc93c")
        compare(Theme.buttonInk.toString(), "#2b1d52")
        compare(Theme.hoverWash.toString(), "#3a2b66")
        compare(Theme.pressWash.toString(), "#473577")
        compare(Theme.boardArea.toString(), "#1a1035")
        compare(Theme.panel.toString(), "#2b1d52")
        compare(Theme.card.toString(), "#2b1d52")
        compare(Theme.surfaceDim.toString(), "#3a2b66")
        compare(Theme.shadow.toString(), "#0b0618")
        compare(Theme.accentText.toString(), "#ffc93c")
        compare(Theme.selectedFill.toString(), "#ff6b4a")
        compare(Theme.selectedInk.toString(), "#2b1d52")
    }

    // Text never sits on pink or blueberry, and a coral or sunshine object on the page carries an ink
    // outline: the roles that would break that rule must not exist as text colours.
    function test_noTextRoleIsCoralPinkOrBlueberry() {
        for (const scheme of ["light", "dark"]) {
            Theme.colourScheme = scheme
            for (const name of ["ink", "inkSecondary", "accentText", "buttonInk", "selectedInk", "dangerInk",
                                "goldInk", "chipInk", "hintCardInk", "dieInk", "dieLitInk", "disabledInk"]) {
                const colour = Theme[name].toString()
                verify(colour !== Theme.coral.toString() && colour !== Theme.pink.toString()
                       && colour !== Theme.blueberry.toString(), scheme + " " + name + " " + colour)
            }
        }
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
        compare(Theme.headingSize, 24.0)
        compare(Theme.bodySize, 18.0)
        compare(Theme.wordSize, 24.0)
        compare(Theme.smallSize, 14.0)
        compare(Theme.rem(1.0), 16.0)

        Theme.textScale = 1.25
        compare(Theme.headingSize, 30.0)
        compare(Theme.wordSize, 30.0)
        compare(Theme.smallSize, 17.5)

        Theme.textScale = 1.5
        compare(Theme.headingSize, 36.0)
        compare(Theme.wordSize, 36.0)
        compare(Theme.smallSize, 21.0)
    }

    // The Toybox scale (14, 16, 18, 20, 24, 26, 34, 36, 40, 56) is the whole of the type: every size
    // token is one of its steps at the default text size.
    function test_everyTypeSizeIsAStepOfTheScale() {
        Theme.textScale = 1.0
        const sizes = {
            monoSize: Theme.monoSize, microSize: Theme.microSize, smallSize: Theme.smallSize,
            captionSize: Theme.captionSize, bodySize: Theme.bodySize, labelSize: Theme.labelSize,
            headingSize: Theme.headingSize, wordSize: Theme.wordSize, childLabelSize: Theme.childLabelSize,
            titleSize: Theme.titleSize, displaySize: Theme.displaySize, numeralSize: Theme.numeralSize,
            wordmarkSize: Theme.wordmarkSize
        }
        for (const name in sizes) {
            verify(Theme.typeScale.indexOf(sizes[name]) >= 0, name + " " + sizes[name] + " is a step of the scale")
        }
        compare(Theme.typeScale.length, 10)
        compare(Theme.typeScale[0], 14)
        compare(Theme.typeScale[9], 56)
    }

    // Nothing is set below 14 (Toybox's caption), whatever the text scale.
    function test_noTypeIsSmallerThanFourteen() {
        Theme.textScale = 0.85
        verify(Theme.monoSize >= 14)
        verify(Theme.smallSize >= 14)
        verify(Theme.rem(0.5) >= 14)
        Theme.textScale = 1.0
        compare(Theme.minTextSize, 14)
    }

    function test_theLineHeightsAreTheBriefs() {
        verify(Theme.lineBody >= 1.4 && Theme.lineBody <= 1.5)
        verify(Theme.lineDisplay >= 1.15 && Theme.lineDisplay <= 1.25)
        verify(Theme.lineCaption >= 1.3 && Theme.lineCaption <= 1.5)
        compare(Theme.trackMono, 0.08)
        compare(Theme.trackDisplay, 0)
    }

    // The tray outset never exceeds the board area's inset minus the rim when it
    // is given that inset (apps/app/src/solo/board_layout.cpp: 12 small, 24 roomy).
    function test_theTrayFitsInsideTheBoardAreasInset() {
        compare(Theme.trayOutset(100), Math.max(Theme.trayInsetMin, 100 * Theme.trayInsetShare))
        compare(Theme.trayOutset(300), 24)
        compare(Theme.trayOutset(300, 12), 12 - Theme.trayRim)
        // The 6 px outline leaves 6 of the inset, less than the least outset: the inset wins.
        compare(Theme.trayOutset(60, 12), 12 - Theme.trayRim)
        compare(Theme.trayOutset(400, 24), 24 - Theme.trayRim)
    }

    // The selected, disabled and danger states are tokens, so no screen reaches
    // for gold or opacity.
    function test_theStateTokensExistInBothSchemes() {
        for (const scheme of ["light", "dark"]) {
            Theme.colourScheme = scheme
            for (const name of ["selectedFill", "selectedEdge", "selectedInk", "disabledFill", "disabledInk",
                                "disabledEdge", "danger", "dangerFill", "dangerInk", "track", "focusRing"]) {
                verify(Theme[name] !== undefined, scheme + " " + name)
            }
            verify(Theme.selectedEdge.toString() !== Theme.gold.toString(), "selection is not gold")
        }
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

    // Toybox's shape (DESIGN.md, sections 3 and 6; tests/design/check_tokens.cmake ties these to tokens.json).
    function test_theShapeTokensAreToyboxs() {
        compare(Theme.outlineControl, 3)
        compare(Theme.outlinePanel, 4)
        compare(Theme.outlineFrame, 6)
        compare(Theme.shadowSmall, 3)
        compare(Theme.shadowMedium, 4)
        compare(Theme.shadowLarge, 6)
        compare(Theme.shadowFrame, 12)
        compare(Theme.pressDepth, 4)
        compare(Theme.focusWidth, 4)
        compare(Theme.focusGap, 3)
        compare(Theme.radiusXs, 4)
        compare(Theme.radiusSmall, 8)
        compare(Theme.radiusControl, 10)
        compare(Theme.radius, 14)
        compare(Theme.radiusPanel, 18)
        compare(Theme.radiusLarge, 20)
    }

    function test_theJewelSizesSayTheirTier() {
        compare(Theme.jewelSpark, 26)
        compare(Theme.jewelPower, 34)
        compare(Theme.jewelMega, 44)
        compare(Theme.jewelColors.length, 5)
    }
}
