import QtQuick
import QtTest
import WordJewelsSolver
import WordJewelsSolver.Parent
import WordJewelsSolver.Scan

// Hebrew and Yiddish (docs/audit-0.1.md, "Languages, RTL and i18n"): the chrome
// mirrors and the board does not. Three things are held here, and they are the
// three the language checks in the stub shells hold for the other two areas:
//
//   - the bar's way back is on the leading edge (the left in English, the right
//     in Hebrew and Yiddish),
//   - the panel that is not the board takes the leading side of the split, and
//     inside the panel the hint takes the leading side,
//   - the board and its tiles keep their reading order in every language: the
//     square's own geometry, the Solo Play trace layer's tiles and the check
//     screen's replica are the same left to right as in English.
TestCase {
    id: testCase
    name: "Rtl"
    when: windowShown
    visible: true

    width: 1100
    height: 700

    Component { id: screenComponent; SoloScreen { } }
    Component { id: panelComponent; PlayingPanel { } }
    Component { id: layoutComponent; SoloLayout { } }
    Component { id: traceComponent; TraceLayer { } }
    Component { id: scanComponent; ScanScreen { } }
    Component { id: checkComponent; CheckScreen { } }
    Component { id: parentComponent; ParentArea { } }

    Component {
        id: mirroredComponent
        Item {
            LayoutMirroring.enabled: true
            LayoutMirroring.childrenInherit: true
        }
    }

    function init() {
        // A QML warning in a test function fails it, as one outside does the
        // whole run: a screen that warned is a screen that is wrong.
        failOnWarning(/.?/)
        fixture.reset()
        useLanguage("en")
    }

    function cleanup() {
        useLanguage("en")
    }

    function useLanguage(code) {
        fixture.useLanguage(code)
        // What the shell does with the manager's answer (Main.qml).
        Theme.rightToLeft = languageManager.rightToLeft
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

    function play() {
        soloModel.shake()
        soloModel.sceneSettled()
        compare(soloModel.state, "playing")
    }

    // --- the direction ------------------------------------------------------
    function test_hebrewAndYiddishReadRightToLeft() {
        for (const code of ["he", "yi"]) {
            useLanguage(code)
            compare(languageManager.code, code)
            compare(languageManager.rightToLeft, true)
            compare(Qt.application.layoutDirection, Qt.RightToLeft,
                    code + " turns the interface around")
        }

        for (const code of ["en", "fr"]) {
            useLanguage(code)
            compare(languageManager.rightToLeft, false)
            compare(Qt.application.layoutDirection, Qt.LeftToRight)
        }
    }

    // --- the bar's way back --------------------------------------------------
    function test_theBarsWayBackIsOnTheLeadingEdge() {
        const screen = createTemporaryObject(screenComponent, testCase,
                                             { width: 1100, height: 700, solo: soloModel })
        verify(screen !== null)
        const back = buttonNamed(screen, SoloWording.back)
        verify(back !== null)

        const backX = () => back.mapToItem(screen, 0, 0).x
        verify(backX() < screen.width / 2, "English: the way back is on the left")

        useLanguage("he")
        compare(screen.mirrored, true)
        tryVerify(() => backX() > screen.width / 2, 5000,
                  "Hebrew: the way back is on the right")
    }

    function test_theScanBarsWayBackFollowsTheLanguage() {
        const scan = createTemporaryObject(scanComponent, testCase,
                                           { width: 900, height: 700, scan: scanModel,
                                             assist: assistModel })
        verify(scan !== null)
        verify(scan.backButtonX < scan.width / 2)

        useLanguage("yi")
        compare(scan.mirrored, true)
        tryVerify(() => scan.backButtonX > scan.width / 2)
    }

    function test_theParentAreasWayBackFollowsTheLanguage() {
        const area = createTemporaryObject(parentComponent, testCase,
                                           { width: 1024, height: 700,
                                             controller: parentController })
        verify(area !== null)
        verify(area.backButtonX < area.width / 2)

        area.rightToLeft = true
        compare(area.mirrored, true)
        tryVerify(() => area.backButtonX > area.width / 2)
        area.rightToLeft = false
        tryVerify(() => area.backButtonX < area.width / 2)
    }

    // --- the panel and the split ---------------------------------------------
    function test_thePanelFollowsTheReadingDirection() {
        play()
        const panel = createTemporaryObject(panelComponent, testCase,
                                            { width: 280, height: 500, wide: false,
                                              solo: soloModel })
        verify(panel !== null)
        const hint = buttonNamed(panel, SoloWording.showStart)
        const history = buttonNamed(panel, SoloWording.earlierBoards)
        verify(hint !== null)
        verify(history !== null)
        // The cards are stacked in reading order, not placed side by side; the
        // panel itself mirrors, and its cards keep the same start edge.
        const hintCard = hint.parent
        const historyCard = history.parent
        compare(panel.mirrored, false)
        verify(hintCard.width > 0)
        verify(historyCard.width > 0)
        verify(hintCard.y < historyCard.y)

        Theme.rightToLeft = true
        compare(panel.mirrored, true)
        verify(hintCard.width > 0)
        verify(historyCard.width > 0)
        verify(hintCard.y < historyCard.y)
    }

    function test_theSplitMirrorsButTheBoardDoesNot() {
        const ltr = createTemporaryObject(layoutComponent, testCase,
                                          { width: 900, height: 700, boardSize: 3,
                                            rightToLeft: false })
        const rtl = createTemporaryObject(layoutComponent, testCase,
                                          { width: 900, height: 700, boardSize: 3,
                                            rightToLeft: true })

        // The split mirrors: the board's area and the panel's change sides.
        verify(ltr.boardArea.x < rtl.boardArea.x)
        verify(ltr.panelArea.x > rtl.panelArea.x)
        verify(ltr.panelArea.x > 900 * 2 / 3 - 1)
        verify(rtl.panelArea.x < 900 / 3 + 1)

        // The board does not: the same square, at the same place within its own
        // area, and the same tiles.
        compare(ltr.wide, rtl.wide)
        compare(ltr.tile, rtl.tile)
        compare(ltr.gap, rtl.gap)
        compare(ltr.boardRect.width, rtl.boardRect.width)
        compare(ltr.boardRect.height, rtl.boardRect.height)
        compare(ltr.boardRect.x - ltr.boardArea.x, rtl.boardRect.x - rtl.boardArea.x)
        compare(ltr.boardRect.y - ltr.boardArea.y, rtl.boardRect.y - rtl.boardArea.y)
    }

    function test_theTraceLayersTilesKeepTheirOrder() {
        play()
        const parent = createTemporaryObject(mirroredComponent, testCase,
                                             { width: 700, height: 700 })
        verify(parent !== null)
        const trace = createTemporaryObject(traceComponent, parent,
                                            { width: 600, height: 600, solo: soloModel,
                                              boardRect: Qt.rect(30, 30, 540, 540),
                                              tileSize: 180, tileGap: 0 })
        verify(trace !== null)
        // The layer is inside a mirrored hierarchy and switches the mirroring
        // off: a tile's place is the board's own, row by row from the left.
        compare(trace.tileX(0), 30)
        compare(trace.tileX(1), 210)
        compare(trace.tileX(2), 390)
        compare(trace.tileY(0), 30)
        compare(trace.tileY(3), 210)
    }

    function test_theChecksReplicaKeepsItsReadingOrder() {
        // The check screen of Mode A mirrors its rows and its buttons, and its
        // replica of the family's board keeps the order the board was confirmed
        // in: the first tile is the top-left one in Hebrew too.
        scanModel.typeBoard()
        const parent = createTemporaryObject(mirroredComponent, testCase,
                                             { width: 900, height: 700 })
        verify(parent !== null)
        const check = createTemporaryObject(checkComponent, parent,
                                            { width: 900, height: 700, scan: scanModel,
                                              assist: assistModel })
        verify(check !== null)
        compare(check.mirrored, true)

        const cells = allItems(check).filter(item => typeof item.row === "number"
                                                      && typeof item.column === "number")
        compare(cells.length, 16)
        const first = cells.find(cell => cell.row === 0 && cell.column === 0)
        const next = cells.find(cell => cell.row === 0 && cell.column === 1)
        const below = cells.find(cell => cell.row === 1 && cell.column === 0)
        verify(first !== null)
        verify(next !== null)
        verify(below !== null)
        verify(first.x < next.x)
        compare(first.y, next.y)
        verify(first.y < below.y)
        compare(first.x, below.x)
    }
}
