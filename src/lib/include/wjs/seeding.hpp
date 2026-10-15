#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "wjs/board.hpp"
#include "wjs/phase_data.hpp"
#include "wjs/solver.hpp"

// Seeding: making the board of Mode B. A board is not a shuffle; it is made so
// that it holds a handful of words of the active phase and is quickly cleared.
// Two or three words of the phase are planted as paths of touching tiles, the
// other tiles are filled, and the board is solved against the phase's words to
// see how many a child can find. A board with three to five is kept; any
// other is changed a tile at a time, or made again, up to a limit, and the
// nearest is returned if none lands in range.
//
// The function is pure: its result depends on the phase data, the options, the
// recent targets and the seed, and on nothing else. Every draw is
// std::mt19937 taken modulo and every shuffle is the library's own, never
// std::shuffle or std::uniform_int_distribution (whose algorithms are each
// standard library's own), so a seed is the same board on every compiler.
//
// The library knows no letter or word of a phase. Dead jewels, tiles that
// carry no letter, are a letter that no word of the phase uses, chosen by the
// caller (it can take one from outside PhaseIndex::alphabet()) and drawn as a
// jewel; the Board stays A-Z.

namespace wjs {

// The targets of the last few boards, so that a board is not made of the words
// the child has just had. The caller keeps it between boards and records each
// board it plays; the seeding only reads it.
class RecentTargets {
public:
    static constexpr std::size_t default_boards = 10;

    RecentTargets() = default;
    explicit RecentTargets(std::size_t boards) : capacity_(boards) {}

    // The targets of one board. The oldest board is forgotten past the
    // capacity.
    void record(const std::vector<std::string>& targets);

    // Whether a word was a target on one of the remembered boards.
    [[nodiscard]] bool contains(std::string_view word) const;

    [[nodiscard]] std::size_t boards() const noexcept { return boards_.size(); }

private:
    std::size_t capacity_ = default_boards;
    std::deque<std::vector<std::string>> boards_;
};

enum class SeedError {
    bad_size,         // the board is not 1 to 16 tiles each way
    bad_range,        // min_viable above max_viable, or no tries
    empty_index,      // no word of the phase fits the board
    bad_dead_letter,  // not A-Z, or a letter the phase uses
    bad_filler,       // not A-Z, empty with no alphabet, or holds the dead letter
};

[[nodiscard]] std::string_view to_string(SeedError error) noexcept;

struct SeedOptions {
    // The board is size by size.
    std::size_t size = 4;

    // How many tiles are dead jewels, all marked with dead_letter. The words
    // are planted so as to leave room for them, and the tiles that are neither
    // planted nor dead are filled from `filler`. No more are made than leave
    // room for the phase's shortest word that fits the board.
    std::size_t dead_cells = 0;
    char dead_letter = 'X';

    // Letters to fill the other tiles from, A-Z; a letter repeated is likelier.
    // Empty means the phase's own alphabet. The alphabet is the letters of the
    // phase, so a board of it shows only what the phase has taught; letters not
    // yet taught are for the caller to pass.
    std::string filler;

    // Sounds, as written in the graphemes file, the parent has ticked. Targets
    // prefer words with a grapheme that makes one.
    std::vector<std::string> focus_sounds;

    // What a board has to hold, counted in words of the phase on it.
    std::size_t min_viable = 3;
    std::size_t max_viable = 5;

    // Boards verified before the nearest is taken. Each costs one solve.
    std::size_t max_tries = 20;

    // Filler tiles changed one at a time, each costing a solve, on a planting
    // before the words are chosen and planted again. Measured no better than
    // planting again (docs/testing.md, "Seeding"), so the default is none.
    std::size_t mutations_per_plant = 0;
};

struct SeededBoard {
    Board board;

    // The words planted, as written in the phase data (lower-case).
    std::vector<std::string> targets;

    // Every word of the phase on the board, as the solver reports them.
    std::vector<FoundWord> viable;

    // The dead jewels, in row order.
    std::vector<Position> dead;

    // Boards verified: 1 to max_tries.
    std::size_t tries = 0;

    // No board landed in the range, and this is the nearest. A test counts
    // these.
    bool fallback = false;

    // Focus sounds were asked for and fewer than every target carries one:
    // "few words use this sound yet".
    bool focus_unmet = false;
};

[[nodiscard]] std::expected<SeededBoard, SeedError> seed_board(const PhaseIndex& index,
                                                               const SeedOptions& options,
                                                               const RecentTargets& recent,
                                                               std::uint32_t seed);

}  // namespace wjs
