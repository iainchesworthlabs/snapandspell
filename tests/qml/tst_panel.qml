import QtQuick
import QtTest
import WordJewelsSolver

// The panel of solo.playing (src/app/qml/solo/PlayingPanel.qml): the tray of
// words found, the hint, and Done while a tapped path waits. The model is the
// application's SoloModel over the fake game, so the words, tiers and stars are
// the design's test data and the panel is the screen's own file.
TestCase {
    id: testCase
    name: "Panel"
    when: windowShown
    visible: true

    width: 800
    height: 600

    Component { id: panelComponent; PlayingPanel { } }

    SignalSpy { id: historySpy; signalName: "historyRequested" }

    // The fake's 3x3 board, seed 1: E N S / O A I / M P T. SIT is tiles 2, 5, 8.
    readonly property var sit: [2, 5, 8]

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

    function panel() {
        const panel = createTemporaryObject(panelComponent, testCase,
                                            { width: 280, height: 500, wide: false,
                                              solo: soloModel })
        verify(panel !== null)
        return panel
    }

    function play() {
        soloModel.shake()
        soloModel.sceneSettled()
        compare(soloModel.state, "playing")
    }

    function tapAndHandIn() {
        for (const index of sit) {
            soloModel.tapTile(index)
        }
        soloModel.done()
    }

    function test_theTrayShowsTheWordsTiersAndStars() {
        play()
        tapAndHandIn()
        compare(soloModel.state, "word-found")
        compare(soloModel.found.length, 1)
        const found = soloModel.found[0]
        compare(found.word, "sit")
        compare(found.tier, "spark")
        compare(found.stars, 1)

        const tray = panel()
        // The tray's list is the model's own: one row, the word, and a jewel
        // whose size says the tier (a colour alone never does).
        const list = allItems(tray).find(item => item.count !== undefined
                                                 && item.model !== undefined)
        verify(list !== null)
        compare(list.count, 1)
        compare(list.model.length, 1)
        compare(list.model[0].word, "sit")
        compare(list.model[0].tier, "spark")
        compare(list.model[0].stars, 1)
        verify(textsShown(tray).indexOf("sit") >= 0)

        const jewel = allItems(tray).find(item => item.diameter !== undefined
                                                  && item.tier !== undefined)
        verify(jewel !== null)
        compare(jewel.tier, "spark")
        compare(jewel.diameter, Theme.jewelSpark)
    }

    function test_findingAWordAgainAddsNothing() {
        play()
        tapAndHandIn()
        soloModel.wordShown()
        compare(soloModel.found.length, 1)
        const tray = panel()
        const list = allItems(tray).find(item => item.count !== undefined
                                                 && item.model !== undefined)
        compare(list.count, 1)

        tapAndHandIn()

        // The word is shown again (the screen times it), and the tray is the
        // same tray: a word is one jewel however often it is made.
        compare(soloModel.state, "word-found")
        compare(soloModel.found.length, 1)
        compare(list.count, 1)
    }

    function test_theHintButtonAsksTheModelForAStart() {
        play()
        const tray = panel()
        compare(soloModel.helpTile, -1)

        const hint = buttonNamed(tray, SoloWording.showStart)
        verify(hint !== null)
        compare(hint.visible, true)
        mouseClick(hint, hint.width / 2, hint.height / 2)

        // The fake's help: the start tile of NIP, N at row 0 column 1.
        compare(soloModel.helpTile, 1)
    }

    function test_aTappedPathLeavesTheHintAndThePanelAlone() {
        play()
        const tray = panel()
        compare(tray.pending, false)

        soloModel.tapTile(2)
        soloModel.tapTile(5)

        compare(tray.pending, true)
        compare(tray.hintShown, true)
        // Done belongs to the screen's action bar now, so the panel keeps its
        // hint rather than swapping a second primary action into the card.
        compare(buttonNamed(tray, SoloWording.showStart).visible, true)
        compare(buttonNamed(tray, SoloWording.done), null)

        soloModel.tapTile(8)
        soloModel.done()

        compare(soloModel.path.length, 0)
        compare(soloModel.state, "word-found")
        compare(soloModel.found.length, 1)
        compare(soloModel.found[0].word, "sit")
    }

    function test_theHistoryButtonAsksForTheSheet() {
        play()
        const tray = panel()
        historySpy.target = tray
        historySpy.clear()

        const button = buttonNamed(tray, SoloWording.earlierBoards)
        verify(button !== null)
        mouseClick(button, button.width / 2, button.height / 2)

        compare(historySpy.count, 1)
    }
}
