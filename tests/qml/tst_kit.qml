import QtQuick
import QtTest
import WordJewelsSolver

// The kit's components (src/app/qml/kit/) as they are used, not as they are
// drawn: AppButton's three target sizes and its release-inside rule, HoldButton's
// hold, TileFace's two states, the Jewel's tier sizes, the Icon's pictures and
// the Feedback seam's cues. Nothing here reads a pixel: a state is a property,
// which is what a screen binds to.
TestCase {
    id: testCase
    name: "Kit"
    when: windowShown
    // The test window is what the kit is drawn in and where pointer events land:
    // Quick Test's own root is invisible until a suite says otherwise.
    visible: true

    width: 800
    height: 600

    Component { id: buttonComponent; AppButton { } }
    Component { id: holdComponent; HoldButton { } }
    Component { id: tileComponent; TileFace { } }
    Component { id: jewelComponent; Jewel { } }
    Component { id: iconComponent; Icon { } }

    SignalSpy { id: clickedSpy; signalName: "clicked" }
    SignalSpy { id: activatedSpy; signalName: "activated" }
    SignalSpy { id: cueSpy; target: Feedback; signalName: "cue" }

    function init() {
        // A QML warning in a test function fails it, as one outside does the
        // whole run: a screen that warned is a screen that is wrong.
        failOnWarning(/.?/)
        Theme.colourScheme = "system"
        Theme.textScale = 1.0
        Theme.reducedMotion = false
        Theme.rightToLeft = false
        cueSpy.clear()
    }

    function cleanup() {
        init()
    }

    // --- AppButton ---------------------------------------------------------
    function test_theThreeTargetsAre48_64_96() {
        const min = createTemporaryObject(buttonComponent, testCase, { size: "min" })
        verify(min !== null)
        compare(min.targetHeight, 48)
        compare(min.implicitHeight, Theme.targetMin)
        verify(min.implicitWidth >= min.implicitHeight)

        const child = createTemporaryObject(buttonComponent, testCase, { size: "child" })
        compare(child.targetHeight, 64)
        compare(child.implicitHeight, Theme.targetChild)

        const primary = createTemporaryObject(buttonComponent, testCase, { size: "primary" })
        compare(primary.targetHeight, 96)
        compare(primary.implicitHeight, Theme.targetPrimary)
        // At least a third of the width: the screen that uses it says the width,
        // and the button keeps its target height.
        primary.minimumWidth = 400
        compare(primary.implicitWidth, 400)
    }

    function test_aReleaseInsideIsAClick() {
        const button = createTemporaryObject(buttonComponent, testCase,
                                             { width: 200, height: 64, accessibleName: "Shake" })
        clickedSpy.target = button
        clickedSpy.clear()

        mouseClick(button, 100, 32)

        compare(clickedSpy.count, 1)
        compare(button.pressed, false)
    }

    function test_slidingOffBeforeTheReleaseCancels() {
        const button = createTemporaryObject(buttonComponent, testCase,
                                             { width: 200, height: 64 })
        clickedSpy.target = button
        clickedSpy.clear()

        mousePress(button, 20, 20)
        verify(button.pressed)
        mouseMove(button, button.width + 60, 20, 0, Qt.LeftButton)
        compare(button.pressed, false)
        mouseRelease(button, button.width + 60, 20)

        compare(clickedSpy.count, 0)
    }

    function test_aPressSpeaksThePicturesName() {
        const button = createTemporaryObject(buttonComponent, testCase,
                                             { width: 64, height: 64, speaks: "solo" })
        mousePress(button, 32, 32)
        compare(cueSpy.count, 1)
        compare(cueSpy.signalArguments[0][0], "speak")
        compare(cueSpy.signalArguments[0][1], "solo")
        mouseRelease(button, 32, 32)

        // A button with no name is silent: the cue is a press's, not a click's.
        // The click itself still plays the quiet tap of a control.
        const silent = createTemporaryObject(buttonComponent, testCase,
                                             { width: 64, height: 64 })
        cueSpy.clear()
        mouseClick(silent, 32, 32)
        compare(cueSpy.count, 1)
        compare(cueSpy.signalArguments[0][0], "tap")
    }

    function test_spaceActsAsAReleaseInside() {
        const button = createTemporaryObject(buttonComponent, testCase,
                                             { width: 64, height: 64 })
        clickedSpy.target = button
        clickedSpy.clear()
        button.forceActiveFocus()
        tryCompare(button, "activeFocus", true)

        keyClick(Qt.Key_Space)

        compare(clickedSpy.count, 1)
    }

    function test_aDisabledButtonDoesNothing() {
        const button = createTemporaryObject(buttonComponent, testCase,
                                             { width: 200, height: 64, enabled: false })
        clickedSpy.target = button
        clickedSpy.clear()

        mouseClick(button, 100, 32)

        compare(clickedSpy.count, 0)
        compare(button.opacity, 0.45)
    }

    // --- HoldButton --------------------------------------------------------
    function test_aHoldFillsTheRingAndThenActs() {
        const hold = createTemporaryObject(holdComponent, testCase,
                                           { width: 220, height: 64, holdMs: 200 })
        activatedSpy.target = hold
        activatedSpy.clear()
        compare(hold.progress, 0)

        mousePress(hold, 110, 32)

        // Part way: the ring is filling but has not finished, and nothing has
        // happened yet. Nothing is lost by letting go here.
        tryVerify(() => hold.progress > 0)
        tryVerify(() => hold.progress < 1)
        verify(hold.holding)
        compare(activatedSpy.count, 0)

        tryCompare(activatedSpy, "count", 1, 5000)
        compare(hold.progress, 0)
        compare(hold.holding, false)
        mouseRelease(hold, 110, 32)
    }

    function test_lettingGoEarlyStartsTheRingOver() {
        const hold = createTemporaryObject(holdComponent, testCase,
                                           { width: 220, height: 64, holdMs: 300 })
        activatedSpy.target = hold
        activatedSpy.clear()

        mousePress(hold, 110, 32)
        tryVerify(() => hold.progress > 0)
        mouseRelease(hold, 110, 32)

        compare(hold.progress, 0)
        compare(hold.holding, false)
        compare(activatedSpy.count, 0)
    }

    function test_theHoldIsAlsoASwitchAction() {
        // A screen reader or a switch activates it without holding (the
        // accessibility action), which is the same signal.
        const hold = createTemporaryObject(holdComponent, testCase,
                                           { width: 220, height: 64 })
        activatedSpy.target = hold
        activatedSpy.clear()

        hold.activated()

        compare(activatedSpy.count, 1)
    }

    // --- TileFace ----------------------------------------------------------
    function test_tileFaceAtRestAndLit() {
        const face = createTemporaryObject(tileComponent, testCase,
                                           { width: 100, height: 100, letters: "A" })
        const faceRect = face.children.find(child => child.objectName === "face")
        const letter = face.children.find(child => child.objectName === "letter")
        verify(faceRect !== undefined)
        verify(letter !== undefined)
        compare(face.shown, "A")
        compare(face.scale, 1)
        compare(faceRect.border.width, Theme.rule)
        compare(faceRect.color.toString(), Theme.die.toString())
        compare(letter.text, "A")
        compare(letter.color.toString(), Theme.dieInk.toString())

        face.lit = true

        // Lit is a shape as well as a colour: the edge turns gold and thickens.
        // The tile keeps its size so the letter over the path cannot drift with
        // the face's scale; the face and its letter stay readable.
        compare(face.scale, 1)
        compare(faceRect.border.width, Theme.litEdge)
        compare(faceRect.border.color.toString(), Theme.gold.toString())
        compare(faceRect.color.toString(), Theme.die.toString())
        compare(letter.color.toString(), Theme.dieInk.toString())
    }

    function test_theQuTileShowsTwoLettersOnOneFace() {
        const face = createTemporaryObject(tileComponent, testCase,
                                           { width: 100, height: 100, letters: "QU" })
        compare(face.shown, "Qu")
        face.letters = "Z"
        compare(face.shown, "Z")

        // The Qu face draws smaller so that both letters fit.
        const qu = createTemporaryObject(tileComponent, testCase,
                                         { width: 100, height: 100, letters: "QU" })
        const z = createTemporaryObject(tileComponent, testCase,
                                        { width: 100, height: 100, letters: "Z" })
        const quLetter = qu.children.find(child => child.objectName === "letter")
        const zLetter = z.children.find(child => child.objectName === "letter")
        verify(quLetter.font.pixelSize < zLetter.font.pixelSize)
    }

    // --- Jewel --------------------------------------------------------------
    function test_theJewelsSizeSaysItsTier() {
        const jewel = createTemporaryObject(jewelComponent, testCase)
        compare(jewel.tier, "spark")
        compare(jewel.diameter, Theme.jewelSpark)
        compare(jewel.width, Theme.jewelSpark)

        jewel.tier = "power"
        compare(jewel.diameter, Theme.jewelPower)
        jewel.tier = "mega"
        compare(jewel.diameter, Theme.jewelMega)

        // The colour is decoration and comes round the list, so nothing reads a
        // tier from it: the same tier in another place is another colour.
        jewel.colorIndex = 2
        compare(jewel.color.toString().toLowerCase(), String(Theme.jewelColors[2]).toLowerCase())
        jewel.colorIndex = 7  // the list is taken in turn
        compare(jewel.color.toString().toLowerCase(), String(Theme.jewelColors[2]).toLowerCase())
    }

    // --- Icon ---------------------------------------------------------------
    function test_theIconsPictures() {
        const icon = createTemporaryObject(iconComponent, testCase, { name: "back" })
        compare(icon.implicitWidth, 24)
        compare(icon.implicitHeight, 24)
        verify(icon.pathFor("back") !== "")
        verify(icon.mirrorsInRtl)

        // The way back is the one picture that turns round with the layout.
        for (const name of ["close", "parent", "history", "hint", "done", "again", "shake"]) {
            icon.name = name
            verify(icon.pathFor(name) !== "", name + " has a picture")
            verify(!icon.mirrorsInRtl, name + " does not turn round")
        }

        icon.name = "there-is-no-such-picture"
        compare(icon.pathFor(icon.name), "")
    }

    // --- Feedback ------------------------------------------------------------
    function test_theFeedbackSeamCarriesCues() {
        // Every moment of the design's table brings a cue; nothing plays them
        // yet, so the seam is all there is to hold down.
        Feedback.cue("rattle", "")
        Feedback.cue("tile", "A")
        Feedback.cue("word", "sat")
        compare(cueSpy.count, 3)
        compare(cueSpy.signalArguments[0][0], "rattle")
        compare(cueSpy.signalArguments[1][0], "tile")
        compare(cueSpy.signalArguments[1][1], "A")
        compare(cueSpy.signalArguments[2][0], "word")
        compare(cueSpy.signalArguments[2][1], "sat")
    }
}
