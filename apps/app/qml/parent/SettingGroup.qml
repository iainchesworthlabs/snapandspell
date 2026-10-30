// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import SnapAndSpell

// A group of settings on one panel, Toybox: the card colour, a 4 px ink outline and the hard shadow,
// with the rows one under another and a dashed line between them (docs/design/toybox/DESIGN.md,
// section 6). The rows are SettingRows and draw their own dashed line above them; the first one's sits
// under the panel's outline, which is drawn over the rows, and is not seen.
Rectangle {
    id: root

    default property alias rows: body.data

    implicitHeight: body.implicitHeight
    radius: Theme.radiusPanel
    color: Theme.card

    // The hard shadow, painted behind the panel.
    Rectangle {
        z: -1
        y: Theme.shadowLarge
        width: root.width
        height: root.height
        radius: root.radius
        color: Theme.shadow
    }

    ColumnLayout {
        id: body
        anchors.fill: parent
        spacing: 0
    }

    // The outline, over the rows.
    Rectangle {
        z: 2
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.width: Theme.outlinePanel
        border.color: Theme.line
    }
}
