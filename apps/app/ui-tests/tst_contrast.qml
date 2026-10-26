// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import SnapAndSpell

// The design's contrast gates, computed from the real Theme tokens in both
// schemes (WCAG 2.x relative luminance): text 4.5:1, non-text 3:1, and the
// separations that tell a card from the paper and the tray from its stage.
// Nothing here is a copy of a colour: a token that moves is measured again.
//
// The gates (docs/design/application.md, "Colour and contrast"):
//   text            4.5   ink and secondary ink on every surface they sit on
//   large / disabled 3.0  a disabled label is exempt from WCAG, the house floor is 3
//   non-text        3.0   a control's edge, the focus ring, the selected edge, a
//                         track, the gold of the path and the hint against the die
//   separation      1.1 - 1.6, measured on the pair named: line on card 1.6,
//                         tray on stage 1.5, panel on stage 1.1
TestCase {
    id: root
    name: "Contrast"
    when: windowShown

    function init() {
        failOnWarning(/.?/)
        Theme.textScale = 1.0
        Theme.reducedMotion = false
        Theme.rightToLeft = false
        Theme.colourScheme = "system"
    }

    function cleanup() {
        init()
    }

    function channel(c) {
        return c <= 0.03928 ? c / 12.92 : Math.pow((c + 0.055) / 1.055, 2.4);
    }

    function luminance(c) {
        return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b);
    }

    function ratio(a, b) {
        const la = luminance(a);
        const lb = luminance(b);
        return (Math.max(la, lb) + 0.05) / (Math.min(la, lb) + 0.05);
    }

    // [foreground token, background token, least ratio, what it is]
    readonly property var gates: [
        // Text.
        ["ink", "page", 4.5, "ink on the page"],
        ["ink", "card", 4.5, "ink on a card"],
        ["ink", "panel", 4.5, "ink on the panel"],
        ["ink", "surface", 4.5, "ink on the bar"],
        ["ink", "boardArea", 4.5, "ink on the stage"],
        ["ink", "hoverWash", 4.5, "ink on a hovered control"],
        ["ink", "pressWash", 4.5, "ink on a pressed control"],
        ["inkSecondary", "page", 4.5, "secondary ink on the page"],
        ["inkSecondary", "card", 4.5, "secondary ink on a card"],
        ["inkSecondary", "panel", 4.5, "secondary ink on the panel"],
        ["inkSecondary", "surface", 4.5, "secondary ink on the bar"],
        ["inkSecondary", "boardArea", 4.5, "secondary ink on the stage"],
        ["inkSecondary", "hoverWash", 4.5, "secondary ink on a hovered control"],
        ["chipInk", "accentSoft", 4.5, "a chip's text on its sunshine fill"],
        ["accentText", "page", 4.5, "emphasised text on the page"],
        ["accentText", "card", 4.5, "emphasised text on a card"],
        ["accentText", "panel", 4.5, "emphasised text on the panel"],
        ["ink", "surfaceDim", 4.5, "ink on the dimmer surface (a disabled fill, an empty well)"],
        ["inkSecondary", "surfaceDim", 4.5, "secondary ink on the dimmer surface"],
        ["dieInk", "leaf", 4.5, "ink on a valid tile"],
        ["cream", "tealLight", 4.5, "a label on the board's well"],
        ["cream", "deepPurple", 4.5, "cream on the deepest purple"],
        ["buttonInk", "buttonFill", 4.5, "label on the primary button"],
        ["selectedInk", "selectedFill", 4.5, "label on a chosen option"],
        ["dangerInk", "dangerFill", 4.5, "label on the destructive button"],
        ["goldInk", "card", 4.5, "emphasised reward text on a card"],
        ["hintCardInk", "hintCard", 4.5, "ink on the hint card"],
        ["dieInk", "die", 7.0, "letters on a die (the content: 7:1)"],
        ["dieLitInk", "dieLit", 4.5, "letters on a lit die"],

        // A disabled control's label: exempt from WCAG, 3:1 in the house.
        ["disabledInk", "disabledFill", 3.0, "a disabled button's label"],
        ["disabledInk", "page", 3.0, "a disabled ghost button's label on the page"],
        ["disabledInk", "card", 3.0, "a disabled ghost button's label on a card"],

        // Non-text: what a person has to see to use the thing.
        ["outline", "page", 3.0, "a control's edge on the page"],
        ["outline", "card", 3.0, "a control's edge on a card"],
        ["outline", "panel", 3.0, "a control's edge on the panel"],
        ["focusRing", "page", 3.0, "the focus ring on the page"],
        ["focusRing", "card", 3.0, "the focus ring on a card"],
        ["focusRing", "panel", 3.0, "the focus ring on the panel"],
        ["focusRing", "boardArea", 3.0, "the focus ring on the stage"],
        ["selectedEdge", "card", 3.0, "a chosen option's edge on a card"],
        ["selectedEdge", "page", 3.0, "a chosen option's edge on the page"],
        ["danger", "card", 3.0, "a destructive button's pink outline on a card"],
        ["outline", "track", 3.0, "an empty progress step's edge against its fill"],
        ["lid", "tray", 3.0, "the start tile's cream ring on the tray"],
        ["ribbonEdge", "die", 3.0, "the path's ink dashes on a die"],
        ["ribbonEdge", "dieLit", 3.0, "the path's ink dashes on a lit die"],
        ["ribbonCore", "ribbonEdge", 3.0, "the path's cream centre against its ink dashes"],
        ["ribbonLine", "die", 3.0, "the path's outline on a die"],
        ["ribbonLine", "dieLit", 3.0, "the path's outline on a lit die"],
        ["dieInk", "ribbonCore", 7.0, "a letter over the path's core"],
        ["die", "tray", 3.0, "a die on the tray"],
        ["dieEdge", "die", 3.0, "a die's edge"],
        ["jewelDead", "card", 3.0, "a jewel not yet found, on a card"],
        ["jewelDead", "tray", 3.0, "a jewel not yet found, on the tray"],

        // Separation: the board is told from its stage, a card from the paper.
        ["line", "card", 1.6, "the edge of a card against the card"],
        ["line", "page", 1.5, "the edge of a card against the page"],
        ["line", "panel", 1.4, "the edge of a card on the panel"],
        ["trayEdge", "boardArea", 3.0, "the tray's frame against its stage"],
        ["panel", "boardArea", 1.1, "the panel against the stage"]
    ]

    function test_gates_data() {
        const rows = [];
        for (const scheme of ["light", "dark"]) {
            for (const gate of root.gates) {
                rows.push({tag: scheme + ": " + gate[3], scheme: scheme, fg: gate[0], bg: gate[1], least: gate[2]});
            }
        }
        return rows;
    }

    function test_gates(data) {
        Theme.colourScheme = data.scheme;
        verify(Theme[data.fg] !== undefined, "the token " + data.fg + " exists");
        verify(Theme[data.bg] !== undefined, "the token " + data.bg + " exists");
        const measured = root.ratio(Theme[data.fg], Theme[data.bg]);
        verify(measured >= data.least,
               data.tag + ": " + data.fg + " " + Theme[data.fg] + " on " + data.bg + " " + Theme[data.bg]
               + " is " + measured.toFixed(2) + ":1, the gate is " + data.least + ":1");
    }

    // The disabled look must not be made by opacity: a disabled primary button
    // keeps its own fill and ink.
    function test_aDisabledButtonUsesTokensNotOpacity() {
        const button = createTemporaryObject(buttonComponent, root, {label: "Done", variant: "primary", enabled: false});
        verify(button !== null);
        compare(button.opacity, 1.0);
        compare(button.contentColor.toString(), Theme.disabledInk.toString());
    }

    Component { id: buttonComponent; AppButton { } }
}
