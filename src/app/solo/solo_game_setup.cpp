#include "solo_game_setup.hpp"

#include <random>
#include <utility>

#include "real_solo_game.hpp"
#include "wjs/game_session.hpp"

namespace wjs::app {
namespace {

// A path in a sentence, as UTF-8 whatever the platform's own encoding.
[[nodiscard]] std::string text_of(const std::filesystem::path& path) {
    const std::u8string utf8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

[[nodiscard]] wjs::BoardSize size_of(int side) noexcept {
    return wjs::board_size_for(static_cast<std::size_t>(side)).value_or(wjs::BoardSize::three);
}

}  // namespace

SoloGameSetup::SoloGameSetup(wjs::Dictionary dictionary, wjs::PhaseSet phases, std::string phase,
                             wjs::Store store, wjs::LoadStatus store_status,
                             std::optional<std::uint32_t> seed, std::function<wjs::Day()> today)
    : dictionary_(std::move(dictionary)),
      phases_(std::move(phases)),
      phase_(phases_.find(phase)),
      store_(std::move(store)),
      store_status_(store_status),
      seed_(seed),
      today_(std::move(today)) {}

SoloGameSetup::~SoloGameSetup() = default;

std::expected<std::unique_ptr<SoloGameSetup>, std::string> SoloGameSetup::open(
    const SoloGameOptions& options) {
    // A find is stamped with the day it was made, and a store given a wrong day
    // keeps it: no calendar is an error here and not a day made up.
    if (!options.today) {
        return std::unexpected("No calendar was given to take the day of a find from.");
    }
    auto dictionary = wjs::Dictionary::load_file(options.dictionary);
    if (!dictionary) {
        return std::unexpected("The word list " + text_of(options.dictionary) +
                               " could not be read (" + std::string(to_string(dictionary.error())) +
                               "). Start the program with --dictionary <file>.");
    }

    auto phases = wjs::PhaseSet::load_directory(options.phase_directory, *dictionary);
    if (!phases) {
        std::string message =
            "The phase files in " + text_of(options.phase_directory) + " could not be used:";
        for (const wjs::DataProblem& problem : phases.error()) {
            message += "\n  " + wjs::to_string(problem);
        }
        return std::unexpected(std::move(message));
    }

    wjs::Store store = wjs::Store::in_memory();
    wjs::LoadStatus store_status = wjs::LoadStatus::created;
    if (!options.store.empty()) {
        wjs::OpenResult opened = wjs::Store::open(options.store);
        store = std::move(opened.store);
        store_status = opened.status;
    }

    const std::string name = options.phase.empty() ? store.settings().phase : options.phase;
    const wjs::PhaseIndex* index = phases->find(name);
    if (index == nullptr) {
        std::string message =
            "There is no phase " + name + " in " + text_of(options.phase_directory) + "; it has:";
        for (const wjs::PhaseIndex& known : phases->phases()) {
            message += " " + known.phase();
        }
        return std::unexpected(std::move(message));
    }

    // The session is made from what the settings say each time (make()), but a
    // phase that cannot make a board fails the same way every time, and here it
    // can be said.
    const auto trial = wjs::GameSession::create(*index, *dictionary);
    if (!trial) {
        return std::unexpected("Phase " + name +
                               " cannot be played: " + std::string(to_string(trial.error())) + ".");
    }

    return std::unique_ptr<SoloGameSetup>(
        new SoloGameSetup(*std::move(dictionary), *std::move(phases), name, std::move(store),
                          store_status, options.seed, options.today));
}

std::unique_ptr<SoloGame> SoloGameSetup::make() {
    const wjs::Settings& settings = store_.settings();
    wjs::SessionOptions options;
    options.size = size_of(settings.board_size);
    options.seed = seed_ ? *seed_ : std::random_device{}();
    options.help = settings.help;
    options.focus_sounds = settings.focus_sounds;
    // open() made a session from this phase and word list, and nothing in the
    // options above can make one fail.
    return std::make_unique<RealSoloGame>(
        RealSoloGame::create(*phase_, dictionary_, store_, options, today_).value());
}

}  // namespace wjs::app
