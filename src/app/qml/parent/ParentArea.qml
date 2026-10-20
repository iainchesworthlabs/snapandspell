pragma ComponentBehavior: Bound
import QtQuick

// The parent area: the gate, then the dashboard, settings and about. The adult
// workflows live here, behind the gate. The dashboard's "Play with a real
// board" leaves this screen for the scanner while keeping the gate open, so a
// parent does not have to pass it again on the way back.
FocusScope {
    id: root

    required property var controller
    property var scanModel: null
    property var assistModel: null

    property string page: "dashboard"
    property bool rightToLeft: false
    property bool stayUnlockedOnClose: false

    signal closed
    signal scanRequested

    readonly property real backButtonX: root.rightToLeft
                                          ? root.width - ParentTheme.space4 - barBack.width / 2
                                          : ParentTheme.space4 + barBack.width / 2
    readonly property bool mirrored: LayoutMirroring.enabled

    LayoutMirroring.enabled: root.rightToLeft
    LayoutMirroring.childrenInherit: true

    Binding {
        target: ParentTheme
        property: "rightToLeft"
        value: root.rightToLeft
    }
    Binding {
        target: ParentTheme
        property: "textScale"
        value: root.controller.textScale
    }

    function back() {
        root.stayUnlockedOnClose = false;
        root.controller.lock();
        root.page = "dashboard";
        root.closed();
    }

    focus: true

    Rectangle {
        anchors.fill: parent
        color: ParentTheme.page
    }

    // --- the bar ---------------------------------------------------------------
    Rectangle {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 72
        color: ParentTheme.surface

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: ParentTheme.line
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: ParentTheme.space4
            anchors.verticalCenter: parent.verticalCenter
            spacing: ParentTheme.space3

            PButton {
                id: barBack
                text: ParentWording.back
                onClicked: root.back()
            }

            Rectangle {
                width: 22
                height: 22
                color: ParentTheme.gold
                rotation: 45
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                height: 48
                verticalAlignment: Text.AlignVCenter
                text: ParentWording.wordmarkTitle
                font.family: ParentTheme.uiFont
                font.pixelSize: 18
                font.weight: Font.ExtraBold
                color: ParentTheme.ink
                Accessible.role: Accessible.Heading
                Accessible.name: text
            }
            Text {
                height: 48
                verticalAlignment: Text.AlignVCenter
                text: ParentWording.parentArea
                font.family: ParentTheme.uiFont
                font.pixelSize: ParentTheme.sizeLead
                color: ParentTheme.inkSecondary
                leftPadding: ParentTheme.space2
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: ParentTheme.space4
            anchors.verticalCenter: parent.verticalCenter
            spacing: ParentTheme.space2
            visible: root.controller.unlocked

            Repeater {
                model: root.controller.unlocked
                    ? [{id: "dashboard", label: ParentWording.tabDashboard},
                       {id: "settings", label: ParentWording.tabSettings},
                       {id: "about", label: ParentWording.tabAbout},
                       {id: "licences", label: ParentWording.tabLicences}]
                    : []
                PButton {
                    required property var modelData
                    text: modelData.label
                    checkable: true
                    checked: root.page === modelData.id
                    onClicked: {
                        root.page = modelData.id;
                        if (modelData.id === "dashboard") {
                            root.controller.refresh();
                        }
                    }
                }
            }
        }
    }

    Loader {
        id: screen
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        focus: true

        sourceComponent: {
            if (!root.controller.unlocked) {
                return gate;
            }
            switch (root.page) {
            case "settings":
                return settings;
            case "about":
                return about;
            case "licences":
                return licences;
            default:
                return dashboard;
            }
        }

        onLoaded: (item as Item).forceActiveFocus()
    }

    Component {
        id: gate
        GateScreen {
            controller: root.controller
        }
    }
    Component {
        id: dashboard
        DashboardScreen {
            controller: root.controller
            onScanRequested: {
                root.stayUnlockedOnClose = true;
                root.scanRequested();
            }
            onPlayRequested: root.back()
        }
    }
    Component {
        id: settings
        SettingsScreen {
            controller: root.controller
        }
    }
    Component {
        id: about
        AboutScreen {
            controller: root.controller
            onLicencesRequested: root.page = "licences"
        }
    }
    Component {
        id: licences
        LicencesScreen {
            controller: root.controller
        }
    }

    Keys.onEscapePressed: event => {
        root.back();
        event.accepted = true;
    }

    onVisibleChanged: {
        if (!root.visible && root.controller && !root.stayUnlockedOnClose) {
            root.controller.lock();
        }
    }
    Component.onDestruction: {
        if (root.controller && !root.stayUnlockedOnClose) {
            root.controller.lock();
        }
    }
}
