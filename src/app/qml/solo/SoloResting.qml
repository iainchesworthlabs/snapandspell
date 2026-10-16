import QtQuick
import QtQuick.Shapes
import WordJewelsSolver

// solo.resting (docs/design/game.md, "The resting screen"): a calm, non-stimulating
// screen that invites putting the device down.
//
//   It shows a still, low-contrast picture with nothing to chase: the closed box,
//   asleep. No number, no star, no word, no best.
//   Nothing moves after it has faded in over 600 ms (a cut with reduced motion):
//   no loop, no sound, no vibration.
//   To play again there is one quiet control at the foot, 64 dp: hold it for a
//   second while its ring fills. A brake, not a gate.
//   It never starts another board, reminds or asks, and the way back (to home, so
//   leaving does not show the shake button) is a quiet corner button.
Item {
    id: root

    signal againRequested
    signal backRequested

    // The art is to be made; this stands in: a closed box with its eyes shut.
    readonly property real boxSide: Math.min(width, height) * 0.42

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
    // The control is there once the screen has finished arriving.
    readonly property bool settled: opacity >= 1

    Rectangle {
        anchors.fill: parent
        color: Theme.restPage
    }

    // The box.
    Item {
        id: sleepingBox
        width: root.boxSide
        height: root.boxSide
        anchors.horizontalCenter: parent.horizontalCenter
        y: (root.height - height) * 0.42

        Rectangle {
            anchors.fill: parent
            color: Theme.restShape
        }
        // The lid, a line across it.
        Rectangle {
            width: parent.width
            height: parent.height * 0.22
            color: Theme.restDetail
        }
        // Two closed eyes.
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: Theme.restDetail
                strokeWidth: Theme.heavy
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                startX: sleepingBox.width * 0.22
                startY: sleepingBox.height * 0.56
                PathQuad {
                    x: sleepingBox.width * 0.42
                    y: sleepingBox.height * 0.56
                    controlX: sleepingBox.width * 0.32
                    controlY: sleepingBox.height * 0.66
                }
            }
            ShapePath {
                strokeColor: Theme.restDetail
                strokeWidth: Theme.heavy
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                startX: sleepingBox.width * 0.58
                startY: sleepingBox.height * 0.56
                PathQuad {
                    x: sleepingBox.width * 0.78
                    y: sleepingBox.height * 0.56
                    controlX: sleepingBox.width * 0.68
                    controlY: sleepingBox.height * 0.66
                }
            }
        }
    }

    // The way back: home.
    AppButton {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.space2
        iconName: "back"
        accessibleName: qsTr("Back")
        onClicked: root.backRequested()
    }

    // The one quiet control: hold to play again.
    HoldButton {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.space8
        width: Theme.targetChild * 2
        height: Theme.targetChild
        enabled: root.settled
        opacity: root.settled ? 1 : 0
        accessibleName: qsTr("Play again")
        onActivated: root.againRequested()
    }
}
