// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import SnapAndSpell
import "SizeMatrix.js" as SizeMatrix

// The size matrix's audit held to cases: it must find the things it is there to
// find (a control under 48 dp, one clipped by its parent or the window, a
// truncated label, two controls on top of each other, a page that scrolls
// sideways, a text past the edge of its card or the window) and nothing in a screen
// that is fine, so a green matrix means something.
TestCase {
    id: testCase
    name: "SizeMatrixAudit"
    when: windowShown
    visible: true
    width: 800
    height: 600

    Component {
        id: fineComponent
        Item {
            width: 400
            height: 300
            AppButton { x: 10; y: 10; size: "min"; label: "Fine" }
            Flickable {
                x: 10; y: 100; width: 300; height: 150
                contentWidth: 300; contentHeight: 600
                clip: true
                // Off the end of a vertical scroller is scrolling, not clipping.
                AppButton { x: 0; y: 400; size: "min"; label: "Scrolled to" }
            }
            // A text below the fold of a scroller is read by scrolling; a short text in a wide label is fine.
            Flickable {
                x: 10; y: 260; width: 300; height: 30
                contentWidth: 300; contentHeight: 600
                clip: true
                Label { x: 0; y: 400; width: 280; text: "Scrolled text" }
            }
            Label { x: 10; y: 70; width: 380; text: "Short" }
            // A list that scrolls sideways on purpose.
            Flickable {
                x: 10; y: 300; width: 300; height: 0
                contentWidth: 900; contentHeight: 10
                flickableDirection: Flickable.HorizontalFlick
            }
        }
    }

    Component {
        id: sidewaysComponent
        Item {
            width: 400
            height: 300
            Flickable {
                x: 10; y: 10; width: 300; height: 200
                contentWidth: 500; contentHeight: 600
                clip: true
                flickableDirection: Flickable.VerticalFlick
            }
        }
    }

    Component {
        id: textOutComponent
        Item {
            width: 400
            height: 300
            Label { x: 330; y: 10; text: "A text that runs off the window" }
            Item {
                x: 10; y: 100; width: 120; height: 40
                clip: true
                Label { x: 0; y: 30; text: "Cut off below" }
            }
        }
    }

    Component {
        id: smallComponent
        Item {
            width: 400
            height: 300
            AppButton { x: 10; y: 10; width: 30; height: 30; size: "min"; label: "" }
        }
    }

    Component {
        id: clippedComponent
        Item {
            width: 400
            height: 300
            Item {
                x: 10; y: 10; width: 100; height: 100
                clip: true
                AppButton { x: 60; y: 10; size: "min"; label: "Clipped" }
            }
            AppButton { x: 380; y: 250; size: "min"; label: "Off the window" }
        }
    }

    Component {
        id: truncatedComponent
        Item {
            width: 400
            height: 300
            AppButton { x: 10; y: 10; width: 60; height: 48; size: "min"; wrap: false; label: "A long label indeed" }
        }
    }

    Component {
        id: overlapComponent
        Item {
            width: 400
            height: 300
            AppButton { x: 10; y: 10; size: "min"; label: "One" }
            AppButton { x: 20; y: 20; size: "min"; label: "Two" }
        }
    }

    function init() {
        failOnWarning(/.?/)
        Theme.textScale = 1.0
    }

    function audit(component) {
        const item = createTemporaryObject(component, testCase)
        wait(50)
        return SizeMatrix.audit(item, 48)
    }

    function test_aFineScreenHasNoFindings() {
        compare(audit(fineComponent).length, 0)
    }

    function test_aControlUnder48IsFound() {
        const found = audit(smallComponent)
        verify(found.some(f => f.indexOf("below 48") >= 0), found.join(" | "))
    }

    function test_aClippedControlAndOneOffTheWindowAreFound() {
        const found = audit(clippedComponent)
        verify(found.some(f => f.indexOf("Clipped") >= 0 && f.indexOf("clipped by") >= 0), found.join(" | "))
        verify(found.some(f => f.indexOf("Off the window") >= 0 && f.indexOf("clipped by") >= 0), found.join(" | "))
    }

    function test_aTruncatedLabelIsFound() {
        const found = audit(truncatedComponent)
        verify(found.some(f => f.indexOf("truncated") >= 0), found.join(" | "))
    }

    function test_twoControlsOnTopOfEachOtherAreFound() {
        const found = audit(overlapComponent)
        verify(found.some(f => f.indexOf("overlaps") >= 0), found.join(" | "))
    }

    function test_aPageThatScrollsSidewaysIsFound() {
        const found = audit(sidewaysComponent)
        verify(found.some(f => f.indexOf("content is 500 wide in a 300 wide view") >= 0), found.join(" | "))
    }

    function test_aTextPastTheWindowOrItsCardIsFound() {
        const found = audit(textOutComponent)
        verify(found.some(f => f.indexOf("A text that runs off") >= 0 && f.indexOf("text outside") >= 0), found.join(" | "))
        verify(found.some(f => f.indexOf("Cut off below") >= 0 && f.indexOf("text outside") >= 0), found.join(" | "))
    }

    function test_theMatrixHoldsThePhonesAndTheTablets() {
        const names = SizeMatrix.sizes.map(s => s.w + "x" + s.h + "@" + s.scale)
        // An iPhone upright and on its side, the smallest phones, an iPad both ways, and big text on a phone.
        for (const wanted of ["390x844@1", "844x390@1", "375x667@1", "360x640@1", "667x375@1", "820x1180@1",
                              "1180x820@1", "390x844@1.5", "844x390@1.5"]) {
            verify(names.indexOf(wanted) >= 0, wanted)
        }
        // The desktop list is the one a screen not yet laid out for them is held to, and is the first part of the whole.
        compare(SizeMatrix.desktopSizes.length, 6)
        verify(SizeMatrix.sizes.length > SizeMatrix.desktopSizes.length)
        for (let i = 0; i < SizeMatrix.desktopSizes.length; ++i) {
            compare(SizeMatrix.sizes[i].w, SizeMatrix.desktopSizes[i].w)
        }
    }

    function test_theMatrixIsTheDesignsSizes() {
        const names = SizeMatrix.sizes.map(s => s.w + "x" + s.h + "@" + s.scale)
        for (const wanted of ["900x620@1", "900x1200@1", "1024x768@1", "2400x1200@1", "900x620@1.5"]) {
            verify(names.indexOf(wanted) >= 0, wanted)
        }
    }
}
