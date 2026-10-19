import QtQuick
import QtTest
import WordJewelsSolver
import WordJewelsSolver.Scan

// Mode A's way in (src/app/qml/scan/ScanScreen.qml and CheckScreen.qml): the
// ready screen with no camera and the retry that looks for one, the live view a
// running camera source shows, the typed board, a correction through the letter
// picker, the camera-only Turn, the secondary action ("Start over" vs "Scan
// again"), the stepper, the frame's corner brackets, and "Start playing". The
// models are the application's ScanModel and AssistModel; a live camera is never
// started, because on this tree's desktops there is none and the suites are
// headless. The camera path uses the fixture's fake source, which is a real
// CameraSource over a backend that returns no frames.
TestCase {
    id: testCase
    name: "Scan"
    when: windowShown
    visible: true

    width: 900
    height: 700

    Component { id: screenComponent; ScanScreen { } }

    SignalSpy { id: playSpy; signalName: "playRequested" }
    SignalSpy { id: closedSpy; signalName: "closed" }

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

    function textsShown(root) {
        const texts = []
        for (const item of allItems(root)) {
            if (item.visible === true && typeof item.text === "string" && item.text !== "") {
                texts.push(item.text)
            }
        }
        return texts
    }

    function screen() {
        const screen = createTemporaryObject(screenComponent, testCase,
                                             { width: 900, height: 700,
                                               scan: scanModel, assist: assistModel })
        verify(screen !== null)
        return screen
    }

    function checkScreen(root) {
        return allItems(root).find(item => item.picked !== undefined
                                            && item.boardRows !== undefined)
    }

    function tiles(root) {
        return allItems(root).filter(item => typeof item.row === "number"
                                              && typeof item.column === "number")
    }

    // A positioner lays its children out in the window's polish, a frame after
    // the model changed, and the check screen's own geometry is resolved over
    // the first few frames as its sub-items arrive. A click before that would be
    // turned into a point on stale geometry, so a suite lets the window draw and
    // then waits for the layout it is about to press rather than for a number of
    // milliseconds alone.
    function waitForTiles(root) {
        wait(150)
        tryVerify(() => {
            const cells = tiles(root)
            const first = cells.find(cell => cell.row === 0 && cell.column === 0)
            const next = cells.find(cell => cell.row === 0 && cell.column === 1)
            const below = cells.find(cell => cell.row === 1 && cell.column === 0)
            return first !== undefined && next !== undefined && below !== undefined
                   && next.x > first.x && next.y === first.y && below.y > first.y
        })
    }

    function waitForLetters(root) {
        wait(100)
        tryVerify(() => {
            const a = buttonNamed(root, "A")
            const b = buttonNamed(root, "B")
            return a !== null && b !== null && b.x > a.x && b.y === a.y
        })
    }

    function fill(letter) {
        for (let index = 0; index < scanModel.rows * scanModel.columns; ++index) {
            scanModel.setTile(index, letter)
        }
    }

    // --- scan.ready ---------------------------------------------------------
    function test_theCameraCodesAreTheDesignsSentences() {
        // CameraSource reports these three; the platform interface's older
        // codes still map to the same sentences. A camera that is not found may
        // still be there, so the absent sentence says so and "Try camera again"
        // follows it (the no-camera test below).
        compare(ScanWording.cameraProblem("no_camera"), ScanWording.cameraAbsent)
        compare(ScanWording.cameraProblem("permission"), ScanWording.cameraRefused)
        compare(ScanWording.cameraProblem("failed"), ScanWording.cameraBroken)
        compare(ScanWording.cameraProblem("unavailable"), ScanWording.cameraAbsent)
        compare(ScanWording.cameraProblem(""), "")
        verify(ScanWording.tryCameraAgain.length > 0)
    }

    function test_readySaysThereIsNoCamera() {
        const view = screen()
        compare(scanModel.state, "ready")
        // Opening the screen starts the camera; this desktop's interface answers
        // "unavailable", and the screen says so in the design's words.
        tryCompare(scanModel, "cameraProblem", "unavailable")
        const texts = textsShown(view)
        verify(texts.indexOf(ScanWording.cameraAbsent) >= 0)
        // There is no Capture without a camera, and the typed board is the way on.
        compare(buttonNamed(view, ScanWording.capture).visible, false)
        verify(buttonNamed(view, ScanWording.typeBoard).visible)
        // The camera's absence can be retried without leaving the screen: the
        // attempt re-enumerates and, with no device, says the same truth again.
        const retry = buttonNamed(view, ScanWording.tryCameraAgain)
        verify(retry !== null)
        verify(retry.visible)
        mouseClick(retry, retry.width / 2, retry.height / 2)
        compare(scanModel.state, "ready")
        compare(scanModel.cameraProblem, "unavailable")
        verify(textsShown(view).indexOf(ScanWording.cameraAbsent) >= 0)
        compare(scanModel.readProblem, "")
        // No live view either: there is no picture to put in the frame.
        compare(view.previewShown, false)
        verify(allItems(view).every(item => item.objectName !== "cameraPreview"))
    }

    // --- scan.ready with a camera -------------------------------------------
    function test_aRunningCameraShowsThePreviewAndOffersCapture() {
        // A fake source, not a device: the preview is created, so the QML path
        // that a camera would take is the one a headless run drives. The frame
        // it would hand over is not needed to show the live view.
        fixture.useCamera()
        const view = screen()
        tryCompare(scanModel, "cameraAvailable", true)
        compare(scanModel.cameraCapturing, true)
        compare(view.previewShown, true)
        verify(allItems(view).some(item => item.objectName === "cameraPreview"))
        verify(buttonNamed(view, ScanWording.capture).visible)
        verify(textsShown(view).indexOf(ScanWording.cameraAbsent) < 0)
        verify(buttonNamed(view, ScanWording.typeBoard).visible)
    }

    // --- scan.ready -> scan.check -------------------------------------------
    function test_typingABoardOpensTheCheckScreen() {
        const view = screen()
        const typeBoard = buttonNamed(view, ScanWording.typeBoard)
        verify(typeBoard !== null)
        mouseClick(typeBoard, typeBoard.width / 2, typeBoard.height / 2)

        compare(scanModel.state, "check")
        compare(view.scanState, "check")
        compare(scanModel.rows, 4)
        compare(scanModel.columns, 4)
        compare(scanModel.draftFromCamera, false)
        const texts = textsShown(view)
        verify(texts.indexOf(ScanWording.checkTitle) >= 0)
        // A typed board was not read, so the secondary action starts a fresh
        // typed board rather than pretending to scan again.
        verify(texts.indexOf(ScanWording.startOver) >= 0)
        verify(texts.indexOf(ScanWording.scanAgain) < 0)
        verify(texts.indexOf(ScanWording.startPlaying) >= 0)
        // Turn is the camera board's control.
        compare(buttonNamed(view, ScanWording.turn).visible, false)
        // The typed board rings nothing: there is no reader's doubt to explain.
        compare(scanModel.doubtful.length, 0)
        verify(texts.indexOf(ScanWording.doubtfulHint) < 0)
    }

    // --- the secondary action and its alignment ------------------------------
    function test_typedBoardsStartOverOnAFreshTypedBoard() {
        const view = screen()
        scanModel.typeBoard()
        scanModel.setTile(0, "A")
        waitForTiles(view)
        compare(scanModel.state, "check")

        const again = buttonNamed(view, ScanWording.startOver)
        verify(again !== null)
        verify(again.visible)
        mouseClick(again, again.width / 2, again.height / 2)

        // The typing task stays: same screen, same size, no letters, no camera.
        compare(scanModel.state, "check")
        compare(scanModel.rows, 4)
        compare(scanModel.columns, 4)
        compare(scanModel.draftFromCamera, false)
        compare(scanModel.letters[0], "")
        compare(scanModel.doubtful.length, 0)
        verify(buttonNamed(view, ScanWording.startOver).visible)
        compare(buttonNamed(view, ScanWording.turn).visible, false)
    }

    function test_theCheckScreensActionsAreAlignedAndOnTheTrailingSide() {
        const view = screen()
        scanModel.typeBoard()
        waitForTiles(view)

        const secondary = buttonNamed(view, ScanWording.startOver)
        const primary = buttonNamed(view, ScanWording.startPlaying)
        verify(secondary !== null && primary !== null)
        // One row, one centre line: the secondary (64 dp) is not left hanging
        // from the top of the primary (96 dp).
        const secondaryCentre = secondary.y + secondary.height / 2
        const primaryCentre = primary.y + primary.height / 2
        verify(Math.abs(secondaryCentre - primaryCentre) < 1)
        // The pair sits at the trailing side, secondary on its leading side.
        verify(primary.x > secondary.x)
        verify(primary.mapToItem(view, 0, 0).x + primary.width > view.width / 2)
    }

    function test_rightToLeftKeepsTheActionsOnTheTrailingSide() {
        // Hebrew and Yiddish: the group follows the direction; the replica of
        // the board does not.
        Theme.rightToLeft = true
        const view = screen()
        compare(view.mirrored, true)
        scanModel.typeBoard()
        waitForTiles(view)

        const secondary = buttonNamed(view, ScanWording.startOver)
        const primary = buttonNamed(view, ScanWording.startPlaying)
        verify(secondary !== null && primary !== null)
        const primaryX = primary.mapToItem(view, 0, 0).x
        const secondaryX = secondary.mapToItem(view, 0, 0).x
        verify(primaryX + primary.width < view.width / 2)
        verify(secondaryX > primaryX)
        // The tiles still read left to right, as the real board does.
        const first = tiles(view).find(cell => cell.row === 0 && cell.column === 0)
        const next = tiles(view).find(cell => cell.row === 0 && cell.column === 1)
        verify(first !== null && next !== null)
        verify(next.x > first.x)
    }

    function test_scanAgainReturnsACameraBoardToTheFrame() {
        verify(fixture.readDoubtfulBoard())
        compare(scanModel.draftFromCamera, true)
        const view = screen()
        waitForTiles(view)

        const scanAgain = buttonNamed(view, ScanWording.scanAgain)
        verify(scanAgain !== null)
        verify(scanAgain.visible)
        verify(buttonNamed(view, ScanWording.startOver) === null
               || !buttonNamed(view, ScanWording.startOver).visible)
        // Turn is offered for a camera-read board, and only for one.
        verify(buttonNamed(view, ScanWording.turn).visible)
        mouseClick(scanAgain, scanAgain.width / 2, scanAgain.height / 2)

        compare(scanModel.state, "ready")
        compare(scanModel.rows, 0)
        compare(scanModel.draftFromCamera, false)
        compare(view.scanState, "ready")
        verify(buttonNamed(view, ScanWording.typeBoard).visible)
    }

    function test_backFromTheCheckScreenReturnsToTheFrame() {
        const view = screen()
        closedSpy.target = view
        closedSpy.clear()
        scanModel.typeBoard()
        compare(view.scanState, "check")

        const back = buttonNamed(view, ScanWording.back)
        verify(back !== null)
        mouseClick(back, back.width / 2, back.height / 2)

        // Back is the step back, not the typing task's Start over: it drops the
        // draft and shows the frame (with no camera, the typed board is there).
        compare(scanModel.state, "ready")
        compare(scanModel.rows, 0)
        compare(closedSpy.count, 0)
        compare(view.scanState, "ready")
    }

    // --- the stepper ---------------------------------------------------------
    function test_theStepperShowsPlaceCheckAndPlay() {
        const view = screen()
        compare(view.step, 1)
        let texts = textsShown(view)
        verify(texts.indexOf("1  Place") >= 0)
        verify(texts.indexOf("2  Check") >= 0)
        verify(texts.indexOf("3  Play") >= 0)

        scanModel.typeBoard()
        compare(view.step, 2)
        texts = textsShown(view)
        verify(texts.indexOf("2  Check") >= 0)
        verify(texts.indexOf("3  Play") >= 0)
    }

    // --- the frame's bracket geometry ---------------------------------------
    function test_theCornerBracketsPointIntoTheFrame() {
        const view = screen()
        const corners = allItems(view).filter(item => item.objectName === "frameCorner")
        compare(corners.length, 4)
        for (const corner of corners) {
            const parts = allItems(corner)
            const horizontal = parts.find(item => item.objectName === "cornerHorizontal")
            const vertical = parts.find(item => item.objectName === "cornerVertical")
            verify(horizontal !== undefined && vertical !== undefined)
            // Top corners carry the horizontal bar at the top, bottom corners at
            // the bottom; the vertical bar runs the full side on the outside.
            compare(horizontal.y, corner.onBottom ? corner.height - horizontal.height : 0)
            compare(horizontal.x, 0)
            compare(vertical.x, corner.onRight ? corner.width - vertical.width : 0)
            compare(vertical.y, 0)
            compare(vertical.height, corner.height)
        }
    }

    // --- a correction -------------------------------------------------------
    function test_aCorrectionSetsTheTilesLetter() {
        const view = screen()
        scanModel.typeBoard()
        waitForTiles(view)
        const check = checkScreen(view)
        verify(check !== null)

        const cells = tiles(view)
        compare(cells.length, 16)
        const first = cells.find(cell => cell.row === 0 && cell.column === 0)
        verify(first !== null)
        mouseClick(first, first.width / 2, first.height / 2)

        // The picker opens on that tile and offers A to Z.
        compare(check.picked, 0)
        waitForLetters(view)
        const a = buttonNamed(view, "A")
        verify(a !== null)
        verify(a.visible)
        mouseClick(a, a.width / 2, a.height / 2)

        compare(scanModel.letters[0], "A")
        compare(check.picked, -1)
        compare(scanModel.complete, false)
    }

    // --- the reader's doubts -------------------------------------------------
    function test_aDoubtfulTileIsRingedAndCanBeSettled() {
        // A reading with three tiles the reader could not trust: the check
        // screen rings them and says what a ring means.
        verify(fixture.readDoubtfulBoard())
        compare(scanModel.state, "check")
        compare(scanModel.rows, 2)
        compare(scanModel.doubtful.length, 3)

        const view = screen()
        waitForTiles(view)
        const marks = allItems(view).filter(item => item.visible === true
                                                   && item.text === ScanWording.doubtfulMark)
        compare(marks.length, 3)
        verify(textsShown(view).indexOf(ScanWording.doubtfulHint) >= 0)

        // The picker offers the reader's other thought first: O before A to Z.
        const check = checkScreen(view)
        const doubtfulTile = tiles(view).find(cell => cell.row === 0 && cell.column === 1)
        mouseClick(doubtfulTile, doubtfulTile.width / 2, doubtfulTile.height / 2)
        compare(check.picked, 1)
        waitForLetters(view)
        compare(scanModel.alternatives(1).length, 1)
        const suggestion = buttonNamed(view, "O")
        verify(suggestion !== null)
        mouseClick(suggestion, suggestion.width / 2, suggestion.height / 2)

        // A correction is the grown-up's: the tile's doubt is settled.
        compare(scanModel.letters[1], "O")
        compare(scanModel.doubtful.length, 2)
        compare(scanModel.doubtful.indexOf(1), -1)
    }

    // --- Start playing -------------------------------------------------------
    function test_startPlayingRefusesAnIncompleteBoard() {
        const view = screen()
        playSpy.target = view
        playSpy.clear()
        scanModel.typeBoard()
        waitForTiles(view)

        const yes = buttonNamed(view, ScanWording.startPlaying)
        verify(yes !== null)
        mouseClick(yes, yes.width / 2, yes.height / 2)

        compare(playSpy.count, 0)
        compare(assistModel.active, false)
        compare(scanModel.state, "check")
        verify(textsShown(view).indexOf(ScanWording.incomplete) >= 0)
    }

    function test_startPlayingHandsTheConfirmedBoardToTheAssistant() {
        const view = screen()
        playSpy.target = view
        playSpy.clear()
        scanModel.typeBoard()
        fill("A")
        compare(scanModel.complete, true)
        waitForTiles(view)

        const yes = buttonNamed(view, ScanWording.startPlaying)
        verify(yes !== null)
        mouseClick(yes, yes.width / 2, yes.height / 2)

        compare(playSpy.count, 1)
        verify(assistModel.active)
        compare(assistModel.rows, 4)
        compare(assistModel.columns, 4)
        compare(assistModel.tiles.length, 16)
        compare(assistModel.tiles[0], "A")
        // The shell shows the assistant's screen; the scan screen itself only
        // says the board is confirmed and play can start.
        compare(assistModel.path.length, 0)
    }

    function test_theWayBackLeavesModeAFromTheReadyScreen() {
        const view = screen()
        closedSpy.target = view
        closedSpy.clear()

        const back = buttonNamed(view, ScanWording.back)
        verify(back !== null)
        mouseClick(back, back.width / 2, back.height / 2)

        compare(closedSpy.count, 1)
        compare(scanModel.state, "ready")
    }
}
