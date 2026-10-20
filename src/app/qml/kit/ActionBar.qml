import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// The action zone of a play screen: the one primary action and its quieter
// neighbours, in one row, on one height ladder. It replaces the four different
// button styles of the Solo panel, the three mismatched buttons of Mode A's
// assist row and the right-aligned pair on the check screen.
//
//   tier "child"    secondary 64 dp, primary 96 dp   (Clear / Done, Shake / Play again)
//   tier "parent"   secondary 48 dp, primary 64 dp   (Cancel / Save)
//
// The primary action is at the trailing end (right in a left-to-right language,
// left in a right-to-left one), the secondary before it. Both fill the width
// they are given and never fall below 64 dp, so a 244 px panel holds a Clear
// and a Done instead of clipping Done. Any further buttons go in as children and
// sit between the two. Every button is labelled; an icon beside it is optional.
RowLayout {
    id: root

    property string tier: "child"
    property string primaryLabel: ""
    property string primaryIcon: ""
    property string primarySpeaks: ""
    property bool primaryEnabled: true
    property string primaryVariant: "primary"
    property string secondaryLabel: ""
    property string secondaryIcon: ""
    property string secondarySpeaks: ""
    property bool secondaryEnabled: true
    property real minimumButtonWidth: Theme.targetChild

    default property alias extra: extras.data

    signal primaryClicked
    signal secondaryClicked

    readonly property string primarySize: tier === "parent" ? "child" : "primary"
    readonly property string secondarySize: tier === "parent" ? "min" : "child"

    spacing: Theme.space3

    AppButton {
        visible: root.secondaryLabel.length > 0
        Layout.fillWidth: true
        Layout.minimumWidth: root.minimumButtonWidth
        Layout.preferredWidth: implicitWidth
        Layout.alignment: Qt.AlignVCenter
        size: root.secondarySize
        variant: "secondary"
        label: root.secondaryLabel
        iconName: root.secondaryIcon
        speaks: root.secondarySpeaks
        quiet: root.tier === "parent"
        enabled: root.secondaryEnabled
        onClicked: root.secondaryClicked()
    }

    RowLayout {
        id: extras
        visible: extras.children.length > 0
        spacing: Theme.space3
        Layout.alignment: Qt.AlignVCenter
    }

    AppButton {
        visible: root.primaryLabel.length > 0
        Layout.fillWidth: true
        Layout.minimumWidth: root.minimumButtonWidth
        Layout.preferredWidth: implicitWidth
        Layout.alignment: Qt.AlignVCenter
        size: root.primarySize
        variant: root.primaryVariant
        label: root.primaryLabel
        iconName: root.primaryIcon
        speaks: root.primarySpeaks
        quiet: root.tier === "parent"
        enabled: root.primaryEnabled
        onClicked: root.primaryClicked()
    }
}
