import QtQuick
import WordJewelsSolver

// The persistent top bar of a screen. On home it carries the wordmark; on a
// child screen the way back and the screen's name; the grown-up entry is always
// a labelled control, never a bare person icon.
Item {
    id: root

    property string title: ""
    property bool showBack: true
    property bool showWordmark: false
    property bool showParent: true
    property bool compact: false
    // The child's own collection: home only, so the words and stars are one
    // press from the first screen rather than hidden behind the play panel.
    property bool showJewels: false
    // Solo Play's earlier boards: home only, so the boards are one press from
    // the first screen rather than hidden behind the play panel.
    property bool showBoards: false

    signal back
    signal parentRequested
    signal jewelsRequested
    signal boardsRequested

    implicitHeight: Theme.appBarHeight
    height: implicitHeight

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
    }
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Theme.hair
        color: Theme.line
    }

    // A Row aligns its children at the top, so each one fills the bar's height
    // (or centres its own text in it): the title, the wordmark and the buttons
    // share the bar's middle line, never its top edge.
    Row {
        anchors.left: parent.left
        anchors.leftMargin: Theme.space4
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.space3

        AppButton {
            visible: root.showBack
            size: "min"
            variant: "ghost"
            iconName: "back"
            label: qsTr("Back")
            accessibleName: qsTr("Back")
            onClicked: root.back()
        }

        Wordmark {
            visible: root.showWordmark
            compact: root.compact
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: !root.showWordmark && root.title.length > 0
            height: Theme.targetMin
            verticalAlignment: Text.AlignVCenter
            text: root.title
            font.family: Theme.uiFont
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
            color: Theme.ink
            elide: Text.ElideRight
        }
    }

    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.space4
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.space2

        AppButton {
            visible: root.showBoards
            size: "min"
            variant: "ghost"
            label: qsTr("My boards")
            accessibleName: qsTr("My boards")
            onClicked: root.boardsRequested()
        }

        AppButton {
            visible: root.showJewels
            size: "min"
            variant: "ghost"
            label: qsTr("My jewels")
            accessibleName: qsTr("My jewels")
            onClicked: root.jewelsRequested()
        }

        AppButton {
            visible: root.showParent
            size: "min"
            variant: "ghost"
            iconName: "parent"
            label: qsTr("For grown-ups")
            accessibleName: qsTr("For grown-ups")
            onClicked: root.parentRequested()
        }
    }
}
