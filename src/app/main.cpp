#include <QDate>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>

#include "parent/parent_controller.hpp"
#include "solo/fake_solo_game.hpp"
#include "solo/solo_game_factory.hpp"
#include "solo/solo_game_setup.hpp"
#include "wjs/calendar.hpp"
#include "wjs/platform/paths.hpp"
#include "wjs/settings.hpp"
#include "wjs/store.hpp"
#include "wjs/version.hpp"

namespace {

// A filesystem path from Qt text. Going through UTF-16 keeps it portable: the
// standard converts to the native encoding.
[[nodiscard]] std::filesystem::path to_path(const QString& text) {
    return std::filesystem::path{text.toStdU16String()};
}

// The value after `flag` in the arguments, if the program was started with one.
[[nodiscard]] std::optional<QString> value_of(const QStringList& arguments, const QString& flag) {
    const qsizetype at = arguments.indexOf(flag);
    if (at < 0 || at + 1 >= arguments.size()) {
        return std::nullopt;
    }
    return arguments.at(at + 1);
}

// The device's own calendar, the one the game and the parent area share: Qt's
// Julian day of 1970-01-01 is 2440588.
[[nodiscard]] wjs::Day device_today() {
    return static_cast<wjs::Day>(QDate::currentDate().toJulianDay() - 2440588);
}

// The program's one store: the file `--store <file>` names, otherwise the
// per-user one, and a store that is never written when there is nowhere to keep
// one. The game and the parent area both work on it, so that a change made in the
// one is what the other reads; the stand-in game too, which the parent's settings
// reach as they reach the real one.
[[nodiscard]] wjs::OpenResult open_store(const QStringList& arguments) {
    std::filesystem::path file;
    if (const auto given = value_of(arguments, QStringLiteral("--store"))) {
        file = to_path(*given);
    } else if (const std::filesystem::path base = wjs::platform::user_data_directory();
               !base.empty()) {
        file = wjs::default_store_path(base);
    }
    if (file.empty()) {
        return {wjs::Store::in_memory(), wjs::LoadStatus::created, {}};
    }
    return wjs::Store::open(file);
}

// What the real game is to be made from, when the program was told where a phase
// is: `--phase-directory <dir>`, with `--phase <name>`, `--dictionary <file>` and
// `--seed <number>` to go with it. Without the directory there is no phase to play
// (the repository has none) and the stand-in game is played.
[[nodiscard]] std::optional<wjs::app::SoloGameOptions> real_game_options(
    const QStringList& arguments) {
    const auto directory = value_of(arguments, QStringLiteral("--phase-directory"));
    if (!directory) {
        for (const char* flag : {"--phase", "--seed"}) {
            if (arguments.contains(QString::fromLatin1(flag))) {
                qWarning() << flag << "applies only with --phase-directory; ignored.";
            }
        }
        return std::nullopt;
    }

    wjs::app::SoloGameOptions options;
    options.phase_directory = to_path(*directory);
    if (const auto phase = value_of(arguments, QStringLiteral("--phase"))) {
        options.phase = phase->toStdString();
    }
    if (const auto dictionary = value_of(arguments, QStringLiteral("--dictionary"))) {
        options.dictionary = to_path(*dictionary);
    } else {
        options.dictionary =
            wjs::platform::executable_path().parent_path() / "dictionaries" / "en.txt";
    }
    if (const auto seed = value_of(arguments, QStringLiteral("--seed"))) {
        bool ok = false;
        const std::uint32_t number = seed->toUInt(&ok);
        if (ok) {
            options.seed = number;
        } else {
            qWarning() << "--seed wants a whole number; a random one is used.";
        }
    }
    options.today = device_today;
    return options;
}

// The stand-in game, made from the parent's settings as SoloGameSetup::make()
// makes the real one: the help switch and the size to offer first.
[[nodiscard]] std::unique_ptr<wjs::app::SoloGame> make_stand_in(const wjs::Settings& settings) {
    auto game = std::make_unique<wjs::app::FakeSoloGame>();
    game->setHelpAllowed(settings.help);
    game->chooseSize(static_cast<std::size_t>(settings.board_size));
    return game;
}

// What to tell the person about the store's file, or null when it was read (or made)
// as it should be.
[[nodiscard]] const char* store_note(wjs::LoadStatus status) noexcept {
    switch (status) {
        case wjs::LoadStatus::created:
        case wjs::LoadStatus::loaded:
        case wjs::LoadStatus::migrated:
            return nullptr;
        case wjs::LoadStatus::unreadable:
            return "The store's file was damaged: it is set aside and a new one is started.";
        case wjs::LoadStatus::newer_version:
            return "The store's file was written by a later version: it is left alone and "
                   "nothing is saved this run.";
        case wjs::LoadStatus::cannot_read:
            return "The store's file could not be read: it is left alone and nothing is saved "
                   "this run.";
    }
    return nullptr;
}

}  // namespace

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("WordJewelsSolver"));
    QGuiApplication::setOrganizationName(QStringLiteral("Iain Chesworth Labs"));
    QGuiApplication::setApplicationVersion(QString::fromUtf8(
        wjs::project_version.data(), static_cast<qsizetype>(wjs::project_version.size())));
    const QStringList arguments = application.arguments();

    // Everything the engine points at is declared before it, which is gone first:
    // the store, which the game and the parent area share, then the game's phase
    // and the parent area's controller.
    wjs::OpenResult opened = open_store(arguments);
    wjs::Store& store = opened.store;
    if (const char* note = store_note(opened.status)) {
        qWarning() << note;
    }

    std::unique_ptr<wjs::app::SoloGameSetup> solo_setup;
    std::optional<wjs::app::ScopedSoloGameMaker> solo_maker;
    if (const auto options = real_game_options(arguments)) {
        auto setup = wjs::app::SoloGameSetup::open(*options, store);
        if (!setup) {
            qCritical().noquote() << QString::fromStdString(setup.error());
            return 2;
        }
        solo_setup = *std::move(setup);
        if (!solo_setup->reviewed()) {
            qWarning() << "The phase files are test data, not a reviewed phase; they are for a"
                          " run on this machine and are not content for a child.";
        }
        solo_maker.emplace([setup = solo_setup.get()] { return setup->make(); });
    } else {
        solo_maker.emplace([&store] { return make_stand_in(store.settings()); });
    }

    // The parent area's controller, on the same store, the same calendar and the
    // phases the game plays (none with the stand-in).
    wjs::app::ParentController parent;
    parent.useCalendar(device_today, 0);
    parent.useStore(&store, opened.status);
    parent.usePhases(solo_setup ? &solo_setup->phases() : nullptr);

    QQmlApplicationEngine engine;
    engine.setInitialProperties(
        {{QStringLiteral("parentController"), QVariant::fromValue(&parent)}});
    engine.loadFromModule("WordJewelsSolver", "Main");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return application.exec();
}
