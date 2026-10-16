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
// program's whole run: the English word list, the phases of a data directory, the
// store, and the day. The game (RealSoloGame) holds the phase and the store by
// reference, so this outlives every game it makes. Qt-free; the program supplies
// the paths and the calendar.
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
    // The store's file; empty for a store that is never written.
    std::filesystem::path store;
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
    [[nodiscard]] static std::expected<std::unique_ptr<SoloGameSetup>, std::string> open(
        const SoloGameOptions& options);

    SoloGameSetup(const SoloGameSetup&) = delete;
    SoloGameSetup& operator=(const SoloGameSetup&) = delete;
    ~SoloGameSetup();

    // A new game in the box, over the phase and the store, with the parent's
    // settings as they are now (the size to offer first, the help, the sounds in
    // focus). The next run is the next to read a change made in the meantime.
    [[nodiscard]] std::unique_ptr<SoloGame> make();

    // The one store of the program, which the parent area is to share.
    [[nodiscard]] wjs::Store& store() noexcept { return store_; }
    [[nodiscard]] wjs::LoadStatus store_status() const noexcept { return store_status_; }

    [[nodiscard]] const wjs::PhaseIndex& phase() const noexcept { return *phase_; }
    // Every phase file carried a complete review; otherwise the files are test data.
    [[nodiscard]] bool reviewed() const noexcept { return phases_.reviewed(); }
    [[nodiscard]] const std::vector<std::string>& unreviewed_files() const noexcept {
        return phases_.unreviewed_files();
    }

private:
    SoloGameSetup(wjs::Dictionary dictionary, wjs::PhaseSet phases, std::string phase,
                  wjs::Store store, wjs::LoadStatus store_status, std::optional<std::uint32_t> seed,
                  std::function<wjs::Day()> today);

    wjs::Dictionary dictionary_;
    wjs::PhaseSet phases_;
    const wjs::PhaseIndex* phase_;
    wjs::Store store_;
    wjs::LoadStatus store_status_;
    std::optional<std::uint32_t> seed_;
    std::function<wjs::Day()> today_;
};

}  // namespace wjs::app
