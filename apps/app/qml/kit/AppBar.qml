// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import SnapAndSpell

// The persistent top bar of a screen, the child's and the parent's. On home it
// carries the wordmark; on a child screen the way back and the screen's name;
// in the parent area the wordmark, "Parent area" and the four tabs. Home's
// three entries (the boards, the jewels, the grown-ups) are bordered buttons
// with a picture and a word, so they read as buttons beside one another; the
// grown-up entry is always a labelled control, never a bare person icon.
//
// On a narrow window (a phone held upright) the boards and the jewels are the
// picture alone, as 48 dp buttons that still carry their names for a screen
// reader; the grown-ups' entry keeps its words.
//
// The bar is 72 dp. When the leading and trailing groups do not fit one line -
// 150% text on a 900 px window - the trailing group drops to a second row and
// the bar grows (`height` follows `implicitHeight`; anchor the screen below
// `bar.bottom`), so nothing is clipped or overlapped. A long title is elided.
Item {
    id: root

    property string title: ""
    property bool showBack: true
    property bool showWordmark: false
    property bool showParent: true
    property bool compact: false
    // Home's bar sits on the page: no panel fill and no rule under it, and the wordmark has its coral shadow.
    property bool transparent: false
    // The child's own collection: home only, so the words and stars are one
    // press from the first screen rather than hidden behind the play panel.
    property bool showJewels: false
    // Solo Play's earlier boards: home only, so the boards are one press from
    // the first screen rather than hidden behind the play panel.
    property bool showBoards: false
    // Page switches, [{id, label}], drawn as tabs at the trailing end; the one
    // whose id is `currentTab` is chosen. Used by the parent area.
    property var tabs: []
    property string currentTab: ""
    property string backLabel: qsTr("Back")

    // The Back button, for a screen that places something by it (the parent
    // area's language check reads where it is).
    readonly property Item backItem: backButton

    signal back
    signal parentRequested
    signal jewelsRequested
    signal boardsRequested
    signal tabSelected(string id)

    // Not before the bar has a width: its first layout is the wide one, so nothing jumps.
    readonly property bool narrow: root.width > 0 && Theme.isNarrow(root.width)
    readonly property real margin: narrow ? Theme.space3 : Theme.space4
    readonly property bool hasTrailing: showBoards || showJewels || showParent || tabs.length > 0
    // One line if both groups fit; otherwise two.
    readonly property real naturalLeading: (backButton.visible ? backButton.implicitWidth + Theme.space3 : 0)
                                           + (mark.visible ? mark.implicitWidth + Theme.space3 : 0)
                                           + (titleLabel.visible ? titleLabel.implicitWidth : 0)
    // The width the trailing group wants on one line: its entries side by side (the group wraps onto
    // rows when it has less than this).
    readonly property real naturalTrailing: {
        let width = 0;
        let count = 0;
        for (const entry of trailing.children) {
            if (entry.visible && entry.implicitWidth > 0) {
                width += entry.implicitWidth;
                ++count;
            }
        }
        return width + Math.max(0, count - 1) * trailing.spacing;
    }
    // The width both groups want on one line.
    readonly property real naturalWidth: naturalLeading + naturalTrailing + 3 * margin
    // On a narrow window the title gives way (it is elided) before the trailing group drops to a row of its
    // own, down to this much of it.
    readonly property real leastTitleWidth: narrow ? Math.min(titleLabel.implicitWidth, Theme.barTitleMin)
                                                   : titleLabel.implicitWidth
    readonly property real leastWidth: naturalWidth - (titleLabel.visible ? titleLabel.implicitWidth - leastTitleWidth : 0)
    readonly property bool stacked: hasTrailing && leastWidth > root.width

    // The first line is 72 dp or, at a large text size, whatever the wordmark and
    // the Back button need; a second line is added when the trailing group drops.
    readonly property real firstRowHeight: Math.max(Theme.appBarHeight, leading.height + 2 * Theme.space2)
    implicitHeight: stacked ? firstRowHeight + trailing.height + Theme.space3 : firstRowHeight
    height: implicitHeight

    Rectangle {
        anchors.fill: parent
        visible: !root.transparent
        color: Theme.surface
    }
    Rectangle {
        visible: !root.transparent
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Theme.outlinePanel
        color: Theme.line
    }

    // A Row aligns its children at the top, so each one centres itself on the
    // bar's middle line: the title, the wordmark and the buttons share it.
    Row {
        id: leading
        anchors.left: parent.left
        anchors.leftMargin: root.margin
        y: (root.firstRowHeight - height) / 2
        spacing: Theme.space3

        AppButton {
            id: backButton
            visible: root.showBack
            size: "min"
            variant: "ghost"
            iconName: "back"
            // The arrow alone on a narrow window (a back button may be a picture); its name stays.
            label: root.narrow ? "" : root.backLabel
            accessibleName: root.backLabel
            onClicked: root.back()
        }

        Wordmark {
            id: mark
            // On a narrow window a screen with a Back button has its title and no room for the name too.
            visible: root.showWordmark && !(root.narrow && root.showBack)
            compact: root.compact
            shadowed: root.transparent
            anchors.verticalCenter: parent.verticalCenter
        }

        Label {
            id: titleLabel
            visible: root.title.length > 0
            height: Math.max(Theme.targetMin, implicitHeight)
            verticalAlignment: Text.AlignVCenter
            // The room left of the trailing group (or of the bar's end, when
            // that group has a row of its own).
            width: Math.min(implicitWidth, Math.max(Theme.targetMin, root.width - 2 * root.margin
                                                     - (backButton.visible ? backButton.implicitWidth + Theme.space3 : 0)
                                                     - (mark.visible ? mark.implicitWidth + Theme.space3 : 0)
                                                     - (root.stacked ? 0 : root.naturalTrailing + Theme.space3)))
            role: root.showWordmark ? "body" : "heading"
            text: root.title
            color: root.showWordmark ? Theme.inkSecondary : Theme.ink
            elide: Text.ElideRight
            // A narrow bar wraps a long title onto a second line rather than cutting it short; the bar grows.
            wrapMode: root.narrow ? Text.WordWrap : Text.NoWrap
            maximumLineCount: root.narrow ? 2 : 1
        }
    }

    // The entries at the trailing end. On a row of their own (stacked) they have the bar's whole width
    // and wrap onto more rows if they need it (four tabs at big text on a phone).
    Flow {
        id: trailing
        anchors.right: parent.right
        anchors.rightMargin: root.margin
        y: root.stacked ? root.firstRowHeight : (root.firstRowHeight - height) / 2
        width: root.stacked ? root.width - 2 * root.margin : root.naturalTrailing
        height: childrenRect.height
        spacing: Theme.space2

        AppButton {
            visible: root.showBoards
            size: "min"
            variant: "secondary"
            quiet: true
            iconName: "history"
            label: root.narrow ? "" : qsTr("My boards")
            accessibleName: qsTr("My boards")
            onClicked: root.boardsRequested()
        }

        AppButton {
            visible: root.showJewels
            size: "min"
            variant: "secondary"
            quiet: true
            iconName: "jewel"
            label: root.narrow ? "" : qsTr("My jewels")
            accessibleName: qsTr("My jewels")
            onClicked: root.jewelsRequested()
        }

        Repeater {
            model: root.tabs
            AppButton {
                required property var modelData
                size: "min"
                variant: "tab"
                compact: root.narrow
                quiet: true
                label: modelData.label
                checkable: true
                checked: root.currentTab === modelData.id
                onClicked: root.tabSelected(modelData.id)
            }
        }

        AppButton {
            visible: root.showParent
            size: "min"
            variant: "secondary"
            quiet: true
            iconName: "parent"
            label: qsTr("For grown-ups")
            accessibleName: qsTr("For grown-ups")
            onClicked: root.parentRequested()
        }
    }
}
