// SPDX-License-Identifier: GPL-3.0-or-later
#include "language_manager.hpp"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QLocale>
#include <QTranslator>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "sns/language_choice.hpp"
#include "sns/profiling.hpp"
#include "sns/settings.hpp"

// The catalogues are a resource of the sns_i18n library. A resource is linked
// out of a static library unless something refers to it, and this is that
// reference. It is made at global scope because Q_INIT_RESOURCE declares its
// extern function where it is expanded: inside sns::app it would declare
// sns::app::qInitResources_..., while the generated object defines the global
// one, and clang-cl + lld-link reject that link.
static void init_translations_resource() {
    Q_INIT_RESOURCE(sns_translations);
}

namespace sns::app {
namespace {

// A code from the library's list as Qt text, so the eight are named once
// (sns/settings.hpp) and the store and the catalogue cannot drift apart.
[[nodiscard]] QString code_text(std::string_view code) {
    return QString::fromLatin1(code.data(), static_cast<qsizetype>(code.size()));
}

// The code in the library's list that `code` names, whatever its case, or empty.
[[nodiscard]] QString canonical(const QString& code) {
    return code_text(sns::canonical_language(code.toUtf8().toStdString()));
}

// The tags in the order given, as the library's rules read them.
[[nodiscard]] std::vector<std::string> tags_of(const QStringList& offered) {
    std::vector<std::string> tags;
    tags.reserve(static_cast<std::size_t>(offered.size()));
    for (const QString& tag : offered) {
        tags.push_back(tag.toStdString());
    }
    return tags;
}

}  // namespace

LanguageManager::LanguageManager(QObject* parent) : QObject(parent) {
    // The catalogues are a resource of this library. A resource is linked out of
    // a static library unless something refers to it, and the helper is that
    // reference (made at global scope; see the note above).
    ::init_translations_resource();
}

LanguageManager::~LanguageManager() {
    if (translator_ && QCoreApplication::instance() != nullptr) {
        QCoreApplication::removeTranslator(translator_.get());
    }
}

QStringList LanguageManager::languages() {
    QStringList codes;
    for (const std::string_view code : sns::languages) {
        codes.append(code_text(code));
    }
    return codes;
}

bool LanguageManager::is_known(const QString& code) {
    return !canonical(code).isEmpty();
}

bool LanguageManager::is_right_to_left(const QString& code) {
    // Hebrew and Yiddish, and nothing else (docs/design/application.md, "Languages").
    return code == QLatin1String("he") || code == QLatin1String("yi");
}

QString LanguageManager::language_from_locale(const QString& locale_name) {
    return code_text(sns::choose_language(locale_name.toStdString()).code);
}

QStringList LanguageManager::system_languages() {
    // The person's ranked list, not the one locale name: a phone that lists
    // English (UK), German and French has a locale of its own whose language is
    // not necessarily the first of them.
    return QLocale::system().uiLanguages();
}

QString LanguageManager::system_language() {
    return code_text(sns::choose_language(tags_of(system_languages())).code);
}

QString LanguageManager::resolved(const QString& setting) {
    if (setting.isEmpty()) {
        return system_language();
    }
    const QString code = canonical(setting);
    return code.isEmpty() ? QStringLiteral("en") : code;
}

bool LanguageManager::rightToLeft() const {
    return is_right_to_left(code_);
}

QString LanguageManager::details() const {
    QString source;
    if (!setting_.isEmpty()) {
        source = QStringLiteral("chosen in Settings");
    } else if (matched_.isEmpty()) {
        source = QStringLiteral("the device offered none of ours");
    } else {
        source = QStringLiteral("from the device: %1").arg(matched_);
    }
    const QString offered =
        offered_.isEmpty() ? QStringLiteral("none") : offered_.join(QStringLiteral(", "));
    return QStringLiteral("language %1 (%2)\ndevice offers: %3").arg(code_, source, offered);
}

void LanguageManager::use_system_languages(const QStringList& offered) {
    offered_ = offered;
}

QString LanguageManager::apply(const QString& setting) {
    const sns::LanguageChoice device = sns::choose_language(tags_of(offered_));
    const QString before = details();
    matched_ = QString::fromStdString(device.matched);
    // The setting the screens read back: "" for the device's own. An unknown
    // code asks for a language this program does not have, so it is English and
    // the setting is not one of the eight either: it is kept as the system's.
    const QString stored = canonical(setting);
    const QString code = setting.isEmpty() ? code_text(device.code)
                         : stored.isEmpty() ? QStringLiteral("en")
                                            : stored;

    if (code == code_ && translator_) {
        const bool changed = stored != setting_;
        setting_ = stored;
        if (changed || details() != before) {
            emit languageChanged();
        }
        return code_;
    }

    install(code);
    code_ = code;
    setting_ = stored;

    if (qobject_cast<QGuiApplication*>(QCoreApplication::instance()) != nullptr) {
        QGuiApplication::setLayoutDirection(rightToLeft() ? Qt::RightToLeft : Qt::LeftToRight);
    }
    emit languageChanged();
    return code_;
}

void LanguageManager::install(const QString& code) {
    // The catalogue's read and install, on the GUI thread: it is a resource, so
    // it is quick, and it is timed so a trace shows when a language change pays
    // it (once per change, not per frame).
    auto zone = sns::profiling::zone<"data.load_catalogue">();
    if (translator_) {
        QCoreApplication::removeTranslator(translator_.get());
        translator_.reset();
    }
    auto translator = std::make_unique<QTranslator>();
    // A catalogue that is not there is not fatal: the source strings are
    // English and the catalogue check and the language smoke would say so.
    if (!translator->load(QStringLiteral(":/i18n/sns_%1.qm").arg(code))) {
        qWarning("LanguageManager: no catalogue for '%s'; the source wording is used.",
                 qPrintable(code));
    }
    if (QCoreApplication::instance() != nullptr) {
        QCoreApplication::installTranslator(translator.get());
    }
    translator_ = std::move(translator);
}

}  // namespace sns::app
