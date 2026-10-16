#pragma once

// The performance tier's metrics: each one a name, what it measures now and
// the budget it is held to. The tests assert every metric of a group and the
// hidden report prints the same lists, so what a failure says and what the
// report shows cannot differ.

#include <cstdint>
#include <string>
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

struct Metric {
    // "<group>.<thing>.<statistic>", e.g. "solve.visits.max.4x4".
    std::string name;
    double measured = 0.0;
    double budget = 0.0;
    Bound bound = Bound::at_most;
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

// --- time: a ratio to the calibration workload, opt-in -----------------------

[[nodiscard]] std::vector<Metric> load_timing_metrics();
[[nodiscard]] std::vector<Metric> solve_timing_metrics();

}  // namespace wjs::perf
