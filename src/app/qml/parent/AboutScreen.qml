import QtQuick
import QtQuick.Layouts

// About, licences and privacy (docs/design/dashboard.md, "Settings"). Plain text:
// no link to anywhere, because the application has nowhere to go.
Flickable {
    id: root

    required property var controller

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * ParentTheme.space6
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    component Paragraph: Text {
        Layout.fillWidth: true
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

        Section {
            Layout.fillWidth: true
            title: ParentWording.aboutHeading
            Paragraph {
                text: ParentWording.version(root.controller.version)
            }
        }

        Section {
            Layout.fillWidth: true
            title: ParentWording.licencesHeading
            Paragraph {
                text: ParentWording.licenceNotice
            }
            Paragraph {
                text: ParentWording.licenceParts
            }
        }

        Section {
            Layout.fillWidth: true
            title: ParentWording.privacyHeading
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
