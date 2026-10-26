// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import WordJewelsSolver
import WordJewelsSolver.Parent
import WordJewelsSolver.Scan

// A picture of every screen and dialog of the application, from the offscreen app (docs/testing.md,
// "Screenshots"). Not a test: the file is not called tst_*.qml, so the harness does not register it, and it
// asserts only that a screen could be made. It writes PNG files into the directory the fixture names
// (WJS_SHOTS_DIR, else <build>/screenshots), numbered in the order of docs/design/toybox/DESIGN.md
// section 8:
//
//   cmake --build --preset <preset> --target wjs_screenshots
//
// Light, then dark, then a larger text size, then the right-to-left, Chinese and Japanese Home, then five frames
// of the board scene (the closed box, the shake, the lid lifting, the drop under way, the settled board).
TestCase {
    id: testCase
    name: "Capture"
    when: windowShown
    visible: true
    width: 1160
    height: 740

    readonly property string out: fixture.shotsDirectory()

    Component { id: homeComponent; Home { controller: parentController } }
    Component { id: soloComponent; SoloScreen { } }
    Component { id: jewelsComponent; JewelsScreen { } }
    Component { id: areaComponent; ParentArea { } }
    Component { id: scanComponent; ScanScreen { } }
    Component { id: checkComponent; CheckScreen { } }
    Component { id: assistComponent; AssistScreen { } }
    Component { id: galleryComponent; KitGallery { } }
    Component { id: sheetComponent; SheetGallery { } }

    function init() {
        fixture.reset()
        Navigation.home()
        Theme.rightToLeft = false
        Theme.language = "en"
        Theme.reducedMotion = false
        Theme.textScale = 1.0
    }

    function shot(item, name) {
        wait(250)
        const image = grabImage(item)
        image.save(out + name + ".png")
    }

    function allItems(root, found) {
        found = found || []
        found.push(root)
        const kids = root.children
        for (let i = 0; i < kids.length; ++i) allItems(kids[i], found)
        return found
    }
    function control(root, text) {
        for (const item of allItems(root))
            if (item.label === text && typeof item.checkable === "boolean") return item
        return null
    }
    function make(component, props, w, h) {
        const o = createTemporaryObject(component, testCase, Object.assign({ width: w || 1160, height: h || 740 }, props || {}))
        verify(o !== null)
        return o
    }
    function pathFor(word) {
        const tiles = soloModel.tiles, columns = soloModel.columns, rows = soloModel.rows
        const upper = word.toUpperCase()
        function walk(at, used, depth) {
            if (tiles[at] !== upper[depth]) return null
            if (depth === upper.length - 1) return used.concat([at])
            for (let dr = -1; dr <= 1; ++dr) for (let dc = -1; dc <= 1; ++dc) {
                const r = Math.floor(at / columns) + dr, c = at % columns + dc
                if ((dr === 0 && dc === 0) || r < 0 || r >= rows || c < 0 || c >= columns) continue
                const next = r * columns + c
                if (used.indexOf(next) >= 0) continue
                const f = walk(next, used.concat([at]), depth + 1)
                if (f !== null) return f
            }
            return null
        }
        for (let first = 0; first < tiles.length; ++first) { const f = walk(first, [], 0); if (f !== null) return f }
        return null
    }
    function findWord(word) {
        for (const i of pathFor(word)) soloModel.tapTile(i)
        soloModel.done()
        soloModel.wordShown()
    }

    function test_01_home() {
        shot(make(homeComponent), "01-home-light")
    }
    function test_02_home_dark_small() {
        const h = make(homeComponent, {}, 900, 620)
        shot(h, "02-home-min-size")
    }
    function test_03_solo_box() {
        shot(make(soloComponent, { solo: soloModel }), "03-solo-box")
    }
    function test_04_solo_playing() {
        const s = make(soloComponent, { solo: soloModel })
        soloModel.shake()
        tryCompare(soloModel, "state", "playing")
        shot(s, "04-solo-playing-empty")
        findWord("sit"); findWord("tap")
        shot(s, "05-solo-playing-two-found")
        const p = pathFor("pat")
        for (const i of p) soloModel.tapTile(i)
        shot(s, "06-solo-playing-word-in-progress")
        soloModel.clearPath()
    }
    function test_05_solo_summary() {
        const s = make(soloComponent, { solo: soloModel })
        soloModel.shake()
        tryCompare(soloModel, "state", "playing")
        for (const w of ["sit", "sat", "tap", "pat", "nip"]) findWord(w)
        tryVerify(() => soloModel.state === "cleared", 3000)
        shot(s, "07-solo-summary")
    }
    function test_06_jewels() {
        shot(make(jewelsComponent, { controller: parentController }), "08-my-jewels")
    }
    function test_06b_gate() {
        const g = make(areaComponent, { controller: parentController }, 1160, 740)
        shot(g, "09-parent-gate")
        parentController.lock()
    }
    function test_07_parent() {
        const a = make(areaComponent, { controller: parentController }, 1160, 1500)
        parentController.unlockForAssistiveTechnology()
        wait(250)
        shot(a, "10-parent-dashboard")
        mouseClick(control(a, ParentWording.tabSettings)); wait(250)
        shot(a, "11-parent-settings")
        mouseClick(control(a, ParentWording.tabAbout)); wait(250)
        shot(a, "12-parent-about")
        mouseClick(control(a, ParentWording.tabLicences)); wait(250)
        shot(a, "13-parent-licences")
        parentController.lock()
    }
    function test_08_scan() {
        shot(make(scanComponent, { scan: scanModel, assist: assistModel }), "14-scan-no-camera")
        fixture.useCamera()
        shot(make(scanComponent, { scan: scanModel, assist: assistModel }), "15-scan-camera-live")
    }
    function test_09_check() {
        fixture.readDoubtfulBoard()
        shot(make(checkComponent, { scan: scanModel, assist: assistModel }), "16-scan-check-doubtful")
    }
    function test_09b_assist() {
        verify(assistModel.begin(4, 4, ["S","A","T","X","P","I","N","X","X","X","X","X","X","X","X","X"]))
        const v = make(assistComponent, { assist: assistModel }, 1100, 700)
        shot(v, "22-assist-board")
        for (const i of [0,1,2]) assistModel.tapTile(i)
        shot(v, "23-assist-word-in-progress")
    }
    function test_09c_dark() {
        Theme.colourScheme = "dark"
        shot(make(homeComponent), "24-home-dark")
        const sc = make(soloComponent, { solo: soloModel })
        soloModel.shake()
        tryCompare(soloModel, "state", "playing")
        findWord("sit")
        shot(sc, "25-solo-playing-dark")
        Theme.colourScheme = "light"
    }
    function test_10_kit() {
        shot(make(galleryComponent, {}, 1160, 900), "17-kit-gallery")
        shot(make(sheetComponent, {}), "18-sheet")
    }
    function test_11_dark_and_scale() {
        Theme.textScale = 1.5
        shot(make(homeComponent), "19-home-text-150")
        Theme.textScale = 1.0
    }
    function test_12_hebrew() {
        fixture.useLanguage("he")
        Theme.rightToLeft = true
        Theme.language = "he"
        shot(make(homeComponent), "20-home-hebrew-rtl")
        fixture.useLanguage("zh_CN")
        Theme.rightToLeft = false
        Theme.language = "zh_CN"
        shot(make(homeComponent), "21-home-chinese")
        fixture.useLanguage("ja")
        Theme.language = "ja"
        shot(make(homeComponent), "26-home-japanese")
        fixture.useLanguage("en")
        Theme.language = "en"
    }
    // A frame of the board scene at a moment of its play.
    function frame(scene, name) {
        const image = grabImage(scene)
        image.save(out + name + ".png")
    }
    Component { id: sceneComponent; BoardScene { width: 1000; height: 620 } }
    function test_13_scene() {
        const scene = make(sceneComponent, {}, 1000, 620)
        const board = { rows: 4, columns: 4, faces: [], up: [],
                        tiles: ["S", "A", "T", "E", "P", "I", "N", "R", "O", "L", "D", "M", "U", "G", "H", "K"] }
        scene.show({ rows: 4, columns: 4, tiles: [], faces: [], up: [] })
        frame(scene, "26-scene-closed-box")
        scene.play(board, 4242)
        wait(190)
        frame(scene, "27-scene-shake")
        tryCompare(scene, "stage", "lid", 4000)
        wait(110)
        frame(scene, "28-scene-lid")
        tryCompare(scene, "stage", "drop", 2000)
        wait(220)
        frame(scene, "29-scene-drop")
        tryCompare(scene, "busy", false, 4000)
        frame(scene, "30-scene-settled")
    }
}
