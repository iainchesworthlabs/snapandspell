import QtQuick
import QtTest
import WordJewelsSolver

// Home (src/app/qml/Home.qml): the box, the family's tablet once its screen
// exists, and the small corner button for a grown-up. A picture and not a word,
// so what is checked is which pictures are there, that pressing one asks for its
// screen, that the box has the keyboard, and that Back does nothing here.
TestCase {
    id: testCase
    name: "Home"
    when: windowShown
    // Quick Test's own root is invisible; a screen created under it needs the
    // test window visible for pointer events and focus.
    visible: true

    width: 900
    height: 600

    Component { id: homeComponent; Home { } }

    SignalSpy { id: soloSpy; signalName: "soloRequested" }
    SignalSpy { id: scanSpy; signalName: "scanRequested" }
    SignalSpy { id: parentSpy; signalName: "parentRequested" }

    function init() {
        // A QML warning in a test function fails it, as one outside does the
        // whole run: a screen that warned is a screen that is wrong.
        failOnWarning(/.?/)
        fixture.reset()
        Navigation.home()
        Navigation.scanAvailable = true
        Navigation.parentAvailable = true
        Theme.rightToLeft = false
    }

    function cleanup() {
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
        return home
    }

    function test_theBoxTheFamilyAndTheGrownUpsButton() {
        const screen = home()

        compare(screen.pictures, 2)
        const box = picture(screen, HomeWording.soloPicture)
        const family = picture(screen, HomeWording.familyPicture)
        const grownUps = picture(screen, HomeWording.forGrownUps)
        verify(box !== null)
        verify(family !== null)
        verify(grownUps !== null)
        compare(box.visible, true)
        compare(family.visible, true)
        compare(grownUps.visible, true)
        // The pictures are big touch targets, at least the primary target:
        verify(box.width >= Theme.targetPrimary)
        verify(box.height >= Theme.targetPrimary)
        // The grown-ups' button is the adult 48 dp in the corner.
        compare(grownUps.targetHeight, Theme.targetChild)
    }

    function test_theFamilyPictureShowsOnlyOnceItsScreenExists() {
        // Navigation.scanAvailable is the shell's switch: a picture that does
        // nothing is not shown.
        Navigation.scanAvailable = false
        const screen = home()

        compare(screen.pictures, 1)
        compare(picture(screen, HomeWording.familyPicture).visible, false)
        compare(picture(screen, HomeWording.soloPicture).visible, true)
    }

    function test_eachPictureAsksForItsScreen() {
        const screen = home()
        soloSpy.target = screen
        scanSpy.target = screen
        parentSpy.target = screen
        soloSpy.clear()
        scanSpy.clear()
        parentSpy.clear()

        const box = picture(screen, HomeWording.soloPicture)
        mouseClick(box, box.width / 2, box.height / 2)
        compare(soloSpy.count, 1)
        compare(scanSpy.count, 0)
        compare(parentSpy.count, 0)

        const family = picture(screen, HomeWording.familyPicture)
        mouseClick(family, family.width / 2, family.height / 2)
        compare(scanSpy.count, 1)
        compare(soloSpy.count, 1)

        const grownUps = picture(screen, HomeWording.forGrownUps)
        mouseClick(grownUps, grownUps.width / 2, grownUps.height / 2)
        compare(parentSpy.count, 1)
    }

    function test_theBoxHasTheKeyboardAndSpaceOpensIt() {
        const screen = home()
        soloSpy.target = screen
        soloSpy.clear()

        screen.forceActiveFocus()
        const box = picture(screen, HomeWording.soloPicture)
        tryCompare(box, "activeFocus", true)

        keyClick(Qt.Key_Space)

        compare(soloSpy.count, 1)
    }

    function test_backOnHomeDoesNothing() {
        // The shell's Back is Main.qml's (Esc and the system back do what the
        // visible Back does, and on home they do nothing). Home itself has no
        // deeper state to undo, so a key it does not use changes nothing: no
        // screen is asked for and the navigation stack stays empty.
        const screen = home()
        soloSpy.target = screen
        scanSpy.target = screen
        parentSpy.target = screen
        soloSpy.clear()
        scanSpy.clear()
        parentSpy.clear()
        screen.forceActiveFocus()

        keyClick(Qt.Key_Escape)
        keyClick(Qt.Key_Back)

        compare(Navigation.screen, "home")
        compare(Navigation.history.length, 0)
        compare(soloSpy.count, 0)
        compare(scanSpy.count, 0)
        compare(parentSpy.count, 0)
    }

    function test_thePicturesFollowTheReadingDirection() {
        // Home's Grid is mirrored, so the box is on the leading side: left in
        // English, right in Hebrew. The pictures themselves are not mirrored
        // (their drawings are not words), only their places.
        const screen = home()
        const box = picture(screen, HomeWording.soloPicture)
        const family = picture(screen, HomeWording.familyPicture)
        verify(box.x < family.x)

        Theme.rightToLeft = true
        tryVerify(() => box.x > family.x)
    }
}
