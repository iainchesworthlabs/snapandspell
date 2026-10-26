// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// A titled card: the pattern behind the panel's Words and Hint cards, the
// dashboard's sections and the jewels page. Toybox: the card colour, a 4 px ink outline, an 18 px
// radius and a hard 6 px shadow (docs/design/toybox/DESIGN.md, section 6).
//
// Every card has a title and an empty state (docs/design/application.md,
// "The Jewel Box design system"): `title` is the heading, `hint` a caption under it, and while
// `empty` is true the content is replaced by a small dead jewel, `emptyTitle`
// and `emptyText`, so a card with nothing in it still says what it is for.
// Children go into the body and may use Layout.* properties.
Rectangle {
    id: root

    property string title: ""
    property string hint: ""
    property bool empty: false
    property string emptyTitle: ""
    property string emptyText: ""
    property real padding: Theme.space4

    default property alias content: body.data

    implicitWidth: column.implicitWidth + 2 * padding
    implicitHeight: column.implicitHeight + 2 * padding
    radius: Theme.radiusPanel
    color: Theme.card
    border.width: Theme.outlinePanel
    border.color: Theme.line

    // The hard shadow, painted behind the card (a negative z is below its parent).
    Rectangle {
        z: -1
        y: Theme.shadowLarge
        width: root.width
        height: root.height
        radius: root.radius
        color: Theme.shadow
    }

    Accessible.role: Accessible.Grouping
    Accessible.name: root.title

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: Theme.space2

        Label {
            visible: root.title.length > 0
            Layout.fillWidth: true
            role: "heading"
            text: root.title
            wrapMode: Text.WordWrap
        }
        Label {
            visible: root.hint.length > 0
            Layout.fillWidth: true
            role: "caption"
            text: root.hint
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            id: body
            visible: !root.empty
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.space2
        }

        ColumnLayout {
            visible: root.empty
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.space2

            Item {
                Layout.fillHeight: true
            }
            Jewel {
                Layout.alignment: Qt.AlignHCenter
                tier: "power"
                color: Theme.jewelDead
                Accessible.ignored: true
            }
            Label {
                visible: root.emptyTitle.length > 0
                Layout.fillWidth: true
                role: "label"
                text: root.emptyTitle
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
            Label {
                visible: root.emptyText.length > 0
                Layout.fillWidth: true
                role: "caption"
                text: root.emptyText
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
            Item {
                Layout.fillHeight: true
            }
        }
    }
}
