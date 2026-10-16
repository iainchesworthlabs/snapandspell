pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts

// One choice among a few: a row of buttons, the chosen one pressed in. Each
// option is { value, label } and may carry `enabled: false`. It wraps to another
// line on a narrow screen, and every button is 48 dp high.
Flow {
    id: root

    property var options: []
    property var current
    // The name for assistive technology: "Help", and each option is "Help: On".
    property string title

    signal chosen(var value)

    Layout.fillWidth: true
    spacing: ParentTheme.space2

    Accessible.role: Accessible.RadioButton
    Accessible.name: root.title

    Repeater {
        model: root.options
        PButton {
            required property var modelData
            text: modelData.label
            checkable: true
            checked: root.current === modelData.value
            enabled: modelData.enabled === undefined || modelData.enabled
            Accessible.name: root.title.length > 0 ? root.title + ": " + modelData.label : modelData.label
            onClicked: root.chosen(modelData.value)
        }
    }
}
