// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

class QTranslator;

namespace sns::app {

// The interface's language, in one place: the catalogue installed on the
// application, the layout direction and the code the screens read
// (docs/design/application.md, "Languages").
//
// The parent's Language setting is "" for the device's own language, or one of
// the eight codes (`languages`). The device's language is used when it is one of
// the eight and English otherwise. Applying a language in the parent area
// installs its catalogue at once and tells the window, which sets
// `LayoutMirroring` from `rightToLeft`; the engine's bindings are retranslated
// by whoever owns the engine, through `languageChanged` (main.cpp connects it to
// `QQmlEngine::retranslate`).
//
// The catalogues are the `:/i18n/sns_<code>.qm` resources built from `i18n/`.
// The board, the dice letters and the word list are never translated: the game
// is an English word game and the chrome around it is what changes.
class LanguageManager : public QObject {
    Q_OBJECT

    // The code in force: one of the eight, never empty ("en" before apply()).
    Q_PROPERTY(QString code READ code NOTIFY languageChanged)
    // The setting it came from: "" for the device's own, else a code.
    Q_PROPERTY(QString setting READ setting NOTIFY languageChanged)
    // Hebrew and Yiddish read right to left; the other six left to right.
    Q_PROPERTY(bool rightToLeft READ rightToLeft NOTIFY languageChanged)
    // What About shows so that a report of the wrong language can be answered
    // from a screenshot: the code in force, what decided it, and the languages
    // the device offered, in its order.
    Q_PROPERTY(QString details READ details NOTIFY languageChanged)

public:
    explicit LanguageManager(QObject* parent = nullptr);
    ~LanguageManager() override;

    // The eight codes, in the order the store and `sns::languages` have them.
    [[nodiscard]] static QStringList languages();
    [[nodiscard]] static bool is_known(const QString& code);
    // Hebrew and Yiddish, and nothing else, are right to left.
    [[nodiscard]] static bool is_right_to_left(const QString& code);
    // The code a locale name ("he_IL", "fr-CA", "ja") names, by the rules of
    // `sns::choose_language` (sns/language_choice.hpp): English otherwise.
    [[nodiscard]] static QString language_from_locale(const QString& locale_name);
    // The languages the device offers, in the order the person ranked them
    // ("en-GB", "de-DE"): the system locale's UI languages.
    [[nodiscard]] static QStringList system_languages();
    // The device's own language: the first of those that has a catalogue.
    [[nodiscard]] static QString system_language();
    // The code a setting comes to: "" is the device's, an unknown code is English.
    [[nodiscard]] static QString resolved(const QString& setting);

    [[nodiscard]] QString code() const { return code_; }
    [[nodiscard]] QString setting() const { return setting_; }
    [[nodiscard]] bool rightToLeft() const;
    [[nodiscard]] QString details() const;

    // The languages the device offers, instead of the system's own: for a test
    // that has to be a German phone. Takes effect at the next `apply`.
    void use_system_languages(const QStringList& offered);

    // Applies a setting ("" or one of the eight), installs its catalogue, sets
    // the application's layout direction and returns the code in force. Doing
    // the same language again changes nothing and emits nothing.
    Q_INVOKABLE QString apply(const QString& setting);

signals:
    // The code changed: the screens retranslate and mirror themselves from here.
    void languageChanged();

private:
    void install(const QString& code);

    QString code_ = QStringLiteral("en");
    QString setting_;
    QStringList offered_ = system_languages();
    // The tag of `offered_` that chose the device's language, or empty.
    QString matched_;
    std::unique_ptr<QTranslator> translator_;
};

}  // namespace sns::app
