pragma Singleton
import QtQuick

// The parent area's view of the one design system. The values mirror
// kit/Theme.qml exactly; the names are the ones the parent screens already use.
// When the two modules are merged this file can disappear without a screen
// changing a value. A parent's targets are 48 dp, and the type is the same
// Archivo + IBM Plex Mono + Andika family as the child area.
QtObject {
    property real textScale: 1.0
    property bool dark: false
    property bool rightToLeft: false

    // --- colour -------------------------------------------------------------
    readonly property color page: dark ? "#11181B" : "#F6F1E7"
    readonly property color surface: dark ? "#1A2428" : "#FFFDF8"
    readonly property color card: dark ? "#1E2A2F" : "#FFFDF8"
    readonly property color panel: dark ? "#1A2428" : "#EDE7DA"
    readonly property color ink: dark ? "#F2ECDE" : "#1E2A30"
    readonly property color inkSecondary: dark ? "#B3BDBB" : "#5C6B70"
    readonly property color line: dark ? "#3A474B" : "#D9D1C2"
    readonly property color outline: dark ? "#8C9A98" : "#6E7A78"
    readonly property color accent: dark ? "#63C7B2" : "#166B5C"
    readonly property color buttonFill: accent
    readonly property color buttonInk: dark ? "#0E1F1C" : "#FFFFFF"
    readonly property color hoverWash: dark ? "#243136" : "#EDE6D6"
    readonly property color pressWash: dark ? "#2E3D42" : "#E0D8C4"
    readonly property color note: dark ? "#3A2E1A" : "#F7E8C6"
    readonly property color noteInk: dark ? "#F2ECDE" : "#1E2A30"
    readonly property color focusRing: accent
    readonly property color gold: "#C98A1E"
    readonly property color danger: dark ? "#F08A78" : "#A33A2B"

    // --- sizes, in dp -------------------------------------------------------
    readonly property real targetMin: 48
    readonly property real space1: 4
    readonly property real space2: 8
    readonly property real space3: 12
    readonly property real space4: 16
    readonly property real space6: 24
    readonly property real space8: 32
    readonly property real space12: 48
    readonly property real hair: 1
    readonly property real rule: 1
    readonly property real radius: 12
    readonly property real contentWidth: 860

    // --- type ---------------------------------------------------------------
    readonly property string uiFont: "Archivo"
    readonly property string fontFamily: "Andika"
    readonly property string monoFont: "IBM Plex Mono"
    readonly property real sizeSmall: 14 * textScale
    readonly property real sizeBody: 16 * textScale
    readonly property real sizeLead: 20 * textScale
    readonly property real sizeTitle: 30 * textScale
    readonly property real sizeSound: 30 * textScale
}
