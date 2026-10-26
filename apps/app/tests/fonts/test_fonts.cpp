// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_fonts.hpp"

#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QString>
#include <QtLogging>
#include <array>

namespace wjs::app::test {

bool load_house_fonts() {
    static const bool loaded = [] {
        // The same five files as WJS_FONT_FILES in apps/app/CMakeLists.txt.
        constexpr std::array<const char*, 5> files = {
            "PixelifySans-Medium.ttf", "PixelifySans-Bold.ttf", "Silkscreen-Regular.ttf",
            "Andika-Regular.ttf", "Andika-Bold.ttf"};
        const QDir directory(QStringLiteral(WJS_TEST_FONTS_DIRECTORY));
        bool all = true;
        for (const char* file : files) {
            const QString path = directory.filePath(QString::fromLatin1(file));
            if (QFontDatabase::addApplicationFont(path) < 0) {
                qWarning() << "The test font" << path << "could not be loaded.";
                all = false;
            }
        }
        QGuiApplication::setFont(QFont(QStringLiteral("Andika")));
        return all;
    }();
    return loaded;
}

}  // namespace wjs::app::test
