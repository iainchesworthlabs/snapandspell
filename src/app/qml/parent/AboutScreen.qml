import QtQuick
import QtQuick.Layouts

// About and privacy (docs/design/dashboard.md, "Settings"). Plain text: no link
// to anywhere, because the application has nowhere to go. The licences have
// their own page (LicencesScreen.qml), reached by the one button here, so the
// legal text is not the longest thing on this page.
Flickable {
    id: root

    required property var controller

    signal licencesRequested

    // The parent area's direction, from the shell.
    LayoutMirroring.enabled: ParentTheme.rightToLeft
    LayoutMirroring.childrenInherit: true

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * ParentTheme.space6
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    component Paragraph: Text {
        Layout.fillWidth: true
        font.family: ParentTheme.uiFont
        font.pixelSize: ParentTheme.sizeBody
        color: ParentTheme.ink
        wrapMode: Text.WordWrap
    }

    ColumnLayout {
        id: column
        x: Math.max(ParentTheme.space4, (root.width - width) / 2)
        y: ParentTheme.space6
        width: Math.min(root.width - 2 * ParentTheme.space4, ParentTheme.contentWidth)
        spacing: ParentTheme.space8

        ColumnLayout {
            Layout.fillWidth: true
            spacing: ParentTheme.space1
            Text {
                Layout.fillWidth: true
                text: ParentWording.appName
                font.family: ParentTheme.uiFont
                font.pixelSize: ParentTheme.sizeTitle
                font.weight: Font.ExtraBold
                color: ParentTheme.ink
            }
            Text {
                Layout.fillWidth: true
                text: ParentWording.tagline
                font.family: ParentTheme.fontFamily
                font.pixelSize: ParentTheme.sizeLead
                color: ParentTheme.inkSecondary
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
                Layout.topMargin: ParentTheme.space2
                text: root.controller.versionDetails
                font.family: ParentTheme.monoFont
                font.pixelSize: ParentTheme.sizeSmall
                color: ParentTheme.inkSecondary
                wrapMode: Text.WordWrap
            }
        }

        // The way to the Licences page: its own screen, so the legal text
        // scrolls there and not through this page.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: linkColumn.implicitHeight + 2 * ParentTheme.space4
            radius: ParentTheme.radius
            color: ParentTheme.card
            border.width: 1
            border.color: ParentTheme.line

            ColumnLayout {
                id: linkColumn
                anchors.fill: parent
                anchors.margins: ParentTheme.space4
                spacing: ParentTheme.space2

                PButton {
                    Layout.fillWidth: true
                    text: ParentWording.aboutLicencesLink
                    onClicked: root.licencesRequested()
                }
                Text {
                    Layout.fillWidth: true
                    text: ParentWording.aboutLicencesLinkHint
                    font.family: ParentTheme.uiFont
                    font.pixelSize: ParentTheme.sizeSmall
                    color: ParentTheme.inkSecondary
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
                font.family: ParentTheme.uiFont
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
