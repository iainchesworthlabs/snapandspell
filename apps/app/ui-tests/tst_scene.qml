// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import SnapAndSpell

// The board scene (apps/app/qml/solo/BoardScene.qml) as it is, not the stub: the pixel theatre of the
// shake and the drop. It plays the stages in order, says each die has landed once and the board is
// settled once, draws the dice where the flat board draws them, and with reduced motion it only fades.
// Its script is held by apps/app/tests/unit/scene_script_test.cpp; this is the part that is QML.
TestCase {
    id: testCase
    name: "Scene"
    when: windowShown
    width: 1000
    height: 700

    Component { id: sceneComponent; BoardScene { width: 1000; height: 700 } }

    function board(rows, columns) {
        const tiles = []
        for (let i = 0; i < rows * columns; ++i) {
            tiles.push(i === 2 ? "" : "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % 26])
        }
        return { rows: rows, columns: columns, tiles: tiles, faces: [], up: [] }
    }

    function allItems(root) {
        const found = [root]
        for (const child of root.children) {
            found.push(...allItems(child))
        }
        return found
    }

    function makeScene(properties) {
        const scene = createTemporaryObject(sceneComponent, testCase, properties || {})
        verify(scene !== null)
        return scene
    }

    // A SceneDie is the item that has `letters` and `dead`.
    function dice(scene) {
        return allItems(scene).filter(item => item.dead !== undefined && item.letters !== undefined)
    }

    function test_theBoxStartsWithItsLidOnAndNoDice() {
        const scene = makeScene()
        scene.show({ rows: 3, columns: 3, tiles: [], faces: [], up: [] })
        compare(scene.busy, false)
        compare(scene.boxClosed, true)
        compare(scene.lidOpen, 0)
        for (const die of dice(scene)) {
            compare(die.opacity, 0)
        }
    }

    function test_aBoardShownIsTheDiceAtRest() {
        const scene = makeScene()
        scene.show(board(4, 4))
        compare(scene.busy, false)
        compare(scene.rows, 4)
        compare(scene.columns, 4)
        compare(scene.lidOpen, 1)
        const rests = dice(scene)
        compare(rests.length, 16)
        for (const die of rests) {
            compare(die.opacity, 1)
        }
        // The geometry is the layout rule's: the tiles are square and fill the board rectangle.
        verify(scene.tileSize > 10)
        compare(scene.boardRect.width, 4 * scene.tileSize + 3 * scene.tileGap)
    }

    function test_aDeadTileIsAJewelNotALetter() {
        const scene = makeScene()
        scene.show(board(3, 3))
        const deads = dice(scene).filter(die => die.dead)
        compare(deads.length, 1)
    }

    function test_aPlayRunsTheStagesInOrderAndLandsEveryDieOnce() {
        const scene = makeScene()
        const settled = []
        const landed = []
        scene.settled.connect(() => settled.push(true))
        scene.landed.connect((index, pitch) => landed.push(index))
        const stages = []
        scene.onStageChanged.connect(() => { if (stages[stages.length - 1] !== scene.stage) stages.push(scene.stage) })
        scene.play(board(3, 3), 12345)
        compare(scene.busy, true)
        compare(scene.stage, "shake")
        tryCompare(scene, "busy", false, 8000)
        compare(settled.length, 1)
        compare(landed.length, 9)
        compare(landed.slice().sort((a, b) => a - b).join(), "0,1,2,3,4,5,6,7,8")
        compare(stages.join(), "shake,lid,drop,settle,idle")
    }

    function test_theBoxHopsInWholePixelsAndOnlyWhileItShakes() {
        const scene = makeScene()
        scene.play(board(3, 3), 7)
        const seen = {}
        let outside = 0
        const watch = () => {
            seen[scene.boxHop.x + "," + scene.boxHop.y] = true
            if (scene.boxHop.x !== Math.round(scene.boxHop.x) || scene.boxHop.y !== Math.round(scene.boxHop.y)) {
                outside++
            }
            if (scene.stage !== "shake" && (scene.boxHop.x !== 0 || scene.boxHop.y !== 0)) {
                outside++
            }
        }
        scene.boxHopChanged.connect(watch)
        tryCompare(scene, "busy", false, 8000)
        compare(outside, 0)
        verify(Object.keys(seen).length >= 3, "the box hopped to several places")
    }

    function test_theLidLiftsAwayAndTheDiceAppearOnlyWhenTheyFall() {
        const scene = makeScene()
        scene.play(board(3, 3), 3)
        tryCompare(scene, "stage", "lid", 4000)
        verify(scene.lidOpen < 1)
        for (const die of dice(scene)) {
            compare(die.opacity, 0)
        }
        tryCompare(scene, "stage", "drop", 2000)
        tryCompare(scene, "busy", false, 4000)
        compare(scene.lidOpen, 1)
        for (const die of dice(scene)) {
            compare(die.opacity, 1)
        }
    }

    function test_aShakeHeldByTheDeviceLastsLongerButNotForever() {
        const scene = makeScene()
        scene.pushMotion(0.5, 0.2, 1.4, 0, 0, 0)
        verify(scene.agitation > 0.05)
        scene.play(board(3, 3), 5)
        wait(1200)
        compare(scene.stage, "shake")
        scene.pushMotion(0, 0, 1, 0, 0, 0)
        compare(scene.agitation, 0)
        tryCompare(scene, "busy", false, 8000)
    }

    function test_motionNeverMovesWhereADieLands() {
        const a = makeScene()
        const b = makeScene()
        a.pushMotion(0.9, 0.9, 0.2, 1, 1, 1)
        a.play(board(3, 3), 99)
        b.play(board(3, 3), 99)
        tryCompare(a, "busy", false, 8000)
        tryCompare(b, "busy", false, 8000)
        compare(a.boardRect, b.boardRect)
        compare(dice(a).map(d => d.x).join(), dice(b).map(d => d.x).join())
        compare(dice(a).map(d => d.y).join(), dice(b).map(d => d.y).join())
    }

    function test_withReducedMotionTheBoxStaysStillAndTheDiceFadeIn() {
        const scene = makeScene({ reducedMotion: true })
        let hopped = 0
        scene.boxHopChanged.connect(() => { if (scene.boxHop.x !== 0 || scene.boxHop.y !== 0) hopped++ })
        const started = Date.now()
        scene.play(board(3, 3), 5)
        tryCompare(scene, "busy", false, 8000)
        const reduced = Date.now() - started
        compare(hopped, 0)
        compare(dice(scene)[0].opacity, 1)

        // 300 ms + 120 ms + 120 ms + the settle against the 2 s of the full play: measured against a
        // full play on the same machine, not against the clock, so a slow runner does not fail it.
        const full = makeScene()
        const fullStarted = Date.now()
        full.play(board(3, 3), 5)
        tryCompare(full, "busy", false, 12000)
        verify(reduced < (Date.now() - fullStarted) * 0.75, reduced + " ms reduced, " + (Date.now() - fullStarted) + " ms full")
    }

    function test_aSecondPlayRestartsTheFirst() {
        const scene = makeScene()
        const settled = []
        scene.settled.connect(() => settled.push(true))
        scene.play(board(3, 3), 1)
        wait(300)
        scene.play(board(3, 3), 2)
        compare(scene.stage, "shake")
        tryCompare(scene, "busy", false, 8000)
        compare(settled.length, 1)
    }

    function test_theRattleIsSignalledAtMostEightTimesASecond() {
        const scene = makeScene()
        let rattles = 0
        scene.rattled.connect(() => rattles++)
        const started = Date.now()
        scene.play(board(3, 3), 8)
        tryCompare(scene, "stage", "lid", 6000)
        const seconds = (Date.now() - started) / 1000
        verify(rattles >= 1)
        verify(rattles <= Math.ceil(seconds * 8) + 1, rattles + " rattles in " + seconds + " s")
    }
}
