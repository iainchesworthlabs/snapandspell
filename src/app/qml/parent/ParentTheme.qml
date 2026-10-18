pragma Singleton
import QtQuick

// The tokens the parent area's screens use. They are the values of
// docs/design/application.md ("Colour and contrast", "Touch targets", "Type"),
// under the names the Solo Play kit's Theme gives the same things, so that when
// the two are one these screens change `ParentTheme.` to `Theme.` and nothing
// else. A parent's screens are for an adult: targets are the adult 48 dp, and
// there is no motion but a progress that fills.
//
// Light and dark follow the system's scheme, as the chrome everywhere does
// (application.md, "Colour and contrast"); Main.qml keeps `dark` in step with
// the kit's Theme. The dark values are the kit's measured dark pairs under the
// parent area's token names, so the contrast ratios application.md holds for
// them carry over; the note chip is the pale one in both.
QtObject {
    // The Text size setting (1, 1.25, 1.5), set by ParentArea from the controller.
    property real textScale: 1.0

    // Whether the system asks for a dark chrome, set by Main from Theme.
    property bool dark: false

    // Hebrew and Yiddish: the parent area reads right to left. Set by ParentArea
    // from the shell, so the positioners and the bar follow it.
    property bool rightToLeft: false

    // --- colour -------------------------------------------------------------
    readonly property color page: dark ? "#12161C" : "#FBF8F1"
    readonly property color ink: dark ? "#F4EFE3" : "#1F2A37"
    readonly property color inkSecondary: dark ? "#B4BDC9" : "#4A5666"
    readonly property color outline: dark ? "#8C98A8" : "#5B6879"
    readonly property color buttonFill: dark ? "#7DB4EE" : "#1F5FA8"
    readonly property color buttonInk: dark ? "#0E1620" : "#FFFFFF"
    readonly property color hoverWash: dark ? "#27303B" : "#EDE6D6"
    readonly property color pressWash: dark ? "#323D4A" : "#E0D8C4"
    readonly property color card: dark ? "#263140" : "#FFFFFF"
    readonly property color panel: dark ? "#1D2733" : "#DCE6F2"
    readonly property color note: "#FFF1C2"
    readonly property color noteInk: "#1F2A37"
    readonly property color focusRing: dark ? "#7DB4EE" : "#1F5FA8"

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
