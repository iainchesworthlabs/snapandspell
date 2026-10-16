#include "system_colour_scheme.hpp"

#include <QGuiApplication>
#include <QStyleHints>

namespace wjs::app {

SystemColourScheme::SystemColourScheme(QObject* parent) : QObject(parent) {
    if (auto* hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this, &SystemColourScheme::darkChanged);
    }
}

bool SystemColourScheme::dark() const {
    const auto* hints = QGuiApplication::styleHints();
    return hints != nullptr && hints->colorScheme() == Qt::ColorScheme::Dark;
}

}  // namespace wjs::app
