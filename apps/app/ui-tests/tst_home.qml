// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import SnapAndSpell

// Home (apps/app/qml/Home.qml): one large child placard, one adult card, and the
// app bar's adult entry. The brief's first rule is that the child's way in is
// the obvious one; these checks hold that shape, its keyboard focus, and the
// reading direction, without looking for the retired family-scanner card.
TestCase {
    id: testCase
    name: "Home"
    when: windowShown
    visible: true

    width: 900
    height: 600

    Component { id: homeComponent; Home { controller: parentController } }

    SignalSpy { id: soloSpy; signalName: "soloRequested" }
    SignalSpy { id: parentSpy; signalName: "parentRequested" }
    SignalSpy { id: boardsSpy; signalName: "boardsRequested" }

    function init() {
        failOnWarning(/.?/)
        fixture.reset()
        Navigation.home()
        Navigation.scanAvailable = true
        Navigation.parentAvailable = true
        Theme.rightToLeft = false
    }

    function cleanup() {
        Theme.textScale = 1.0
        Navigation.home()
        Navigation.scanAvailable = true
        Navigation.parentAvailable = true
        Theme.rightToLeft = false
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

    function picture(root, name) {
        for (const item of allItems(root)) {
            if (item.accessibleName === name) {
                return item
            }
        }
        return null
    }

    function home() {
        const home = createTemporaryObject(homeComponent, testCase, { width: 900, height: 600 })
        verify(home !== null)
        // The bar's entries are placed on the first polish, as a person sees them: after a frame.
        verify(waitForRendering(home, 2000))
        return home
    }

    function test_oneChildPlacardAndTheAdultWayIn() {
        const screen = home()

        compare(screen.pictures, 2)
        const play = picture(screen, HomeWording.soloPicture)
        const board = picture(screen, HomeWording.familyPicture)
        const grownUps = picture(screen, HomeWording.forGrownUps)
        verify(play !== null)
        verify(board !== null)
        verify(grownUps !== null)
        compare(play.visible, true)
        compare(board.visible, true)
        compare(grownUps.visible, true)
        verify(play.width >= Theme.targetPrimary)
        verify(play.height >= Theme.targetPrimary)
        compare(grownUps.targetHeight, Theme.targetMin)
    }

    function test_eachWayInAsksForItsScreen() {
        const screen = home()
        soloSpy.target = screen
        parentSpy.target = screen
        soloSpy.clear()
        parentSpy.clear()

        const play = picture(screen, HomeWording.soloPicture)
        const board = picture(screen, HomeWording.familyPicture)
        // The Grid gets its child widths in the first frame; wait until the two
        // cards have taken their places, so the click cannot fall through to the
        // card underneath.
        tryVerify(() => board.x > play.x + play.width, 2000)
        mouseClick(play, play.width / 2, play.height / 2)
        compare(soloSpy.count, 1)
        compare(parentSpy.count, 0)

        mouseClick(board, board.width / 2, board.height / 2)
        compare(parentSpy.count, 1)
        compare(soloSpy.count, 1)

        const grownUps = picture(screen, HomeWording.forGrownUps)
        mouseClick(grownUps, grownUps.width / 2, grownUps.height / 2)
        compare(parentSpy.count, 2)
    }

    function test_theBoardsEntryAsksForTheHistory() {
        const screen = home()
        boardsSpy.target = screen
        boardsSpy.clear()

        const boards = picture(screen, qsTr("My boards"))
        verify(boards !== null)
        mouseClick(boards, boards.width / 2, boards.height / 2)

        compare(boardsSpy.count, 1)
    }

    function test_theChildPlacardHasTheKeyboardAndSpaceOpensIt() {
        const screen = home()
        soloSpy.target = screen
        soloSpy.clear()

        screen.forceActiveFocus()
        const play = picture(screen, HomeWording.soloPicture)
        tryCompare(play, "activeFocus", true)

        keyClick(Qt.Key_Space)
        compare(soloSpy.count, 1)
    }

    function test_backOnHomeDoesNothing() {
        const screen = home()
        soloSpy.target = screen
        parentSpy.target = screen
        soloSpy.clear()
        parentSpy.clear()
        screen.forceActiveFocus()

        keyClick(Qt.Key_Escape)
        keyClick(Qt.Key_Back)

        compare(Navigation.screen, "home")
        compare(Navigation.history.length, 0)
        compare(soloSpy.count, 0)
        compare(parentSpy.count, 0)
    }

    // A card's name for assistive technology is the title written on it: the
    // right card was once "Play with the family" over "Play with a real board".
    function test_aCardIsNamedByItsVisibleTitle() {
        const screen = home()
        for (const item of allItems(screen)) {
            if (typeof item.title === "string" && typeof item.accessibleName === "string"
                    && typeof item.grownUp === "boolean") {
                compare(item.accessibleName, item.title)
            }
        }
        verify(picture(screen, "Play with a real board") !== null)
        verify(picture(screen, "Play on my own") !== null)
    }

    // The words of a card are never drawn over its picture: at 150% text the
    // card is as tall as its words and its least picture need, not a fixed
    // height, and the page scrolls when that is more than the window has.
    function test_aLargerTextSizeMakesTheCardTallerNotCrowded() {
        Theme.textScale = 1.5
        const screen = home()
        const play = picture(screen, HomeWording.soloPicture)
        const board = picture(screen, HomeWording.familyPicture)
        verify(play.height >= play.minimumHeight)
        verify(board.height >= board.minimumHeight)
        compare(play.height, board.height)
        Theme.textScale = 1.0
    }

    // At the largest text in the window the app opens at, the picture gives way and both cards, with
    // their buttons, are in view: the child's Play does not wait under a scroll.
    function test_playIsInViewAtLargeTextInTheDefaultWindow() {
        Theme.textScale = 1.5
        const screen = createTemporaryObject(homeComponent, testCase, { width: 1160, height: 740 })
        verify(screen !== null)
        verify(waitForRendering(screen, 2000))
        const play = picture(screen, HomeWording.soloPicture)
        const at = play.mapToItem(screen, 0, 0)
        verify(at.y + play.height <= screen.height, "the Play card ends at " + (at.y + play.height))
        Theme.textScale = 1.0
    }

    // In a window far larger than the design the content is centred under the
    // bar, not left at the top of an empty page.
    function test_aVeryLargeWindowCentresTheColumn() {
        const screen = createTemporaryObject(homeComponent, testCase, { width: 2400, height: 1200 })
        verify(screen !== null)
        const play = picture(screen, HomeWording.soloPicture)
        const board = picture(screen, HomeWording.familyPicture)
        tryVerify(() => board.x > play.x + play.width, 2000)
        // The column grows to the window's zoom and the cards follow it a frame later.
        tryVerify(() => Math.abs(play.parent.width - play.parent.parent.width) < 1, 2000)
        const left = play.mapToItem(screen, 0, 0).x
        const right = screen.width - (board.mapToItem(screen, 0, 0).x + board.width)
        verify(Math.abs(left - right) < 2)
        const top = play.mapToItem(screen, 0, 0).y - Theme.appBarHeight
        const bottom = screen.height - (play.mapToItem(screen, 0, 0).y + play.height)
        verify(bottom > top * 0.4)
    }

    // A phone held upright: one card to a page, the whole of the first one in view with its button, the
    // second a swipe away (its edge shows). Nothing on the page scrolls down.
    function test_aPhoneShowsOneWholeCardAndTheNextIsASwipeAway() {
        const screen = createTemporaryObject(homeComponent, testCase, { width: 402, height: 778 })
        verify(screen !== null)
        verify(waitForRendering(screen, 2000))
        const pager = findChild(screen, "homePager")
        verify(pager !== null)
        compare(pager.currentIndex, 0)
        compare(screen.paged, true)

        const play = picture(screen, HomeWording.soloPicture)
        const board = picture(screen, HomeWording.familyPicture)
        verify(play !== null && board !== null)
        const at = play.mapToItem(screen, 0, 0)
        verify(at.x >= 0 && at.x + play.width <= screen.width, "Play is across the screen")
        verify(at.y >= Theme.appBarHeight - 1 && at.y + play.height <= screen.height, "Play is whole on the screen")
        verify(play.height >= play.minimumHeight)
        // The next card's edge is in view, and no more than its edge.
        const next = board.mapToItem(screen, 0, 0).x
        verify(next > at.x + play.width && next < screen.width, "the grown-ups' card shows its edge")

        // A swipe to the side brings it in.
        mouseDrag(pager, 300, 200, -240, 0, Qt.LeftButton, Qt.NoModifier, 20)
        tryCompare(pager, "currentIndex", 1, 2000)
        tryVerify(() => board.mapToItem(screen, 0, 0).x < screen.width / 4, 2000)
        parentSpy.target = screen
        parentSpy.clear()
        // The pager has come to rest.
        wait(2 * Theme.turn)
        mouseClick(board, board.width / 2, board.height / 2)
        compare(parentSpy.count, 1)

        // Nothing scrolls up and down.
        for (const item of allItems(screen)) {
            if (item.contentHeight !== undefined && item.flickableDirection !== undefined && item.visible) {
                verify(item.contentHeight <= item.height + 1, "the page scrolls down")
            }
        }
    }

    // A phone on its side is too low for a column: a card to a page again, the picture beside the words.
    function test_aPhoneOnItsSideShowsTheCardSideways() {
        const screen = createTemporaryObject(homeComponent, testCase, { width: 750, height: 381 })
        verify(screen !== null)
        verify(waitForRendering(screen, 2000))
        compare(screen.paged, true)
        const play = picture(screen, HomeWording.soloPicture)
        verify(play.sideways)
        const at = play.mapToItem(screen, 0, 0)
        verify(at.y + play.height <= screen.height, "Play is whole on the screen")
    }

    function test_theCardsFollowTheReadingDirection() {
        const screen = home()
        const play = picture(screen, HomeWording.soloPicture)
        const board = picture(screen, HomeWording.familyPicture)
        tryVerify(() => play.x < board.x, 2000)

        Theme.rightToLeft = true
        tryVerify(() => play.x > board.x, 2000)
    }
}
