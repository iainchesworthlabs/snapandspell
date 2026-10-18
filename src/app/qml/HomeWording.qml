pragma Singleton
import QtQuick

// The names of home's three pictures. There is no text on the screen: a picture
// says what it does, and these are what a screen reader hears when it is pressed
// (docs/design/application.md, "Screens and navigation").
QtObject {
    readonly property string soloPicture: qsTr("Play on my own")
    readonly property string familyPicture: qsTr("Play with the family")
    readonly property string forGrownUps: qsTr("For grown-ups")
}
