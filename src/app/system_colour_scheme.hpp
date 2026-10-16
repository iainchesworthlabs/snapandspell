#pragma once

#include <QtQml/qqmlregistration.h>
#include <QObject>

namespace wjs::app {

// Whether the system is in dark mode, for the chrome around the board (the board
// itself is dark in both: docs/design/application.md, "Colour and contrast").
// A wrapper on QStyleHints::colorSchemeChanged and not a QML binding on
// Qt.styleHints.colorScheme, whose change notification QML does not get (the
// lesson of Cubeulator's SystemTheme).
class SystemColourScheme : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool dark READ dark NOTIFY darkChanged)

public:
    explicit SystemColourScheme(QObject* parent = nullptr);

    [[nodiscard]] bool dark() const;

signals:
    void darkChanged();
};

}  // namespace wjs::app
