#pragma once

// The performance tier's metrics: each one a name, what it measures now and
// the budget it is held to. The tests assert every metric of a group and the
// hidden report prints the same lists, so what a failure says and what the
// report shows cannot differ.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wjs::perf {

// A seed that is not one corpus board: a total, or something that has no board
// behind it.
inline constexpr unsigned kNoSeed = ~0U;

enum class Bound {
    // measured <= budget: a ceiling a quarter above the measurement.
    at_most,
    // measured == budget: what pins the corpus and the data together.
    exactly,
};

// The board mode a budget is for (docs/design/performance.md, "What was
// measured"). The two modes do different work and a figure for one says nothing
// about the other, so every metric names its own.
enum class BoardMode {
    // The default: dice, 3x3 to 6x6, solved against the full word list.
    standard,
    // A parent's switch: a seeded board of a phase, grapheme-bound, solved
    // against the phase's index.
    phonics,
};

[[nodiscard]] constexpr std::string_view to_string(BoardMode mode) noexcept {
    return mode == BoardMode::standard ? "standard" : "phonics";
}

struct Metric {
    // "<group>.<thing>.<statistic>", e.g. "solve.visits.max.4x4".
    std::string name;
    double measured = 0.0;
    double budget = 0.0;
    Bound bound = Bound::at_most;
    // The board mode the work is part of. Every metric states one; the
    // dictionary and the solve are the standard board's.
    BoardMode mode = BoardMode::standard;
    // The corpus seed that gave `measured` where it is a maximum, so a failure
    // can be reproduced with the report.
    unsigned seed = kNoSeed;
    // What the numbers are counted in, for the line that prints them.
    std::string unit{};
    // Anything else worth reading beside a failure: the raw times behind a
    // ratio, say.
    std::string note{};

    [[nodiscard]] bool holds() const noexcept {
        return bound == Bound::exactly ? measured == budget : measured <= budget;
    }
};

// --- work: counted, exact on every machine -----------------------------------

// The word list: words and trie nodes, exactly.
[[nodiscard]] std::vector<Metric> dictionary_metrics();

// The most visits of the random boards of each size.
[[nodiscard]] std::vector<Metric> random_board_metrics();

// The visits and the words of each board built to hold many words, and the
// visits of each board with none.
[[nodiscard]] std::vector<Metric> special_board_metrics();

// The digest of every solution over the whole corpus: random boards, boards
// built to hold many words and boards with none (measure.hpp, Digest).
[[nodiscard]] std::uint64_t corpus_digest();

// STANDARD BOARD. The most visits of a solve of a rolled board of each size,
// over kBoardsPerSize rolls of the project's dice (a roll followed by a solve
// against the full word list: what making a standard board costs).
[[nodiscard]] std::vector<Metric> dice_board_metrics();

// The digest of every solution over the rolled boards (all four sizes).
[[nodiscard]] std::uint64_t dice_digest();

// PHONICS BOARD. What seeding did for kSeedsPerSize seeds of each size on the
// test phase, with the options of phonics_board_options: verifications and
// solves spent, the solver's visits, the boards that fell back, and the cap on
// tries.
[[nodiscard]] std::vector<Metric> seeding_metrics();

// The digest of every board seeded over that corpus: tiles, targets, the words
// found, tries and solves.
[[nodiscard]] std::uint64_t seeding_digest();

// PHONICS BOARD. The binder's comparisons for each path of nine tiles
// (corpus.hpp, kBindPaths).
[[nodiscard]] std::vector<Metric> binder_metrics();

// PHONICS BOARD. What the game session spent over kSeedsPerSize sessions of each
// size it plays (3x3 and 4x4, kSessionSizes), each dealt from its corpus seed
// and played through to the resting screen: seedings, verifications and solves
// to deal, the solver's visits to deal, what a board that was not dealt in range
// came to, and that the play and the save make no solve at all and a restore
// makes one.
[[nodiscard]] std::vector<Metric> session_metrics();

// The digest of every board a session dealt and played: the deal, the kind of
// every trace, and the snapshots after the first find and at rest.
[[nodiscard]] std::uint64_t session_digest();

// --- the scanner, counted (wjs/scan/work.hpp) ---------------------------------

// What reading four fixed synthetic frames costs the reader (a 4x4, a 5x5, a 6x6
// and a 4x4 under glare): the frames and pixels, which pin the corpus, and the
// blobs, dice candidates, cells read, template comparisons, warped pixels and pixel
// buffers, which are what a read does. Totals over the four, and the most buffers
// any one read allocated.
[[nodiscard]] std::vector<Metric> scan_metrics();

// --- time: a ratio to the calibration workload, opt-in -----------------------

[[nodiscard]] std::vector<Metric> load_timing_metrics();
[[nodiscard]] std::vector<Metric> solve_timing_metrics();
// One read of a 4x4 and of a 6x6 frame.
[[nodiscard]] std::vector<Metric> scan_timing_metrics();

// PHONICS BOARD. Dealing a session's board (the shake: seeding, up to five
// times, until a board lands in range) of each size in kSessionSizes, against
// the light calibration. One deal's ratio.
[[nodiscard]] std::vector<Metric> session_timing_metrics();

}  // namespace wjs::perf
