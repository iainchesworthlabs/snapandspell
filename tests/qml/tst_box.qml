import QtQuick
import QtTest
import WordJewelsSolver

// solo.box (src/app/qml/solo/SoloScreen.qml and SoloBox.qml): the board-size
// choice, the Shake button and Space. The screen is the application's own; the
// 3D board scene it imports is replaced by the harness's stub (tests/qml/
// scene_stub), which records the play(board, seed) a shake hands it and settles
// at once - no display and no Quick 3D in a test.
TestCase {
    id: testCase
    name: "Box"
    when: windowShown
    visible: true

    width: 1100
    height: 700

    Component { id: screenComponent; SoloScreen { } }
    Component { id: boxComponent; SoloBox { } }
    Component { id: layoutComponent; SoloLayout { } }

    // The device's motion without hardware: a MotionSource-shaped object the
    // screen can start and stop, whose sample and shake signals a suite emits.
    Component {
        id: motionComponent
        QtObject {
            property bool available: true
            property bool active: false
            property bool shakeEnabled: true
            signal sample(real ax, real ay, real az, real wx, real wy, real wz)
            signal shakeDetected()
        }
    }

    SignalSpy { id: homeSpy; signalName: "homeRequested" }

    function init() {
        // A QML warning in a test function fails it, as one outside does the
        // whole run: a screen that warned is a screen that is wrong.
        failOnWarning(/.?/)
        fixture.reset()
        Theme.rightToLeft = false
        Theme.reducedMotion = false
    }

    function cleanup() {
        Theme.rightToLeft = false
        Theme.reducedMotion = false
    }

    function allItems(root, found) {
        found = found || []
        found.push(root)
        const kids = root.children
        for (let i = 0; i < kids.length; ++i) {
            allItems(kids[i], found)
        }
        return found
    }

    function buttonNamed(root, name) {
        for (const item of allItems(root)) {
            if (item.accessibleName === name) {
                return item
            }
        }
        return null
    }

    function soloScreen() {
        const screen = createTemporaryObject(screenComponent, testCase,
                                             { width: 1100, height: 700, solo: soloModel })
        verify(screen !== null)
        return screen
    }

    function boxFor(width, height, wide) {
        const rule = createTemporaryObject(layoutComponent, testCase,
                                           { width: width, height: height, boardSize: 3 })
        const box = createTemporaryObject(boxComponent, testCase,
                                          { width: width, height: height, wide: wide,
                                            solo: soloModel, layoutRule: rule })
        verify(box !== null)
        return box
    }

    function sizeButtons(box) {
        return allItems(box).filter(item => typeof item.boardSize === "number"
                                             && item.checkable === true)
    }

    // --- the size choice ----------------------------------------------------
    function test_theSizesOfferedAreTheOnesThatFit() {
        // A window with room for the 3x3 and the 4x4 at 48 dp a tile: both are
        // offered, and the model's own list is what is offered from.
        const box = boxFor(1100, 700, false)
        compare(box.offered.length, 2)
        compare(box.offered[0], 3)
        compare(box.offered[1], 4)
        compare(sizeButtons(box).length, 2)

        // A small window where the 3x3's tile is 54 dp: only it is offered. The
        // 4x4's tile would be 39 dp, so it is not shown - not shown disabled.
        const small = boxFor(300, 300, false)
        compare(small.offered.length, 1)
        compare(small.offered[0], 3)
        compare(small.layoutRule.offers(4), false)
        compare(sizeButtons(small).length, 1)

        // A window too small for even the 3x3 at 48 dp: no size is offered, and
        // Shake is still there.
        const tiny = boxFor(200, 200, false)
        compare(tiny.offered.length, 0)
        compare(sizeButtons(tiny).length, 0)
        verify(buttonNamed(tiny, SoloWording.shake) !== null)
    }

    function test_theBoxPictureFollowsTheChosenSize() {
        const screen = soloScreen()
        const art = allItems(screen).find(item => item.closed === true
                                                  && item.boardSize !== undefined)
        verify(art !== null)
        compare(art.boardSize, 3)
        // The fake game offers 3 and 4; the picture follows the model, not a
        // hard-coded preview.
        soloModel.chooseSize(4)
        compare(art.boardSize, 4)
    }

    function test_aSizeIsChosenByPressingItsPicture() {
        const box = boxFor(1100, 700, false)
        const buttons = sizeButtons(box)
        compare(buttons.length, 2)
        // The size in play is shown pressed in (a state, not colour alone).
        compare(buttons[0].checked, true)
        compare(buttons[1].checked, false)

        mouseClick(buttons[1], buttons[1].width / 2, buttons[1].height / 2)

        compare(soloModel.boardSize, 4)
        compare(buttons[1].checked, true)
        compare(buttons[0].checked, false)
        // The choice is the box's: it does nothing once the dice are out.
        soloModel.shake()
        soloModel.chooseSize(3)
        compare(soloModel.boardSize, 4)
    }

    function test_theBoxIsTheShaperAndTheShakeButtonRunsWithIt() {
        // Wide: Shake across the panel with the sizes beneath it. Tall: the
        // sizes beside it, Shake taking the rest and at least a third wide.
        const wide = boxFor(400, 420, true)
        const wideShake = buttonNamed(wide, SoloWording.shake)
        verify(wideShake !== null)
        compare(wideShake.size, "primary")
        compare(wideShake.height, Theme.targetPrimary)
        compare(wideShake.width, wide.width - 2 * Theme.space6)

        const tall = boxFor(400, 420, false)
        const tallShake = buttonNamed(tall, SoloWording.shake)
        verify(tallShake.width >= tall.width / 3)
        verify(tallShake.height >= Theme.targetPrimary)
    }

    // --- the shake and the scene -------------------------------------------
    function test_shakeDealsABoardAndTheSceneIsGivenIt() {
        const screen = soloScreen()
        const scene = findChild(screen, "boardSceneStub")
        verify(scene !== null)
        // The box goes to the scene at start-up, with its lid on: no tiles.
        verify(scene.showCount >= 1)
        compare(scene.plays.length, 0)
        compare(soloModel.state, "box")

        const shake = buttonNamed(screen, SoloWording.shake)
        verify(shake !== null)
        mouseClick(shake, shake.width / 2, shake.height / 2)
        compare(soloModel.state, "shaking")

        // The screen asks for the board's faces a frame ahead, then plays it.
        tryCompare(scene, "playCount", 1)
        const played = scene.plays[0]
        compare(played.board.rows, 3)
        compare(played.board.columns, 3)
        compare(played.board.tiles.length, 9)
        compare(played.board.tiles[0], soloModel.tiles[0])
        compare(played.seed, soloModel.seed)
        // And the stub's settle moves the game on, as the real scene's would.
        tryCompare(soloModel, "state", "playing")
        compare(soloModel.rows, 3)
    }

    function test_spaceShakesTheBox() {
        const screen = soloScreen()
        const scene = findChild(screen, "boardSceneStub")
        verify(scene !== null)

        screen.forceActiveFocus()
        keyClick(Qt.Key_Space)

        compare(soloModel.state, "shaking")
        tryCompare(scene, "playCount", 1)
        tryCompare(soloModel, "state", "playing")
    }

    function test_aShakeInPlayDoesNothing() {
        const screen = soloScreen()
        const scene = findChild(screen, "boardSceneStub")
        verify(scene !== null)
        soloModel.shake()
        tryCompare(soloModel, "state", "playing")

        soloModel.shake()

        compare(soloModel.state, "playing")
        compare(scene.playCount, 1)
    }

    function test_theDevicesMotionDrivesTheSceneAndRollsTheBox() {
        // MotionSource's half of the Seam: while the box needs it, its samples
        // go to the scene's pushMotion, and a detected shake rolls the box as
        // the Shake button does. No sensor: a QML object with the same shape.
        const motion = createTemporaryObject(motionComponent, testCase)
        const screen = createTemporaryObject(screenComponent, testCase,
                                             { width: 1100, height: 700, solo: soloModel,
                                               motionSource: motion })
        verify(screen !== null)
        const scene = findChild(screen, "boardSceneStub")
        verify(scene !== null)

        // The box is on screen and the parent allows shaking: the source is
        // running and its own shake setting follows the screen's.
        tryCompare(motion, "active", true)
        compare(motion.shakeEnabled, true)
        compare(scene.motions.length, 0)

        motion.sample(0.1, -0.2, 0.9, 0.01, 0.02, 0.03)
        compare(scene.motions.length, 1)
        compare(scene.motions[0].ax, 0.1)
        compare(scene.motions[0].az, 0.9)
        compare(scene.motions[0].wz, 0.03)

        motion.shakeDetected()
        compare(soloModel.state, "shaking")
        tryCompare(scene, "playCount", 1)
        tryCompare(soloModel, "state", "playing")

        // Play is on: the box no longer needs the sensors, so they stop.
        tryCompare(motion, "active", false)
    }

    function test_theShakeSettingStopsTheSensorsAndTheRoll() {
        // The parent's Shake row off: the source is told, never started, and a
        // shake that arrives anyway does not roll the box. The Shake button
        // and Space are unaffected (test_spaceShakesTheBox).
        const motion = createTemporaryObject(motionComponent, testCase)
        const screen = createTemporaryObject(screenComponent, testCase,
                                             { width: 1100, height: 700, solo: soloModel,
                                               motionSource: motion, shakeEnabled: false })
        verify(screen !== null)

        compare(motion.shakeEnabled, false)
        compare(motion.active, false)
        motion.shakeDetected()
        compare(soloModel.state, "box")
    }

    function test_theLidGoesBackOnWithTheWayBack() {
        // The bar's way back from solo.box asks to leave for home; the screen
        // says so, and the shell takes it home (the suites do not show home).
        const screen = soloScreen()
        homeSpy.target = screen
        homeSpy.clear()

        const back = buttonNamed(screen, SoloWording.back)
        verify(back !== null)
        mouseClick(back, back.width / 2, back.height / 2)

        compare(homeSpy.count, 1)
    }
}
