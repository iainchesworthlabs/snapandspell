pragma Singleton
import QtQuick

// The tokens the parent area's screens use. They are the values of
// docs/design/application.md ("Colour and contrast", "Touch targets", "Type"),
// under the names the Solo Play kit's Theme gives the same things, so that when
// the two are one these screens change `ParentTheme.` to `Theme.` and nothing
// else. A parent's screens are for an adult: targets are the adult 48 dp, and
// there is no motion but a progress that fills.
//
// The colours are the light theme's placeholders; the dark theme's values are
// not computed yet (application.md).
QtObject {
    // The Text size setting (1, 1.25, 1.5), set by ParentArea from the controller.
    property real textScale: 1.0

    // --- colour -------------------------------------------------------------
    readonly property color page: "#FBF8F1"
    readonly property color ink: "#1F2A37"
    readonly property color inkSecondary: "#4A5666"
    readonly property color outline: "#5B6879"
    readonly property color buttonFill: "#1F5FA8"
    readonly property color buttonInk: "#FFFFFF"
    readonly property color hoverWash: "#EDE6D6"
    readonly property color pressWash: "#E0D8C4"
    readonly property color card: "#FFFFFF"
    readonly property color panel: "#DCE6F2"
    readonly property color note: "#FFF1C2"
    readonly property color noteInk: "#1F2A37"
    readonly property color focusRing: "#1F5FA8"

    // --- sizes, in dp ---------------------------------------------------------
    readonly property real targetMin: 48
    readonly property real space1: 4
    readonly property real space2: 8
    readonly property real space3: 12
    readonly property real space4: 16
    readonly property real space6: 24
    readonly property real space8: 32
    readonly property real space12: 48
    readonly property real hair: 1
    readonly property real rule: 2
    readonly property real radius: 12
    readonly property real contentWidth: 760

    // --- type -------------------------------------------------------------------
    readonly property real sizeSmall: 15 * textScale
    readonly property real sizeBody: 18 * textScale
    readonly property real sizeLead: 22 * textScale
    readonly property real sizeTitle: 28 * textScale
    readonly property real sizeSound: 30 * textScale
}
