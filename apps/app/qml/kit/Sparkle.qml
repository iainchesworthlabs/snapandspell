// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import SnapAndSpell

// A word's sparkle, Toybox: a few small gold squares fly out from a point and fade over about half a
// second, once (docs/design/application.md, "Motion"). It is the whole of the effect the design
// allows on a found word - a burst confined to the board that rises and fades, never a flash, never a
// loop, never the screen shaking. With reduced motion nothing is drawn: the word and its jewel appear
// as they do without it. Call burst() to play it; the item sits at the point it is placed at.
Item {
    id: root

    // How far a spark travels, as a multiple of `reach`, and how many there are.
    property real reach: 48
    property int count: 6
    // 0 at rest, 1 at the end of a burst; the sparks read it.
    property real progress: 0

    // True while a burst is on screen.
    readonly property bool playing: anim.running

    width: 0
    height: 0
    visible: root.playing
    // The sparks are decoration.
    Accessible.ignored: true

    function burst() {
        if (Theme.reducedMotion) {
            return;
        }
        anim.restart();
    }

    NumberAnimation {
        id: anim
        target: root
        property: "progress"
        from: 0
        to: 1
        duration: Theme.sparkle
        easing.type: Easing.OutCubic
    }

    Repeater {
        model: root.count

        Rectangle {
            id: spark
            required property int index
            readonly property real angle: (2 * Math.PI * spark.index) / root.count + Math.PI / 6
            readonly property real side: Math.max(4, root.reach * 0.16) * (1 - 0.5 * root.progress)

            x: Math.cos(spark.angle) * root.reach * root.progress - spark.side / 2
            y: Math.sin(spark.angle) * root.reach * root.progress - spark.side / 2
            width: spark.side
            height: spark.side
            rotation: 45
            radius: 0
            color: Theme.gold
            border.width: Theme.outlineSpark
            border.color: Theme.ink
            opacity: 1 - root.progress
        }
    }
}
