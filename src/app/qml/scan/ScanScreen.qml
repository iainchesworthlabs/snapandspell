pragma ComponentBehavior: Bound
import QtQuick
import WordJewelsSolver

// Mode A, the Family Companion's first screen (docs/design/game.md, "The MVP:
// one capture"): `scan.ready`, `scan.reading` and `scan.check` are one screen,
// because the camera, the frame and the check belong to one act and the check
// screen comes back to the ready screen with "Read again".
//
//   scan.ready    the frame to put the board in, Capture (96 dp), and Type in
//                 your board. The camera is on only here: the screen starts it
//                 when it opens and stops it when it leaves (the screens' and the
//                 model's rule; docs/design/game.md, "Mode A"). Where there is a
//                 camera, its live picture is shown in the frame.
//   scan.reading one frame is read on the device; nothing is kept.
//   scan.check    the replica, the doubts, the letter picker and "Yes, play"
//                 (CheckScreen.qml). "Yes, play" hands the confirmed board to the
//                 assistant and asks the shell to show it.
//
// With no camera (a desktop with no webcam, or a kit without Qt Multimedia) the
// camera's absence is said in words and "Type in your board" is the way on: the
// check screen with an empty 4x4 (docs/design/game.md, "No camera").
FocusScope {
    id: root

    required property ScanModel scan
    required property AssistModel assist

    // Back from `scan.ready`: out of Mode A, to wherever the person came from.
    signal closed
    // "Yes, play" has confirmed a board and the assistant has it.
    signal playRequested

    readonly property string scanState: root.scan.state
    readonly property bool cameraShown: root.scan.cameraAvailable
    // The live view is shown only while the camera is running: ScanModel starts
    // it on scan.ready and stops it as soon as a frame is in hand.
    readonly property bool cameraRunning: root.scan.cameraCapturing
    readonly property bool previewShown: cameraPreview.active
    readonly property string cameraMessage: ScanWording.cameraProblem(root.scan.cameraProblem)
    readonly property string readMessage: ScanWording.readProblem(root.scan.readProblem)
    // The interface's direction, set by the shell: Hebrew and Yiddish are right
    // to left. The frame, the bar and the check screen's rows follow it; the
    // replica of the board does not.
    property bool rightToLeft: Theme.rightToLeft
    // For the RTL check in the stub shell: the bar's way back follows the
    // leading edge, and the window's mirroring reaches this screen.
    readonly property real backButtonX: Theme.rightToLeft ? width - Theme.space4 - Theme.targetMin
                                                          : Theme.space4
    readonly property bool mirrored: LayoutMirroring.enabled
    // The check screen's confirm button, for the RTL check.
    readonly property real checkYesPlayX: check.yesPlayX

    LayoutMirroring.enabled: root.rightToLeft
    LayoutMirroring.childrenInherit: true

    activeFocusOnTab: true

    Component.onCompleted: {
        // A screen that opens on `check` is one the person came back to; the
        // camera stays off. A screen that opens on `ready` is a new scan.
        if (root.scan.state === "ready") {
            root.scan.open();
        }
    }

    Component.onDestruction: root.scan.close()

    // The way back: from the check screen it is "Read again" (to the frame and a
    // camera that is on again); from the ready screen it leaves Mode A.
    function back() {
        if (root.scanState === "check" || root.scanState === "reading") {
            root.scan.readAgain();
        } else {
            root.scan.close();
            root.closed();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.page
    }

    // --- the bar ---------------------------------------------------------------
    AppBar {
        id: appBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        showBack: true
        showWordmark: false
        showParent: false
        title: root.scanState === "check" ? ScanWording.checkTitle
             : (root.scanState === "reading" ? ScanWording.reading : qsTr("Set up a board"))
        onBack: root.back()
    }

    // --- scan.ready and scan.reading ------------------------------------------------
    Item {
        id: readyStage
        anchors.top: appBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: root.scanState !== "check"

        Row {
            id: stepper
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: Theme.space4
            spacing: Theme.space3

            Text {
                text: qsTr("1  Place")
                font.family: Theme.monoFont
                font.pixelSize: Theme.smallSize
                color: Theme.accent
            }
            Text {
                text: qsTr("2  Check")
                font.family: Theme.monoFont
                font.pixelSize: Theme.smallSize
                color: Theme.inkSecondary
            }
            Text {
                text: qsTr("3  Play")
                font.family: Theme.monoFont
                font.pixelSize: Theme.smallSize
                color: Theme.inkSecondary
            }
        }

        Text {
            id: prompt
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: stepper.bottom
            anchors.topMargin: Theme.space3
            width: Math.min(parent.width - 2 * Theme.space6, 720)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: root.scanState === "reading" ? ScanWording.reading : ScanWording.readyTitle
            font.family: Theme.uiFont
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
            color: Theme.ink
        }

        // The live view: the camera's own picture while it is on, with the
        // corners of the board's place over it. When the camera is off (the
        // check screen, or a device with none) the place keeps its picture of a
        // board, so the screen still says what it is for without a sentence.
        Rectangle {
            id: viewfinder
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: prompt.bottom
            anchors.topMargin: Theme.space4
            width: Math.max(Theme.targetMin, Math.min(parent.width - 2 * Theme.space8,
                                                     Math.min(parent.height * 0.55, 640)))
            height: width * 0.72
            radius: Theme.radiusLarge
            color: Theme.tray
            border.width: 2
            border.color: Theme.gold
            clip: true

            // The camera's picture. The view is a CameraPreview of the build's
            // camera variant: a VideoOutput where the kit has Qt Multimedia, and
            // an empty Item where it does not. The source hands its sink over in
            // Component.onCompleted, and nothing is recorded.
            Loader {
                id: cameraPreview
                anchors.fill: parent
                active: root.cameraRunning && root.scan.cameraSource !== null
                sourceComponent: Component {
                    CameraPreview {
                        camera: root.scan.cameraSource
                    }
                }
            }

            // The four corners of a board's place, as brackets.
            Repeater {
                model: 4

                Item {
                    id: corner
                    required property int index
                    readonly property bool onRight: index % 2 === 1
                    readonly property bool onBottom: index >= 2
                    x: corner.onRight ? viewfinder.width - width - Theme.space4 : Theme.space4
                    y: corner.onBottom ? viewfinder.height - height - Theme.space4 : Theme.space4
                    width: Math.min(viewfinder.width, viewfinder.height) * 0.16
                    height: width

                    Rectangle {
                        width: parent.width
                        height: Theme.heavy
                        color: Theme.die
                    }
                    Rectangle {
                        width: Theme.heavy
                        height: parent.height
                        color: Theme.die
                        x: corner.onRight ? parent.width - width : 0
                        y: corner.onBottom ? parent.height - height : 0
                    }
                }
            }

            // A small board of dice, so the place says what goes in it without
            // a sentence: a picture, not text. It is behind the camera's view
            // while the camera is on.
            Grid {
                anchors.centerIn: parent
                columns: 4
                spacing: Math.max(Theme.space1, viewfinder.width * 0.012)
                opacity: 0.5
                visible: !root.cameraRunning

                Repeater {
                    model: 16

                    Rectangle {
                        width: (Math.min(viewfinder.width, viewfinder.height) * 0.4
                                - 3 * Math.max(Theme.space1, viewfinder.width * 0.012)) / 4
                        height: width
                        color: Theme.die
                    }
                }
            }

            // While reading, a quiet band across the picture.
            Rectangle {
                visible: root.scanState === "reading"
                anchors.centerIn: parent
                width: parent.width
                height: Theme.targetChild
                color: Theme.scrim
            }
        }

        // The camera's absence, and a read that failed: two facts, no blame.
        Column {
            id: messages
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: viewfinder.bottom
            anchors.topMargin: Theme.space3
            width: Math.min(parent.width - 2 * Theme.space6, 720)
            spacing: Theme.space2

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: text !== ""
                text: root.cameraMessage
                font.family: Theme.uiFont
                font.pixelSize: Theme.bodySize
                color: Theme.inkSecondary
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: text !== ""
                text: root.readMessage
                font.family: Theme.uiFont
                font.pixelSize: Theme.bodySize
                color: Theme.inkSecondary
            }
        }

        // Capture, and the typed board beside it: the grown-up chooses.
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Theme.space6
            spacing: Theme.space4

            AppButton {
                id: capture
                visible: root.cameraShown && root.scanState === "ready"
                width: Math.max(root.width / 3, Theme.targetPrimary)
                height: Theme.targetPrimary
                size: "primary"
                variant: "primary"
                iconName: ""
                label: ScanWording.capture
                accessibleName: ScanWording.capture
                onClicked: root.scan.capture()
            }

            AppButton {
                id: typeBoard
                width: Math.max(root.width / 3, root.cameraShown ? Theme.targetChild : Theme.targetPrimary)
                height: root.cameraShown ? Theme.targetChild : Theme.targetPrimary
                size: root.cameraShown ? "child" : "primary"
                variant: root.cameraShown ? "secondary" : "primary"
                label: ScanWording.typeBoard
                accessibleName: ScanWording.typeBoard
                onClicked: root.scan.typeBoard()
            }
        }
    }

    // --- scan.check --------------------------------------------------------------------
    CheckScreen {
        id: check
        anchors.top: appBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: root.scanState === "check"
        scan: root.scan
        assist: root.assist
        onBackRequested: root.scan.readAgain()
        onPlayRequested: root.playRequested()
    }
}
