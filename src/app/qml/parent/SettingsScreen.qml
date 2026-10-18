pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

// What a parent sets (docs/design/dashboard.md, "Settings"). Every value is one
// the table allows; the controller refuses any other, and the store holds the
// result on this device and nowhere else.
Flickable {
    id: root

    required property var controller

    // The parent area's direction, from the shell: Hebrew and Yiddish mirror the
    // rows and every choice.
    LayoutMirroring.enabled: ParentTheme.rightToLeft
    LayoutMirroring.childrenInherit: true

    readonly property var s: controller.settings

    function onOff() {
        return [{value: true, label: ParentWording.valueOn}, {value: false, label: ParentWording.valueOff}];
    }

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * ParentTheme.space6
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    ColumnLayout {
        id: column
        x: Math.max(ParentTheme.space4, (root.width - width) / 2)
        y: ParentTheme.space6
        width: Math.min(root.width - 2 * ParentTheme.space4, ParentTheme.contentWidth)
        spacing: ParentTheme.space8

        Section {
            Layout.fillWidth: true
            title: ParentWording.tabSettings

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsHelp
                hint: ParentWording.settingsHelpHint
                PChoice {
                    title: ParentWording.settingsHelp
                    options: root.onOff()
                    current: root.s.help
                    onChosen: value => root.controller.setSetting("help", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsBoard
                hint: ParentWording.settingsBoardHint
                PChoice {
                    title: ParentWording.settingsBoard
                    options: [
                        {value: "standard", label: ParentWording.valueStandard},
                        {value: "phonics", label: ParentWording.valuePhonics, enabled: root.controller.phonicsAvailable}
                    ]
                    current: root.s.board
                    onChosen: value => root.controller.setSetting("board", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                visible: root.controller.phonicsAvailable
                title: ParentWording.settingsPhase
                PChoice {
                    title: ParentWording.settingsPhase
                    options: root.controller.phases.map(name => ({value: name, label: name}))
                    current: root.s.phase
                    onChosen: value => root.controller.setSetting("phase", value)
                }
            }

            SettingRow {
                id: focusRow
                Layout.fillWidth: true
                visible: root.controller.focusChoices.length > 0
                title: ParentWording.settingsFocus
                hint: ParentWording.settingsFocusHint

                Flow {
                    Layout.fillWidth: true
                    spacing: ParentTheme.space2

                    Repeater {
                        model: root.controller.focusChoices

                        PButton {
                            required property string modelData
                            text: modelData
                            compact: true
                            checkable: true
                            checked: root.s.focusSounds.indexOf(modelData) >= 0
                            Accessible.name: ParentWording.settingsFocus + ": " + modelData
                            onClicked: root.controller.toggleFocusSound(modelData)
                        }
                    }
                }

                // How each ticked sound is going, in the words of the dashboard.
                Repeater {
                    model: root.controller.dashboard.sounds.filter(row => row.focus)

                    ColumnLayout {
                        id: tickedRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            Layout.fillWidth: true
                            textFormat: Text.StyledText
                            wrapMode: Text.WordWrap
                            font.pixelSize: ParentTheme.sizeSmall
                            color: ParentTheme.ink
                            text: "<i>" + tickedRow.modelData.letters + "</i>: "
                                  + (tickedRow.modelData.boardsMet > 0
                                         ? ParentWording.onBoardsFoundOn(tickedRow.modelData.boardsMet, tickedRow.modelData.boardsFound)
                                         : ParentWording.notMetYet)
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: tickedRow.modelData.fewWords
                            wrapMode: Text.WordWrap
                            font.pixelSize: ParentTheme.sizeSmall
                            color: ParentTheme.inkSecondary
                            text: ParentWording.fewWords
                        }
                    }
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsBoardSize
                hint: ParentWording.settingsBoardSizeHint
                PChoice {
                    title: ParentWording.settingsBoardSize
                    options: root.s.board === "standard"
                        ? [{value: 3, label: "3x3"}, {value: 4, label: "4x4"},
                           {value: 5, label: "5x5"}, {value: 6, label: "6x6"}]
                        : [{value: 3, label: "3x3"}, {value: 4, label: "4x4"}]
                    current: root.s.boardSize
                    onChosen: value => root.controller.setSetting("boardSize", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsSoundItOut
                PChoice {
                    title: ParentWording.settingsSoundItOut
                    options: [
                        {value: "full", label: ParentWording.valueFull},
                        {value: "word", label: ParentWording.valueWordOnly},
                        {value: "off", label: ParentWording.valueOff}
                    ]
                    current: root.s.soundItOut
                    onChosen: value => root.controller.setSetting("soundItOut", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsCelebration
                PChoice {
                    title: ParentWording.settingsCelebration
                    options: [
                        {value: "full", label: ParentWording.valueFull},
                        {value: "quiet", label: ParentWording.valueQuiet},
                        {value: "off", label: ParentWording.valueOff}
                    ]
                    current: root.s.celebrationSounds
                    onChosen: value => root.controller.setSetting("celebrationSounds", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsSound
                PChoice {
                    title: ParentWording.settingsSound
                    options: root.onOff()
                    current: root.s.sound
                    onChosen: value => root.controller.setSetting("sound", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsVibration
                PChoice {
                    title: ParentWording.settingsVibration
                    options: root.onOff()
                    current: root.s.vibration
                    onChosen: value => root.controller.setSetting("vibration", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsShake
                PChoice {
                    title: ParentWording.settingsShake
                    options: root.onOff()
                    current: root.s.shake
                    onChosen: value => root.controller.setSetting("shake", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsMotion
                hint: ParentWording.settingsMotionHint
                PChoice {
                    title: ParentWording.settingsMotion
                    options: [
                        {value: "system", label: ParentWording.valueSystem},
                        {value: "reduced", label: ParentWording.valueReduced}
                    ]
                    current: root.s.motion
                    onChosen: value => root.controller.setSetting("motion", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsColour
                hint: ParentWording.settingsColourHint
                PChoice {
                    title: ParentWording.settingsColour
                    options: [
                        {value: "system", label: ParentWording.valueSystem},
                        {value: "light", label: ParentWording.valueLight},
                        {value: "dark", label: ParentWording.valueDark}
                    ]
                    current: root.s.colourScheme
                    onChosen: value => root.controller.setSetting("colourScheme", value)
                }
            }

            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsTextSize
                PChoice {
                    title: ParentWording.settingsTextSize
                    options: [{value: 100, label: "1"}, {value: 125, label: "1.25"}, {value: 150, label: "1.5"}]
                    current: root.s.textSizePercent
                    onChosen: value => root.controller.setSetting("textSizePercent", value)
                }
            }

            // Offered only if a font whose licence lets it be bundled exists.
            SettingRow {
                Layout.fillWidth: true
                visible: root.controller.schoolFonts.length > 0
                title: ParentWording.settingsSchoolFont
                PChoice {
                    title: ParentWording.settingsSchoolFont
                    options: [{value: "", label: ParentWording.valueOff}].concat(
                        root.controller.schoolFonts.map(name => ({value: name, label: name})))
                    current: root.s.schoolFont
                    onChosen: value => root.controller.setSetting("schoolFont", value)
                }
            }

            // The interface's language. The board and the words on it are the
            // same English board in every language; the screens around it change,
            // and Hebrew and Yiddish turn the layout around.
            SettingRow {
                Layout.fillWidth: true
                title: ParentWording.settingsLanguage
                hint: ParentWording.settingsLanguageHint
                PChoice {
                    title: ParentWording.settingsLanguage
                    options: [{value: "", label: ParentWording.valueSystem}].concat(
                        ParentWording.languageChoices)
                    current: root.s.language
                    onChosen: value => root.controller.setSetting("language", value)
                }
            }
        }

        Section {
            Layout.fillWidth: true
            title: ParentWording.resetHeading

            ResetStep {
                Layout.fillWidth: true
                label: ParentWording.resetBoards
                says: ParentWording.resetBoardsSays
                onConfirmed: root.controller.resetBoardsAndFinds()
            }
            ResetStep {
                Layout.fillWidth: true
                label: ParentWording.resetStars
                says: ParentWording.resetStarsSays
                onConfirmed: root.controller.resetStarsAndStickers()
            }
        }
    }
}
