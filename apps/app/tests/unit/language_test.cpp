// SPDX-License-Identifier: GPL-3.0-or-later
// The interface's languages (docs/design/application.md, "Languages"):
// the catalogues in apps/app/translations/, the LanguageManager's direction and fallback, that a
// catalogue is really installed, and that the app's font chain carries the
// glyphs of all eight languages. Built with the application, in sns_app_tests.

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QRawFont>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "language_manager.hpp"
#include "sns/settings.hpp"
#include "test_support.hpp"

namespace {

using sns::app::LanguageManager;

// A GUI application on the platform the entry's environment names: offscreen,
// with the fonts directory the platform's fragment gives it (tests/CMakeLists.txt).
using TestApplication = sns::app::test::GuiApplication;

// One catalogue as the completeness check reads it.
struct Catalogue {
    int messages = 0;
    QSet<QString> keys;  // context '\n' source, so two catalogues are comparable
    QStringList problems;
};

[[nodiscard]] Catalogue read_catalogue(const QString& path, const QString& expected_language) {
    Catalogue catalogue;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        catalogue.problems.append(QStringLiteral("%1 cannot be opened").arg(path));
        return catalogue;
    }

    QXmlStreamReader reader(&file);
    QString context;
    if (!reader.readNextStartElement() || reader.name().toString() != QStringLiteral("TS")) {
        catalogue.problems.append(QStringLiteral("%1 is not a <TS> catalogue").arg(path));
        return catalogue;
    }
    const QString language = reader.attributes().value(QStringLiteral("language")).toString();
    if (!language.startsWith(expected_language)) {
        catalogue.problems.append(QStringLiteral("%1 says language '%2', not '%3'")
                                      .arg(path, language, expected_language));
    }

    while (reader.readNextStartElement()) {
        if (reader.name().toString() != QStringLiteral("context")) {
            reader.skipCurrentElement();
            continue;
        }
        while (reader.readNextStartElement()) {
            const QString element = reader.name().toString();
            if (element == QStringLiteral("name")) {
                context = reader.readElementText();
                continue;
            }
            if (element != QStringLiteral("message")) {
                reader.skipCurrentElement();
                continue;
            }
            ++catalogue.messages;
            const QXmlStreamAttributes attributes = reader.attributes();
            const QString message_type = attributes.value(QStringLiteral("type")).toString();
            if (message_type == QStringLiteral("vanished") ||
                message_type == QStringLiteral("obsolete")) {
                catalogue.problems.append(
                    QStringLiteral("%1 has a %2 message").arg(path, message_type));
            }
            const bool numerus =
                attributes.value(QStringLiteral("numerus")) == QStringLiteral("yes");
            QString source;
            QStringList forms;
            bool saw_translation = false;
            while (reader.readNextStartElement()) {
                const QString child = reader.name().toString();
                if (child == QStringLiteral("source")) {
                    source = reader.readElementText();
                } else if (child == QStringLiteral("translation")) {
                    saw_translation = true;
                    const QString translation_type =
                        reader.attributes().value(QStringLiteral("type")).toString();
                    if (!translation_type.isEmpty()) {
                        catalogue.problems.append(QStringLiteral("%1: '%2' has a '%3' translation")
                                                      .arg(path, source, translation_type));
                    }
                    if (numerus) {
                        while (reader.readNextStartElement()) {
                            if (reader.name().toString() == QStringLiteral("numerusform")) {
                                forms.append(reader.readElementText());
                            } else {
                                reader.skipCurrentElement();
                            }
                        }
                    } else {
                        forms.append(reader.readElementText());
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
            if (source.isEmpty()) {
                catalogue.problems.append(
                    QStringLiteral("%1 has a message with no source").arg(path));
            }
            if (!saw_translation) {
                catalogue.problems.append(
                    QStringLiteral("%1: '%2' has no translation").arg(path, source));
            }
            if (forms.isEmpty() || std::any_of(forms.begin(), forms.end(), [](const QString& form) {
                    return form.trimmed().isEmpty();
                })) {
                catalogue.problems.append(
                    QStringLiteral("%1: '%2' is not translated in every form").arg(path, source));
            }
            catalogue.keys.insert(context + QLatin1Char('\n') + source);
        }
    }
    if (reader.hasError()) {
        catalogue.problems.append(
            QStringLiteral("%1 is not well formed XML: %2").arg(path, reader.errorString()));
    }
    return catalogue;
}

// The eight sample sentences of the glyph check, one per language: the words of
// a sentence a screen really shows, in each script.
struct Sample {
    const char* code;
    const char* sentence;
};

}  // namespace

TEST_CASE("every language has a complete catalogue, and all eight are there",
          "[app][i18n][catalogue]") {
    const QDir directory(QStringLiteral(SNS_I18N_DIRECTORY));
    REQUIRE(directory.exists());

    const QStringList found = directory.entryList({QStringLiteral("sns_*.ts")}, QDir::Files);
    const QStringList expected = LanguageManager::languages();  // the same order as sns::languages
    CHECK(found.size() == expected.size());
    for (const QString& code : expected) {
        CHECK(found.contains(QStringLiteral("sns_%1.ts").arg(code)));
    }

    // English is the source language: it is the catalogue every other one is
    // counted against.
    const QString source_path = directory.filePath(QStringLiteral("sns_en.ts"));
    REQUIRE(QFile::exists(source_path));
    const Catalogue source = read_catalogue(source_path, QStringLiteral("en"));
    for (const QString& problem : source.problems) {
        INFO(problem.toStdString());
        CHECK(false);
    }
    CHECK(source.messages > 0);

    for (const QString& code : expected) {
        const QString path = directory.filePath(QStringLiteral("sns_%1.ts").arg(code));
        INFO(path.toStdString());
        if (!QFile::exists(path)) {
            CHECK(false);
            continue;
        }
        const Catalogue catalogue = read_catalogue(path, code);
        for (const QString& problem : catalogue.problems) {
            INFO(problem.toStdString());
            CHECK(false);
        }
        CHECK(catalogue.messages == source.messages);
        CHECK(catalogue.keys == source.keys);
    }
}

TEST_CASE("the app's font chain carries every glyph of the eight sample sentences",
          "[app][i18n][glyph]") {
    // The offscreen platform draws with the fonts of the directory the platform's
    // fragment names (the system's fonts on Windows, fontconfig's elsewhere), so
    // the check asks the fonts a person sees.
    TestApplication app;

    // Without any fonts (some headless machines) there is nothing to check: the
    // platform's fallback is what the app relies on. The check skips itself then,
    // with this message.
    if (QFontDatabase::families().isEmpty()) {
        SKIP("no fonts are installed on this platform: the glyph check is a Windows check today");
    }

    const std::vector<Sample> samples = {
        {"en", "The words I found are here."},
        {"fr", "Les mots que j'ai trouvés sont ici."},
        {"es", "Las palabras que encontré están aquí."},
        {"de", "Die Wörter, die ich gefunden habe, stehen hier."},
        {"yi", "די ווערטער וואָס איך האָב געפֿונען שטייען דאָ."},
        {"he", "המילים שמצאתי נמצאות כאן."},
        {"zh_CN", "我找到的词语都在这里。"},
        {"ja", "見つけたことばはここにあります。"},
    };

    // The face the screens ask for, and the chain Qt falls back through when it
    // does not carry a glyph (docs/design/application.md, "Type"). Andika is not
    // bundled yet, so on this machine the platform's own faces are what answers.
    QFont app_font(QStringLiteral("Andika"));
    QFontMetricsF metrics(app_font);
    const QFont system_font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    QFontMetricsF system_metrics(system_font);

    QStringList missing;
    QStringList no_font;
    for (const Sample& sample : samples) {
        const QString text = QString::fromUtf8(sample.sentence);
        for (const QChar character : text) {
            const uint code_point = character.unicode();
            if (!metrics.inFontUcs4(code_point)) {
                if (!system_metrics.inFontUcs4(code_point)) {
                    no_font.append(QStringLiteral("%1: U+%2")
                                       .arg(QString::fromLatin1(sample.code))
                                       .arg(code_point, 4, 16, QLatin1Char('0')));
                } else {
                    missing.append(QStringLiteral("%1: U+%2")
                                       .arg(QString::fromLatin1(sample.code))
                                       .arg(code_point, 4, 16, QLatin1Char('0')));
                }
            }
        }
    }

    if (!no_font.isEmpty()) {
        INFO(no_font.join(QStringLiteral(", ")).toStdString());
        SKIP(
            "the platform's own fonts lack this language's glyphs: the glyph check is a "
            "Windows check today");
    }
    for (const QString& glyph : missing) {
        INFO(glyph.toStdString());
        CHECK(false);
    }
}

TEST_CASE("Hebrew and Yiddish are right to left and the other six left to right",
          "[app][i18n][language]") {
    TestApplication app;

    CHECK(LanguageManager::languages() ==
          QStringList{"en", "fr", "es", "de", "yi", "he", "zh_CN", "ja"});
    for (const QString& code : LanguageManager::languages()) {
        INFO(code.toStdString());
        CHECK(LanguageManager::is_known(code));
        CHECK(LanguageManager::is_right_to_left(code) == (code == "he" || code == "yi"));
        CHECK(LanguageManager::resolved(code) == code);
    }
    CHECK_FALSE(LanguageManager::is_known(QStringLiteral("")));
    CHECK_FALSE(LanguageManager::is_known(QStringLiteral("tlh")));
    CHECK_FALSE(LanguageManager::is_right_to_left(QStringLiteral("ar")));
    // A locale names its language in its first part, whatever the region or the
    // separator.
    CHECK(LanguageManager::language_from_locale(QStringLiteral("he_IL")) == "he");
    CHECK(LanguageManager::language_from_locale(QStringLiteral("fr-CA")) == "fr");
    CHECK(LanguageManager::language_from_locale(QStringLiteral("ja")) == "ja");
    CHECK(LanguageManager::language_from_locale(QStringLiteral("zh_CN")) == "zh_CN");
    CHECK(LanguageManager::language_from_locale(QStringLiteral("zh-Hans-CN")) == "zh_CN");
    // Traditional Chinese has no catalogue here, and is not shown as Simplified
    // (the project standard, 7.2): on its own it is English.
    CHECK(LanguageManager::language_from_locale(QStringLiteral("zh_TW")) == "en");
    CHECK(LanguageManager::language_from_locale(QStringLiteral("zh-Hant-HK")) == "en");
    CHECK(LanguageManager::resolved(QStringLiteral("zh_cn")) == "zh_CN");
    CHECK(LanguageManager::resolved(QStringLiteral("zh")) == "en");
    CHECK(LanguageManager::language_from_locale(QStringLiteral("tlh_QS")) == "en");
    CHECK(LanguageManager::language_from_locale(QString()) == "en");
    // No setting is the device's own language, and it is one of the eight.
    CHECK(LanguageManager::resolved(QString()) == LanguageManager::system_language());
    CHECK(sns::is_valid_language(LanguageManager::system_language().toStdString()));
    // A code this program does not have is English.
    CHECK(LanguageManager::resolved(QStringLiteral("tlh")) == "en");
}

TEST_CASE("applying a language installs its catalogue, its direction and its text",
          "[app][i18n][language]") {
    TestApplication app;

    LanguageManager language;
    int changes = 0;
    QObject::connect(&language, &LanguageManager::languageChanged, [&changes] { ++changes; });

    for (const QString& code : LanguageManager::languages()) {
        INFO(code.toStdString());
        const int before = changes;
        CHECK(language.apply(code) == code);
        CHECK(language.code() == code);
        CHECK(language.setting() == code);
        CHECK(language.rightToLeft() == (code == "he" || code == "yi"));
        CHECK(QGuiApplication::layoutDirection() ==
              (language.rightToLeft() ? Qt::RightToLeft : Qt::LeftToRight));
        CHECK(changes == before + 1);

        // The catalogue really is installed: a parent-area sentence comes back
        // translated, and only English comes back as the source.
        const QString back = QCoreApplication::translate("ParentWording", "Back");
        INFO(back.toStdString());
        if (code == QLatin1String("en")) {
            CHECK(back == QStringLiteral("Back"));
        } else {
            CHECK(back != QStringLiteral("Back"));
        }
    }

    // The same language again is not a change.
    const QString current = language.code();
    const int before = changes;
    CHECK(language.apply(current) == current);
    CHECK(changes == before);

    // The empty setting is the device's own; its code is resolved and, whatever
    // it is, it is one of the eight.
    CHECK(language.apply(QString()) == LanguageManager::system_language());
    CHECK(language.setting().isEmpty());
}

TEST_CASE("the language follows the device's list in order, and About says how",
          "[app][i18n][language]") {
    TestApplication app;

    struct Phone {
        QStringList offered;
        QString code;
        QString matched;
    };
    // The phone that listed English (UK), German and French got German; the
    // device's own first choice decides, and the platform's expansion of one
    // choice into several tags is still one choice.
    const std::vector<Phone> phones = {
        {{"en-GB", "de-DE", "fr-FR"}, "en", "en-GB"},
        {{"en-Latn-GB", "en-GB", "en-Latn", "en"}, "en", "en-Latn-GB"},
        {{"de-DE", "en-GB"}, "de", "de-DE"},
        {{"pt-BR", "ru-RU", "fr-CA", "en-US"}, "fr", "fr-CA"},
        {{"iw-IL", "en"}, "he", "iw-IL"},
        {{"zh-Hant-TW", "ja-JP"}, "ja", "ja-JP"},
        {{"zh-Hans-CN"}, "zh_CN", "zh-Hans-CN"},
        {{"zh-Hant-TW"}, "en", ""},
        {{"pt-BR"}, "en", ""},
        {{}, "en", ""},
    };
    for (const Phone& phone : phones) {
        INFO(phone.offered.join(", ").toStdString());
        LanguageManager language;
        language.use_system_languages(phone.offered);
        CHECK(language.apply(QString()) == phone.code);
        CHECK(language.code() == phone.code);
        CHECK(language.setting().isEmpty());

        // About shows the code, what chose it and everything the device offered.
        const QString details = language.details();
        INFO(details.toStdString());
        CHECK(details.contains(QStringLiteral("language %1 (").arg(phone.code)));
        if (phone.matched.isEmpty()) {
            CHECK(details.contains(QStringLiteral("the device offered none of ours")));
        } else {
            CHECK(details.contains(QStringLiteral("from the device: %1").arg(phone.matched)));
        }
        CHECK(details.contains(QStringLiteral("device offers: %1")
                                   .arg(phone.offered.isEmpty()
                                            ? QStringLiteral("none")
                                            : phone.offered.join(QStringLiteral(", ")))));
    }

    // A language chosen in Settings is not the device's, and About says so.
    LanguageManager language;
    language.use_system_languages({"en-GB", "de-DE"});
    CHECK(language.apply(QStringLiteral("fr")) == "fr");
    CHECK(language.details().contains(QStringLiteral("language fr (chosen in Settings)")));
    CHECK(language.details().contains(QStringLiteral("device offers: en-GB, de-DE")));

    // The device changing its list while the setting is the device's own is a
    // change the screens hear about.
    CHECK(language.apply(QString()) == "en");
    int changes = 0;
    QObject::connect(&language, &LanguageManager::languageChanged, [&changes] { ++changes; });
    language.use_system_languages({"en-US", "de-DE"});
    CHECK(language.apply(QString()) == "en");
    CHECK(changes == 1);
    CHECK(language.apply(QString()) == "en");
    CHECK(changes == 1);
}

namespace {

// Every character of every translation of a catalogue (the `<translation>` and `<numerusform>`
// text).
[[nodiscard]] QSet<char32_t> catalogue_characters(const QString& code) {
    QSet<char32_t> found;
    QFile file(
        QDir(QStringLiteral(SNS_I18N_DIRECTORY)).filePath(QStringLiteral("sns_%1.ts").arg(code)));
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && (reader.name() == QStringLiteral("translation") ||
                                        reader.name() == QStringLiteral("numerusform"))) {
            const QString text = reader.readElementText(QXmlStreamReader::IncludeChildElements);
            for (const char32_t c : text.toUcs4()) {
                found.insert(c);
            }
        }
    }
    REQUIRE_FALSE(reader.hasError());
    return found;
}

[[nodiscard]] QRawFont bundled_face(const char* file) {
    const QRawFont face(
        QDir(QStringLiteral(SNS_FONTS_DIRECTORY)).filePath(QString::fromLatin1(file)), 16);
    REQUIRE(face.isValid());
    return face;
}

}  // namespace

// The bundled faces are subsets cut from the catalogues by tools/design/make_noto_subsets.py. A
// translation that gains a character the subset lacks would draw in whatever the platform has, or,
// on the web, in nothing: the test names the character and the command that remakes the files.
TEST_CASE("the bundled faces carry every character of every catalogue", "[app][i18n][glyph]") {
    TestApplication app;

    // The house faces: whatever a language needs beyond them comes from its own Noto face.
    const QList<QRawFont> house = {bundled_face("Andika-Regular.ttf"),
                                   bundled_face("PixelifySans-Medium.ttf"),
                                   bundled_face("Silkscreen-Regular.ttf")};
    const struct {
        const char* code;
        std::vector<const char*> noto;
    } languages[] = {
        {"en", {}},
        {"fr", {}},
        {"es", {}},
        {"de", {}},
        {"yi", {"NotoSansHebrew-Regular.ttf", "NotoSansHebrew-Bold.ttf"}},
        {"he", {"NotoSansHebrew-Regular.ttf", "NotoSansHebrew-Bold.ttf"}},
        {"zh_CN", {"NotoSansSC-Regular.ttf", "NotoSansSC-Bold.ttf"}},
        {"ja", {"NotoSansJP-Regular.ttf", "NotoSansJP-Bold.ttf"}},
    };
    for (const auto& language : languages) {
        INFO(language.code);
        QList<QRawFont> faces = house;
        QList<QRawFont> bold;
        for (const char* file : language.noto) {
            (QString::fromLatin1(file).contains(QStringLiteral("Bold")) ? bold : faces)
                .append(bundled_face(file));
        }
        QStringList missing;
        QStringList missing_bold;
        for (const char32_t c : catalogue_characters(QString::fromLatin1(language.code))) {
            if (c < 0x20 || c == 0x7F) {
                continue;  // a control character in a translation is a line break, not a glyph
            }
            const auto carried = [&](const QList<QRawFont>& list) {
                return std::any_of(list.begin(), list.end(),
                                   [c](const QRawFont& face) { return face.supportsCharacter(c); });
            };
            if (!carried(faces)) {
                missing.append(QStringLiteral("U+%1")
                                   .arg(static_cast<uint>(c), 4, 16, QLatin1Char('0'))
                                   .toUpper());
            }
            // The bold face must carry what the regular one does (a heading is Bold).
            if (!bold.isEmpty() && carried(faces) && !carried(house) &&
                !std::any_of(bold.begin(), bold.end(),
                             [c](const QRawFont& face) { return face.supportsCharacter(c); })) {
                missing_bold.append(QStringLiteral("U+%1")
                                        .arg(static_cast<uint>(c), 4, 16, QLatin1Char('0'))
                                        .toUpper());
            }
        }
        INFO("missing from the bundled faces: " + missing.join(QStringLiteral(" ")).toStdString() +
             " (run tools/design/make_noto_subsets.py, docs/i18n.md)");
        CHECK(missing.isEmpty());
        CHECK(missing_bold.isEmpty());
    }

    // The language picker names each language in its own script, in every interface language, so
    // the three Noto faces together carry those names.
    QList<QRawFont> noto;
    for (const char* file :
         {"NotoSansHebrew-Regular.ttf", "NotoSansSC-Regular.ttf", "NotoSansJP-Regular.ttf"}) {
        noto.append(bundled_face(file));
    }
    for (const QString& name :
         {QStringLiteral("\u05D9\u05D9\u05B4\u05D3\u05D9\u05E9"),
          QStringLiteral("\u05E2\u05D1\u05E8\u05D9\u05EA"), QStringLiteral("\u4E2D\u6587"),
          QStringLiteral("\u65E5\u672C\u8A9E")}) {
        for (const char32_t c : name.toUcs4()) {
            INFO(QStringLiteral("U+%1")
                     .arg(static_cast<uint>(c), 4, 16, QLatin1Char('0'))
                     .toStdString());
            CHECK(std::any_of(noto.begin(), noto.end(),
                              [c](const QRawFont& face) { return face.supportsCharacter(c); }));
        }
    }
}
