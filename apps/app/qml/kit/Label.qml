// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import WordJewelsSolver

// The one text of the application. A screen says what a piece of text IS (its
// `role`) and the kit decides how it looks: face, size, weight, line height,
// tracking and colour all come from Theme, scale with the parent's Text size
// setting, and cannot be forgotten - a bare Text falls back to the system face.
//
// Toybox (docs/design/toybox/DESIGN.md, section 4): the chrome is Pixelify Sans, the face a child
// reads is Andika.
//
//   display  Pixelify Sans Bold 56                  the home hero, the cleared summary
//   title    Pixelify Sans Bold 34                  a screen's title
//   heading  Pixelify Sans Bold 24                  a card's or section's title
//   child    Pixelify Sans Bold 26                  a child's button
//   label    Pixelify Sans Medium 16                a parent's button
//   body     Andika 18                              a sentence
//   word     Andika 24                              a found word, a hint
//   caption  Andika 14                              a hint, a time stamp
//   numeral  Andika Bold 24, tabular figures        every number: points, counts, sizes, order badges
//   eyebrow  Silkscreen 16, capitals, 0.08 em       a section's label, board size
//
// Pixelify Sans is hard to read for a beginning reader (its O and 0 match, its 2, 3 and 5 read as Z, 8 and
// S at small sizes), so a numeral and anything a child reads is Andika. `learning: true` sets a label in
// Andika Bold for a sentence a child reads. A decorative Label (an icon's twin, a repeated name) is kept
// out of the accessibility tree.
Text {
    id: root

    property string role: "body"
    property bool learning: false
    property bool decorative: false
    // Letter spacing in em; the role's own unless set. Positive tracking is
    // ignored in a right-to-left language.
    property real tracking: spec.tracking
    // The size in pixels; set this, not font.pixelSize, to override the role's (the
    // tracking is in em of it, and reading font.pixelSize back would loop).
    property real size: spec.size
    // Line height as a multiple of the size. Qt's proportional mode multiplies the
    // font's own line spacing (Andika's is about 1.4 em), so the height is fixed in
    // pixels instead: 24 px of type at 1.5 is a 36 px line.
    property real lineRatio: spec.line

    readonly property var spec: specFor(root.role, root.learning)
    readonly property bool isHeading: root.role === "display" || root.role === "title" || root.role === "heading"

    function specFor(role, learning) {
        switch (role) {
        case "display":
            return {face: Theme.uiFont, size: Theme.displaySize, weight: Font.Bold, line: Theme.lineDisplay,
                    tracking: Theme.trackDisplay, caps: Font.MixedCase, color: Theme.ink};
        case "title":
            return {face: Theme.uiFont, size: Theme.titleSize, weight: Font.Bold, line: Theme.lineTitle,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.ink};
        case "heading":
            return {face: Theme.uiFont, size: Theme.headingSize, weight: Font.Bold, line: Theme.lineHeading,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.ink};
        case "label":
            return {face: learning ? Theme.fontFamily : Theme.uiFont, size: Theme.labelSize,
                    weight: learning ? Font.Bold : Font.Medium, line: Theme.lineHeading, tracking: Theme.trackNone,
                    caps: Font.MixedCase, color: Theme.ink};
        case "child":
            return {face: Theme.uiFont, size: Theme.childLabelSize, weight: Font.Bold, line: Theme.lineChild,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.ink};
        case "word":
            return {face: Theme.fontFamily, size: Theme.wordSize, weight: Font.Normal, line: Theme.lineWord,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.ink};
        case "numeral":
            return {face: Theme.fontFamily, size: Theme.wordSize, weight: Font.Bold, line: Theme.lineWord,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.ink};
        case "eyebrow":
            return {face: Theme.eyebrowFont, size: Theme.monoSize, weight: Font.Normal, line: Theme.lineMono,
                    tracking: Theme.trackMono, caps: Font.AllUppercase, color: Theme.inkSecondary};
        case "caption":
            return {face: Theme.fontFamily, size: Theme.captionSize, weight: Font.Normal, line: Theme.lineCaption,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.inkSecondary};
        default:
            return {face: Theme.fontFamily, size: Theme.bodySize, weight: Font.Normal, line: Theme.lineBody,
                    tracking: Theme.trackNone, caps: Font.MixedCase, color: Theme.ink};
        }
    }

    color: spec.color
    font.family: spec.face
    font.pixelSize: root.size
    font.weight: spec.weight
    font.capitalization: spec.caps
    font.features: root.role === "numeral" ? {"tnum": 1} : ({})
    font.letterSpacing: (root.tracking > 0 && Theme.rightToLeft) ? 0 : root.tracking * root.size
    lineHeight: root.lineRatio * root.size
    lineHeightMode: Text.FixedHeight

    Accessible.ignored: root.decorative
    Accessible.role: root.isHeading ? Accessible.Heading : Accessible.StaticText
    Accessible.name: root.text
}
