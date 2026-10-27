// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import SnapAndSpell
import "SizeMatrix.js" as SizeMatrix

// The size matrix as a test case. A suite for a screen or a kit component is
// this, with the things to try:
//
//   SizeMatrixCase {
//       name: "SizeMatrixHome"
//       subjects: [homeComponent]            // Components, each made to fill the stage
//       Component { id: homeComponent; Home { } }
//   }
//
// Each subject is created at every size and text scale of SizeMatrix.sizes
// (900x620, 900x1200, 1024x768, 2400x1200 and 150% text) and every interactive
// item under it - anything that takes Tab focus - is audited (SizeMatrix.js):
// at least 48 dp each way, wholly inside the window and its clipping parents,
// no truncated label, no two controls on top of each other. A finding fails the
// case with each problem named.
//
// `knownIssues` is the escape hatch for a screen whose stream has not fixed
// something yet: a list of substrings, and a finding that contains one is
// reported in the log but does not fail. It is a ratchet like the lint
// allow-list - an entry that matches nothing fails the case, so the list can
// only shrink.
TestCase {
    id: root

    when: windowShown
    visible: true
    width: 800
    height: 600

    // Components to try; each takes the stage's width and height.
    property var subjects: []
    property var sizes: SizeMatrix.sizes
    // The languages each size is also held to, [{code, rtl}]: English alone by default; a suite of the
    // long translations (German), the right-to-left (Hebrew) and the wide scripts (Japanese) names them.
    property var languages: [{code: "en", rtl: false}]
    property var knownIssues: []
    // Indexes of subjects that cannot be made on this machine, and why (a live camera view with
    // no Qt Multimedia backend): their rows are skipped, the others run.
    property var skipSubjects: []
    property string skipReason: ""
    // Time for a layout to settle after a resize or a scale change.
    property int settle: 50

    Item {
        id: stage
    }

    function init() {
        failOnWarning(/.?/)
        Theme.colourScheme = "light"
        Theme.textScale = 1.0
        Theme.reducedMotion = true
        Theme.rightToLeft = false
    }

    function useLanguage(language) {
        fixture.useLanguage(language.code)
        Theme.language = language.code
        Theme.rightToLeft = language.rtl
    }

    function cleanup() {
        fixture.useLanguage("en")
        Theme.language = "en"
        Theme.colourScheme = "system"
        Theme.textScale = 1.0
        Theme.reducedMotion = false
        Theme.rightToLeft = false
    }

    function test_matrix_data() {
        const rows = [];
        for (let subject = 0; subject < root.subjects.length; ++subject) {
            for (const size of root.sizes) {
                for (const language of root.languages) {
                    rows.push({
                        tag: "subject " + subject + " at " + size.w + "x" + size.h + " text " + Math.round(size.scale * 100) + "%"
                             + (root.languages.length > 1 ? " in " + language.code : ""),
                        subject: subject,
                        w: size.w,
                        h: size.h,
                        scale: size.scale,
                        language: language
                    });
                }
            }
        }
        return rows;
    }

    function test_matrix(data) {
        if (root.skipSubjects.indexOf(data.subject) >= 0) {
            skip(root.skipReason);
        }
        Theme.textScale = data.scale;
        root.useLanguage(data.language);
        stage.width = data.w;
        stage.height = data.h;
        const item = root.subjects[data.subject].createObject(stage, {width: data.w, height: data.h});
        verify(item !== null, "the subject was created");
        // A subject that resets the fixture (a game) put English back; the screen retranslates.
        root.useLanguage(data.language);
        wait(root.settle);

        const found = SizeMatrix.audit(item, Theme.targetMin);
        const failing = [];
        const used = {};
        for (const finding of found) {
            let known = false;
            for (const issue of root.knownIssues) {
                if (finding.indexOf(issue) >= 0) {
                    known = true;
                    used[issue] = true;
                }
            }
            if (known) {
                console.log("known issue (" + data.tag + "): " + finding);
            } else {
                failing.push(finding);
            }
        }
        item.destroy();
        compare(failing.length, 0, data.tag + ":\n  " + failing.join("\n  "));
    }

    // A known issue that no longer happens must come off the list. Checked once,
    // at the largest text size on the smallest window, where most things do.
    function test_knownIssuesStillHappen() {
        if (root.knownIssues.length === 0) {
            skip("no known issues");
        }
        const stale = root.knownIssues.slice();
        for (let subject = 0; subject < root.subjects.length; ++subject) {
            for (const size of root.sizes) {
                Theme.textScale = size.scale;
                stage.width = size.w;
                stage.height = size.h;
                const item = root.subjects[subject].createObject(stage, {width: size.w, height: size.h});
                wait(root.settle);
                const found = SizeMatrix.audit(item, Theme.targetMin);
                item.destroy();
                for (const finding of found) {
                    for (let i = stale.length - 1; i >= 0; --i) {
                        if (finding.indexOf(stale[i]) >= 0) {
                            stale.splice(i, 1);
                        }
                    }
                }
            }
        }
        compare(stale.length, 0, "known issues that no longer happen (remove them): " + stale.join(" | "));
    }
}
