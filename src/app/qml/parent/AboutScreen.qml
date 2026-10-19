import QtQuick
import QtQuick.Layouts

// About, licences and privacy (docs/design/dashboard.md, "Settings"). Plain text:
// no link to anywhere, because the application has nowhere to go.
Flickable {
    id: root

    required property var controller

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
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: privacySummary.implicitHeight + 2 * ParentTheme.space4
            radius: ParentTheme.radius
            color: ParentTheme.card
            border.width: 1
            border.color: ParentTheme.gold

            ColumnLayout {
                id: privacySummary
                anchors.fill: parent
                anchors.margins: ParentTheme.space4
                spacing: ParentTheme.space1
                Text {
                    Layout.fillWidth: true
                    text: ParentWording.privacyHeading
                    font.family: ParentTheme.uiFont
                    font.pixelSize: ParentTheme.sizeLead
                    font.weight: Font.ExtraBold
                    color: ParentTheme.ink
                }
                Text {
                    Layout.fillWidth: true
                    text: ParentWording.privacySummary
                    font.family: ParentTheme.uiFont
                    font.pixelSize: ParentTheme.sizeBody
                    color: ParentTheme.inkSecondary
                    wrapMode: Text.WordWrap
                }
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
            Paragraph {
                text: ParentWording.licencesFull
            }
            // The full notices of this build: the same text as NOTICES.txt
            // beside the programs, embedded as :/notices/NOTICES.txt
            // (cmake/Notices.cmake). It has a box and a scroll of its own - a
            // licence text is long, and the page should not have to be as long
            // as the licences to reach the privacy section below it.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 360
                color: ParentTheme.card
                border.width: ParentTheme.hair
                border.color: ParentTheme.outline
                radius: ParentTheme.radius

                Flickable {
                    id: notices
                    objectName: "licencesView"
                    anchors.fill: parent
                    anchors.margins: ParentTheme.space3
                    contentWidth: width
                    contentHeight: noticeText.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick

                    Text {
                        id: noticeText
                        // The wording check leaves this one text alone
                        // (tests/app/parent_ui/preview_main.cpp): it is quoted
                        // legal text, not a sentence about a child.
                        objectName: "licenceNotices"
                        width: notices.width
                        text: root.controller.licenceNotices
                        font.pixelSize: ParentTheme.sizeSmall
                        color: ParentTheme.ink
                        wrapMode: Text.WordWrap
                    }
                }
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
