// SPDX-License-Identifier: GPL-3.0-or-later
.pragma library

// The size matrix: the window sizes and text scales a screen of the application
// is held to, and the audit that says what is wrong with a laid-out item tree.
// SizeMatrixCase.qml runs a screen through the matrix; this file is the part
// that reads the tree, so a suite can call it on its own as well.
//
// The sizes are the design's own. The desktop: the 900 x 620 minimum window, a
// 4:3 window (1024 x 768), a tall one (900 x 1200) and a big monitor
// (2400 x 1200). The tablets: an iPad held upright (820 x 1180) and on its side
// (1180 x 820). The phones: an iPhone upright (390 x 844), a small one (375 x
// 667), the smallest Android we hold to (360 x 640) and the iPhone on its side
// (844 x 390, and 667 x 375 for the small one). And 150% text on the smallest
// of each kind. A phone at the normal text size (`onePage`) is also held to the rule of its play
// screens: all of the screen is in view, nothing is left to scroll down to (`scrollingPages`).

var desktopSizes = [
    {w: 900, h: 620, scale: 1.0},
    {w: 900, h: 1200, scale: 1.0},
    {w: 1024, h: 768, scale: 1.0},
    {w: 2400, h: 1200, scale: 1.0},
    {w: 900, h: 620, scale: 1.5},
    {w: 1024, h: 768, scale: 1.5}
];

// Every shape. A suite whose screen has not been laid out for the tablets and
// the phones yet names `desktopSizes` instead (SizeMatrixCase.sizes), with the
// stage that fixes it, and the line comes out when it does.
var sizes = desktopSizes.concat([
    {w: 820, h: 1180, scale: 1.0},
    {w: 1180, h: 820, scale: 1.0},
    {w: 390, h: 844, scale: 1.0, onePage: true},
    {w: 375, h: 667, scale: 1.0, onePage: true},
    {w: 360, h: 640, scale: 1.0, onePage: true},
    {w: 844, h: 390, scale: 1.0, onePage: true},
    {w: 667, h: 375, scale: 1.0, onePage: true},
    {w: 390, h: 844, scale: 1.5},
    {w: 844, h: 390, scale: 1.5}
]);

var epsilon = 0.6;

function typeName(item) {
    return item.toString().replace(/\(0x[0-9a-f]+\)$/i, "").replace(/_QMLTYPE_\d+$/, "")
                          .replace(/_QML_\d+$/, "");
}

function nameOf(item) {
    const named = item.accessibleName || item.label || item.text || item.title || item.objectName || "";
    return typeName(item) + (named ? " \"" + named + "\"" : "");
}

function isMouseArea(item) {
    return typeName(item).indexOf("MouseArea") >= 0;
}

// An item a person can press: focusable on Tab (buttons, cards, rows made
// focusable) or a MouseArea of its own that no interactive parent already is.
function isInteractive(item) {
    if (item.activeFocusOnTab === true) {
        return true;
    }
    return false;
}

function isScrollContent(parent, child) {
    return parent.contentItem !== undefined && parent.contentItem === child;
}

function walk(item, visit) {
    visit(item);
    for (let i = 0; i < item.children.length; ++i) {
        walk(item.children[i], visit);
    }
}

// The sheets that are open under `root`. A sheet is modal: it traps the Tab focus and its scrim takes
// the clicks (Sheet.qml, tst_kit), so while one is open the controls behind it are not reachable and
// only its own are held to the audit. They are audited in the states where no sheet is open.
function openSheets(root) {
    const sheets = [];
    walk(root, function (it) {
        if (it !== root && it.visible && it.open === true && typeName(it) === "Sheet") {
            sheets.push(it);
        }
    });
    return sheets;
}

function isInside(item, ancestor) {
    for (let at = item; at; at = at.parent) {
        if (at === ancestor) {
            return true;
        }
    }
    return false;
}

function truncatedDescendants(item) {
    const found = [];
    walk(item, function (it) {
        if (it !== item && it.truncated === true && it.visible) {
            found.push(it);
        }
    });
    return found;
}

function isFlickable(item) {
    return item.contentItem !== undefined && item.contentWidth !== undefined
           && item.flickableDirection !== undefined;
}

// A text a person is meant to read: a visible, non-empty Text under `root`.
function isReadable(item) {
    return typeof item.text === "string" && item.text.length > 0
           && item.contentWidth !== undefined && item.lineCount !== undefined;
}

// The problems with the interactive items under `root`, as readable strings:
//   - smaller than `min` (default 48) in either direction;
//   - not wholly inside the viewport `root`, or inside an ancestor that clips
//     (a Flickable's content may run off the end of its scrolling direction);
//   - holding a label that is truncated ("Cl...");
//   - overlapping another interactive item that is not its ancestor or child, where both can be seen.
// While a sheet is open only the controls inside it are audited (openSheets).
function audit(root, min) {
    const floor = min === undefined ? 48 : min;
    const found = [];
    const items = [];
    const sheets = openSheets(root);
    const reachable = function (item) {
        return sheets.length === 0 || sheets.some(function (sheet) { return isInside(item, sheet); });
    };

    walk(root, function (item) {
        if (item === root || !item.visible || item.opacity <= 0 || item.width <= 0 || item.height <= 0) {
            return;
        }
        if (isInteractive(item) && reachable(item)) {
            items.push(item);
        }
    });

    // Small MouseAreas that are not the face of an interactive item and not a
    // backdrop behind some (a sheet's click-eater is a MouseArea over its buttons).
    const focusables = items.slice();
    walk(root, function (item) {
        if (item === root || !isMouseArea(item) || !item.visible || !item.enabled
                || item.width <= 0 || item.height <= 0 || !reachable(item)) {
            return;
        }
        for (let at = item.parent; at; at = at.parent) {
            if (focusables.indexOf(at) >= 0) {
                return;
            }
        }
        const here = item.mapToItem(root, 0, 0);
        for (const other of focusables) {
            const there = other.mapToItem(root, 0, 0);
            if (there.x >= here.x && there.y >= here.y
                    && there.x + other.width <= here.x + item.width + epsilon
                    && there.y + other.height <= here.y + item.height + epsilon) {
                return;
            }
        }
        if (item.width < root.width * 0.5 && item.height < root.height * 0.5) {
            items.push(item);
        }
    });

    const rects = [];
    for (const item of items) {
        const at = item.mapToItem(root, 0, 0);
        // Two controls overlap where both can be seen: the part of a control that a clipping ancestor
        // (a Flickable) hides is scrolled away, not under its neighbour.
        let left = at.x;
        let top = at.y;
        let right = at.x + item.width;
        let bottom = at.y + item.height;
        for (let up = item.parent; up && up !== root; up = up.parent) {
            if (up.clip === true) {
                const p = up.mapToItem(root, 0, 0);
                left = Math.max(left, p.x);
                top = Math.max(top, p.y);
                right = Math.min(right, p.x + up.width);
                bottom = Math.min(bottom, p.y + up.height);
            }
        }
        const rect = {item: item, x: left, y: top, w: Math.max(0, right - left), h: Math.max(0, bottom - top)};
        rects.push(rect);
        const who = nameOf(item) + " at " + Math.round(at.x) + "," + Math.round(at.y)
                  + " " + Math.round(item.width) + "x" + Math.round(item.height);

        if (item.width < floor - epsilon || item.height < floor - epsilon) {
            found.push(who + ": below " + floor + " dp");
        }

        // Walk up: every clipping ancestor, and the viewport, must contain it.
        let scrolls = false;
        let sideways = false;
        let child = item;
        for (let at2 = item.parent; at2; child = at2, at2 = at2.parent) {
            if (isScrollContent(at2, child)) {
                scrolls = true;
                // A row of cards or words that scrolls sideways (Home's pager) runs off its sides by nature.
                sideways = sideways || at2.contentWidth > at2.width + epsilon;
            }
            const clips = at2 === root || at2.clip === true || at2.contentItem !== undefined;
            if (!clips) {
                continue;
            }
            const p = item.mapToItem(at2, 0, 0);
            const bound = at2 === root ? at2 : at2;
            const outRight = p.x + item.width > bound.width + epsilon;
            const outLeft = p.x < -epsilon;
            const outBottom = p.y + item.height > bound.height + epsilon;
            const outTop = p.y < -epsilon;
            if (scrolls && at2.contentItem === undefined && at2 !== root) {
                // Above a Flickable the content is wherever it is scrolled to.
            }
            if ((!sideways && (outRight || outLeft)) || (!scrolls && (outBottom || outTop))) {
                found.push(who + ": clipped by " + typeName(at2));
                break;
            }
        }

        for (const label of truncatedDescendants(item)) {
            found.push(who + ": label truncated (\"" + label.text + "\")");
        }
    }

    // A page does not scroll sideways: a Flickable that is not a horizontal list is as wide as its
    // content (a phone has no room for a row that is wider than the screen).
    // HorizontalFlick is 1 in Flickable.FlickableDirection.
    walk(root, function (item) {
        if (item === root || !item.visible || !isFlickable(item) || item.flickableDirection === 1) {
            return;
        }
        if (item.contentWidth > item.width + epsilon && reachable(item)) {
            found.push(nameOf(item) + ": content is " + Math.round(item.contentWidth)
                       + " wide in a " + Math.round(item.width) + " wide view");
        }
    });

    // A text is wholly inside the window, and inside what clips it, across: it may be below the
    // fold of a Flickable, but never past its sides, and a text outside the window altogether
    // (not in a scrolled view) is lost.
    walk(root, function (item) {
        if (item === root || !item.visible || item.opacity <= 0 || !isReadable(item) || !reachable(item)) {
            return;
        }
        if (item.width <= 0 || item.height <= 0) {
            return;
        }
        let shown = true;
        for (let up = item; up && shown; up = up.parent) {
            shown = up.visible && up.opacity > 0;
        }
        if (!shown) {
            return;
        }
        let scrolls = false;
        let sideways = false;
        let child = item;
        for (let at2 = item.parent; at2; child = at2, at2 = at2.parent) {
            if (isScrollContent(at2, child)) {
                scrolls = true;
                sideways = sideways || at2.contentWidth > at2.width + epsilon;
            }
            if (!(at2 === root || at2.clip === true)) {
                continue;
            }
            // The text as painted: a label wider than its words is not what overflows.
            const p = item.mapToItem(at2, 0, 0);
            const painted = Math.min(item.width, item.contentWidth);
            const align = item.effectiveHorizontalAlignment;
            const left = p.x + (align === 2 ? item.width - painted : align === 4 ? (item.width - painted) / 2 : 0);
            const outSide = !sideways && (left + painted > at2.width + epsilon || left < -epsilon);
            const outEnd = !scrolls && (p.y + item.height > at2.height + epsilon || p.y < -epsilon);
            if (outSide || outEnd) {
                found.push(nameOf(item) + " at " + Math.round(p.x) + "," + Math.round(p.y) + " "
                           + Math.round(item.width) + "x" + Math.round(item.height)
                           + ": text outside " + typeName(at2) + " " + Math.round(at2.width) + "x"
                           + Math.round(at2.height));
                break;
            }
        }
    });

    for (let i = 0; i < rects.length; ++i) {
        for (let j = i + 1; j < rects.length; ++j) {
            const a = rects[i];
            const b = rects[j];
            let related = false;
            for (let at = a.item.parent; at; at = at.parent) {
                if (at === b.item) {
                    related = true;
                }
            }
            for (let at = b.item.parent; at; at = at.parent) {
                if (at === a.item) {
                    related = true;
                }
            }
            if (related) {
                continue;
            }
            const overlapX = Math.min(a.x + a.w, b.x + b.w) - Math.max(a.x, b.x);
            const overlapY = Math.min(a.y + a.h, b.y + b.h) - Math.max(a.y, b.y);
            if (overlapX > 4 && overlapY > 4) {
                found.push(nameOf(a.item) + " overlaps " + nameOf(b.item) + " by "
                           + Math.round(overlapX) + "x" + Math.round(overlapY));
            }
        }
    }
    return found;
}

// What scrolls up and down under `root`, as readable strings: a Flickable that is visible, can move
// vertically, is not a list (a list of words scrolls by its nature) and holds more than it shows. A
// phone's play screens (Home, Solo Play's box and its board) are held to none: the child's way on is
// in view, not below the fold.
function scrollingPages(root) {
    const found = [];
    walk(root, function (item) {
        if (item === root || !item.visible || item.opacity <= 0 || !isFlickable(item)
                || item.flickableDirection === 1 || item.model !== undefined) {
            return;
        }
        if (item.contentHeight > item.height + 1 && item.height > 0) {
            found.push(nameOf(item) + " scrolls: " + Math.round(item.contentHeight) + " dp of content in "
                       + Math.round(item.height) + " dp");
        }
    });
    return found;
}
