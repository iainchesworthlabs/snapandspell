pragma ComponentBehavior: Bound
import QtQuick

// The parent area (docs/design/application.md, screens `parent.gate` and
// `parent.*`): the gate, and behind it the dashboard, the settings and the
// about page. It is one screen of the application's navigation; it knows
// nothing of the others. Whoever shows it gives it a controller and listens for
// `closed()`, which is Back and Esc: it shuts the gate again, so that the next
// time a person comes the gate is there.
//
//   ParentArea {
//       controller: theController
//       onClosed: Navigation.back()
//   }
//
// Back goes where the person came from: out of the parent area, whichever page
// of it they are on. The tabs are pages of one place, not steps, so switching
// between them is not "back".
FocusScope {
    id: root

    required property var controller

    // dashboard, settings or about; the first behind the gate.
    property string page: "dashboard"

    // The interface's direction, set by the shell: Hebrew and Yiddish are right
    // to left. Mirroring is declared here, not on the instance outside, so the
    // children created with this component inherit it from the start.
    property bool rightToLeft: false

    signal closed

    // Where the bar's way back is drawn, for the RTL check in the stub shell:
    // the leading edge is the left in a left-to-right language and the right in
    // a right-to-left one. A mirrored Flow orders its children from that edge.
    readonly property real backButtonX: barBack.x
    // Whether the window's mirroring reaches this screen.
    readonly property bool mirrored: LayoutMirroring.enabled

    LayoutMirroring.enabled: root.rightToLeft
    LayoutMirroring.childrenInherit: true

    Binding {
        target: ParentTheme
        property: "rightToLeft"
        value: root.rightToLeft
    }

    // The Text size setting applies to these screens as it does everywhere.
    Binding {
        target: ParentTheme
        property: "textScale"
        value: root.controller.textScale
    }

    // What Esc and the system's back do: the same as the visible Back.
    function back() {
        root.controller.lock();
        root.page = "dashboard";
        root.closed();
    }

    focus: true

    Rectangle {
        anchors.fill: parent
        color: ParentTheme.page
    }

    // --- the bar: the way back, the title, and (past the gate) the pages --------
    Rectangle {
        id: bar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        implicitHeight: barLayout.implicitHeight + 2 * ParentTheme.space2
        height: implicitHeight
        color: ParentTheme.card

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: ParentTheme.hair
            color: ParentTheme.outline
        }

        Flow {
            id: barLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: ParentTheme.space2
            spacing: ParentTheme.space2
            // The way back is at the leading edge of the bar: left in English
            // and the other left-to-right languages, right in Hebrew and Yiddish.

            PButton {
                id: barBack
                text: ParentWording.back
                onClicked: root.back()
            }

            Text {
                height: ParentTheme.targetMin
                verticalAlignment: Text.AlignVCenter
                text: ParentWording.parentArea
                font.pixelSize: ParentTheme.sizeLead
                font.bold: true
                color: ParentTheme.ink
                Accessible.role: Accessible.Heading
                Accessible.name: text
                leftPadding: ParentTheme.space2
                rightPadding: ParentTheme.space4
            }

            Repeater {
                model: root.controller.unlocked
                    ? [{id: "dashboard", label: ParentWording.tabDashboard},
                       {id: "settings", label: ParentWording.tabSettings},
                       {id: "about", label: ParentWording.tabAbout}]
                    : []

                PButton {
                    required property var modelData
                    text: modelData.label
                    checkable: true
                    checked: root.page === modelData.id
                    onClicked: {
                        root.page = modelData.id;
                        if (modelData.id === "dashboard") {
                            // Worked out when it is opened, as of now.
                            root.controller.refresh();
                        }
                    }
                }
            }
        }
    }

    // --- the screen ---------------------------------------------------------------------
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
        }
    }

    Keys.onEscapePressed: event => {
        root.back();
        event.accepted = true;
    }

    // Leaving by any route shuts the gate again, so that the next visit meets
    // it: the screen is hidden by its host, or taken away.
    onVisibleChanged: {
        if (!root.visible && root.controller) {
            root.controller.lock();
        }
    }
    Component.onDestruction: {
        if (root.controller) {
            root.controller.lock();
        }
    }
}
