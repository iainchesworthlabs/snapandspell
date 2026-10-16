#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "solo_game.hpp"
#include "wjs/calendar.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/phase_data.hpp"
#include "wjs/store.hpp"

// What the real game needs to exist, loaded and owned in one place for the
// program's whole run: the English word list, the phases of a data directory, and
// the day. The store is the program's one (main.cpp opens it and the parent area
// shares it), held here by reference. The game (RealSoloGame) holds the phase and
// the store by reference too, so this outlives every game it makes. Qt-free; the
// program supplies the paths and the calendar.
//
// The repository has no phase to load: the files are written and reviewed by a
// person qualified to do it (docs/design/phonics.md), and none may be published
// until they are. So the real game is played only when the program is told where a
// phase is (`--phase-directory`, see main.cpp), and a phase given that way is
// reported as test data unless every file in it carries a complete review header.

namespace wjs::app {

struct SoloGameOptions {
    // The directory of phase-<x>.graphemes.txt and phase-<x>.words.txt files.
    std::filesystem::path phase_directory;
    // The phase to play ("A"); empty for the one in the store's settings.
    std::string phase;
    // The English word list the phase is checked against and the session tells a
    // real word from a string of letters with.
    std::filesystem::path dictionary;
    // The seed of the run's boards; a random one when not given. A fixed seed
    // makes a run repeat, which a test and a screenshot want.
    std::optional<std::uint32_t> seed;
    // The device's calendar day. Required: open() refuses without it.
    std::function<wjs::Day()> today;
};

class SoloGameSetup {
public:
    // Loads everything the game needs, or says in a sentence why it cannot: the
    // dictionary or the phase files cannot be read (every problem of the files is
    // listed), the phase asked for is not in them, or it cannot make a board.
    // `store` is the program's own, which must outlive the setup.
    [[nodiscard]] static std::expected<std::unique_ptr<SoloGameSetup>, std::string> open(
        const SoloGameOptions& options, wjs::Store& store);

    SoloGameSetup(const SoloGameSetup&) = delete;
    SoloGameSetup& operator=(const SoloGameSetup&) = delete;
    ~SoloGameSetup();

    // The game to play, over the phase and the store, with the parent's settings as
    // they are now (the size to offer first, the help, the sounds in focus). A change
    // made in the meantime is read by the next game made: the program makes one when
    // the parent has changed a setting (SoloModel::storeEdited), and at its next start.
    //
    // If the store holds a game in progress of the phase being played (a board on the
    // table, or the resting screen), that game, as it was left. Otherwise a new game
    // in the box: the store holds none, or the one it holds cannot come back
    // (GameSession::restore refuses it: another phase, a board the phase's words no
    // longer fit, a record no session makes), and is then cleared. Nothing is said of
    // either to the child.
    [[nodiscard]] std::unique_ptr<SoloGame> make();

    [[nodiscard]] const wjs::PhaseIndex& phase() const noexcept { return *phase_; }
    // Every phase of the directory, which the parent area chooses among.
    [[nodiscard]] const wjs::PhaseSet& phases() const noexcept { return phases_; }
    // Every phase file carried a complete review; otherwise the files are test data.
    [[nodiscard]] bool reviewed() const noexcept { return phases_.reviewed(); }
    [[nodiscard]] const std::vector<std::string>& unreviewed_files() const noexcept {
        return phases_.unreviewed_files();
    }

private:
    SoloGameSetup(wjs::Dictionary dictionary, wjs::PhaseSet phases, std::string phase,
                  wjs::Store& store, std::optional<std::uint32_t> seed,
                  std::function<wjs::Day()> today);

    wjs::Dictionary dictionary_;
    wjs::PhaseSet phases_;
    const wjs::PhaseIndex* phase_;
    wjs::Store& store_;
    std::optional<std::uint32_t> seed_;
    std::function<wjs::Day()> today_;
};

}  // namespace wjs::app
