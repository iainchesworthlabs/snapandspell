import QtQuick
import WordJewelsSolver

// The calm resting screen: the closed box asleep, a short line, and a visible
// Play again control whose ring asks for a deliberate press. The app bar above
// it holds the way home.
Item {
    id: root

    signal againRequested
    signal backRequested

    opacity: 0
    Component.onCompleted: fadeIn.start()

    NumberAnimation {
        id: fadeIn
        target: root
        property: "opacity"
        from: 0
        to: 1
        duration: Theme.restFade
    }
    readonly property bool settled: opacity >= 1

    Rectangle {
        anchors.fill: parent
        color: Theme.restPage
    }

    Column {
        anchors.centerIn: parent
        spacing: Theme.space6

        JewelBox {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(root.width * 0.55, 360)
            height: width * 0.78
            closed: true
            asleep: true
            showDice: false
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Time for a rest.")
            font.family: Theme.uiFont
            font.pixelSize: Theme.titleSize
            font.weight: Font.ExtraBold
            color: Theme.ink
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("You did it. The words are safe in your collection.")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            color: Theme.inkSecondary
        }
    }

    HoldButton {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.space8
        width: 220
        height: 72
        enabled: root.settled
        opacity: root.settled ? 1 : 0
        iconName: "again"
        label: qsTr("Play again")
        accessibleName: qsTr("Play again")
        onActivated: root.againRequested()
    }
}
