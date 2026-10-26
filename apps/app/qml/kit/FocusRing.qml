// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes
import SnapAndSpell

// The keyboard focus ring: a 4 px dashed line in the focus colour (the outline colour: ink, cream in
// Night), a 3 px gap outside the thing it rings, drawn by the thing itself (AppButton, ModeCard,
// HoldButton) so every focusable object shows focus the same way. Put it as a child, set `inner` to
// the parent's radius, and bind `visible` to "focused and not by the pointer".
Item {
    id: ring

    // The parent's own radius; the ring is concentric with it.
    property real inner: Theme.radius

    anchors.fill: parent
    anchors.margins: -(Theme.focusGap + Theme.focusWidth)
    visible: false
    // Not a thing to tap or to read out.
    Accessible.ignored: true

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.focusRing
            strokeWidth: Theme.focusWidth
            fillColor: "transparent"
            strokeStyle: ShapePath.DashLine
            dashPattern: [Theme.dashLength, Theme.dashLength]
            capStyle: ShapePath.FlatCap

            PathRectangle {
                x: Theme.focusWidth / 2
                y: Theme.focusWidth / 2
                width: ring.width - Theme.focusWidth
                height: ring.height - Theme.focusWidth
                radius: ring.inner + Theme.focusGap + Theme.focusWidth / 2
            }
        }
    }
}
