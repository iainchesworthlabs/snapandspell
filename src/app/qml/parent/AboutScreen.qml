import QtQuick
import QtQuick.Layouts
import WordJewelsSolver

// About and privacy (docs/design/dashboard.md, "Settings"). Plain text: no link
// to anywhere, because the application has nowhere to go. The licences have
// their own page (LicencesScreen.qml), reached by the one button here, so the
// legal text is not the longest thing on this page.
Flickable {
    id: root

    required property var controller

    signal licencesRequested

    // The parent area's direction, from the shell.
    LayoutMirroring.enabled: Theme.rightToLeft
    LayoutMirroring.childrenInherit: true

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * Theme.space6
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    component Paragraph: Text {
        Layout.fillWidth: true
        font.family: Theme.uiFont
        font.pixelSize: Theme.bodySize
        color: Theme.ink
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        id: column
        x: Math.max(Theme.space4, (root.width - width) / 2)
        y: Theme.space6
        width: Math.min(root.width - 2 * Theme.space4, Theme.contentWidth)
        spacing: Theme.space8

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space1
            Text {
                Layout.fillWidth: true
                text: ParentWording.appName
                font.family: Theme.uiFont
                font.pixelSize: Theme.titleSize
                font.weight: Font.ExtraBold
                color: Theme.ink
            }
            Text {
                Layout.fillWidth: true
                text: ParentWording.tagline
                font.family: Theme.fontFamily
                font.pixelSize: Theme.headingSize
                color: Theme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        Section {
            Layout.fillWidth: true
            title: ParentWording.aboutHeading
            Paragraph {
                text: ParentWording.version(root.controller.version)
            }
            // The build's provenance, as the build system stamped it: the
            // release, the commit, the branch and the target. A technical
            // label, in the mono face, not a sentence to translate.
            Text {
                id: details
                objectName: "versionDetails"
                Layout.fillWidth: true
                Layout.topMargin: Theme.space2
                text: root.controller.versionDetails
                font.family: Theme.monoFont
                font.pixelSize: Theme.smallSize
                color: Theme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        // The way to the Licences page: its own screen, so the legal text
        // scrolls there and not through this page.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: linkColumn.implicitHeight + 2 * Theme.space4
            radius: Theme.radius
            color: Theme.card
            border.width: 1
            border.color: Theme.line

            ColumnLayout {
                id: linkColumn
                anchors.fill: parent
                anchors.margins: Theme.space4
                spacing: Theme.space2

                AppButton {
                    size: "min"
                    quiet: true
                    Layout.fillWidth: true
                    label: ParentWording.aboutLicencesLink
                    onClicked: root.licencesRequested()
                }
                Text {
                    Layout.fillWidth: true
                    text: ParentWording.aboutLicencesLinkHint
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.smallSize
                    color: Theme.inkSecondary
                    wrapMode: Text.WordWrap
                }
            }
        }

        // The one privacy section: the summary line, then what is kept and
        // what is never done.
        Section {
            Layout.fillWidth: true
            title: ParentWording.privacyHeading
            Paragraph {
                text: ParentWording.privacySummary
                font.family: Theme.uiFont
                font.weight: Font.DemiBold
            }
            Paragraph {
                text: ParentWording.privacyKept
            }
            Paragraph {
                text: ParentWording.privacyNever
            }
            Paragraph {
                text: ParentWording.privacyDevice
            }
        }
    }
}
