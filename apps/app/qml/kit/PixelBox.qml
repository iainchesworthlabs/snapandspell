// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes
import SnapAndSpell

// A Toybox box: a fill, an ink outline, and a hard drop shadow with no blur (docs/design/toybox/DESIGN.md,
// sections 3 and 6). A button, a panel, a dialog and a chip are each one of these. The shadow is drawn
// below the box, outside its size, so a box takes the room of its face and the shadow falls into the
// gap beside it.
//
//   shadowOffset  how far down the shadow falls; 0 for none (a ghost button, a disabled one)
//   sunk          the face has been pressed onto its shadow: it is `shadowOffset` lower and the shadow
//                 is gone from view
//   dashed        the outline is dashed (a disabled control), drawn over the fill
//
// The outline is inside the box, as a CSS border is, so a 3 px outline on a 48 dp box leaves a 42 dp
// face. Nothing here animates: a press is a step, as pixel art is.
Item {
    id: root

    property color fill: Theme.surface
    property color edge: Theme.outline
    property real edgeWidth: Theme.outlinePanel
    property real radius: Theme.radiusPanel
    property real shadowOffset: Theme.shadowMedium
    property color shadowColor: Theme.shadow
    property bool sunk: false
    property bool dashed: false

    // The face, for a child that needs to be positioned on it.
    readonly property alias face: face

    Rectangle {
        id: shadow
        visible: root.shadowOffset > 0
        width: root.width
        height: root.height
        y: root.shadowOffset
        radius: root.radius
        color: root.shadowColor
    }

    Rectangle {
        id: face
        width: root.width
        height: root.height
        y: root.sunk ? root.shadowOffset : 0
        radius: root.radius
        color: root.fill
        border.width: root.dashed ? 0 : root.edgeWidth
        border.color: root.edge

        Shape {
            visible: root.dashed && root.edgeWidth > 0
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                strokeColor: root.edge
                strokeWidth: root.edgeWidth
                fillColor: "transparent"
                strokeStyle: ShapePath.DashLine
                dashPattern: [Theme.dashLength, Theme.dashLength]
                capStyle: ShapePath.FlatCap

                PathRectangle {
                    x: root.edgeWidth / 2
                    y: root.edgeWidth / 2
                    width: root.width - root.edgeWidth
                    height: root.height - root.edgeWidth
                    radius: Math.max(0, root.radius - root.edgeWidth / 2)
                }
            }
        }
    }
}
