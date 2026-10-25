// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_fonts.hpp"

#include <QDir>
#include <QFontDatabase>
#include <QString>
#include <QtLogging>
#include <array>

namespace wjs::app::test {

bool load_house_fonts() {
    static const bool loaded = [] {
        // The same six files as WJS_FONT_FILES in apps/app/CMakeLists.txt.
        constexpr std::array<const char*, 6> files = {
            "Archivo-Regular.ttf",    "Archivo-SemiBold.ttf", "Archivo-ExtraBold.ttf",
            "IBMPlexMono-Medium.ttf", "Andika-Regular.ttf",   "Andika-Bold.ttf"};
        const QDir directory(QStringLiteral(WJS_TEST_FONTS_DIRECTORY));
        bool all = true;
        for (const char* file : files) {
            const QString path = directory.filePath(QString::fromLatin1(file));
            if (QFontDatabase::addApplicationFont(path) < 0) {
                qWarning() << "The test font" << path << "could not be loaded.";
                all = false;
            }
        }
        return all;
    }();
    return loaded;
}

}  // namespace wjs::app::test
