#include "metrics.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "budgets.hpp"
#include "corpus.hpp"
#include "measure.hpp"
#include "perf_build.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/seeding.hpp"

namespace wjs::perf {
namespace {

[[nodiscard]] std::string size_label(std::size_t size) {
    return std::format("{}x{}", size, size);
}

// Every metric states the board mode it is for, with no default to fall back
// on: a figure for one mode says nothing about the other.
[[nodiscard]] Metric exact(std::string name, BoardMode mode, std::uint64_t measured,
                           std::uint64_t budget, std::string unit) {
    Metric metric;
    metric.name = std::move(name);
    metric.mode = mode;
    metric.measured = static_cast<double>(measured);
    metric.budget = static_cast<double>(budget);
    metric.bound = Bound::exactly;
    metric.unit = std::move(unit);
    return metric;
}

[[nodiscard]] Metric at_most(std::string name, BoardMode mode, std::uint64_t measured,
                             std::uint64_t budget, std::string unit, unsigned seed = kNoSeed) {
    Metric metric;
    metric.name = std::move(name);
    metric.mode = mode;
    metric.measured = static_cast<double>(measured);
    metric.budget = static_cast<double>(budget);
    metric.seed = seed;
    metric.unit = std::move(unit);
    return metric;
}

[[nodiscard]] Metric ratio(std::string name, BoardMode mode, const RelativeTime& time,
                           const budget::TimeBudget& allowed) {
    Metric metric;
    metric.name = std::move(name);
    metric.mode = mode;
    metric.measured = time.ratio;
    metric.budget = allowed.ratio;
    metric.unit = "x calibration";
    metric.note =
        std::format("fastest {:.3f} ms against {:.3f} ms (medians {:.3f} / {:.3f})", time.task_ms,
                    time.calibration_ms, time.task_median_ms, time.calibration_median_ms);
    return metric;
}

}  // namespace

std::vector<Metric> dictionary_metrics() {
    const wjs::Dictionary& dictionary = english();
    return {
        exact("dictionary.words", BoardMode::standard, dictionary.size(), budget::kDictionaryWords,
              "words"),
        exact("dictionary.nodes", BoardMode::standard, trie_nodes(dictionary),
              budget::kDictionaryNodes, "nodes"),
    };
}

std::vector<Metric> random_board_metrics() {
    const wjs::Dictionary& dictionary = english();
    std::vector<Metric> metrics;
    for (std::size_t which = 0; which < kSizes.size(); ++which) {
        const std::size_t size = kSizes[which];
        std::uint64_t most = 0;
        unsigned seed = kNoSeed;
        for (unsigned index = 0; index < kBoardsPerSize; ++index) {
            const SolveWork work = measure_solve(random_board(size, index), dictionary);
            if (work.visits > most) {
                most = work.visits;
                seed = corpus_seed(size, index);
            }
        }
        metrics.push_back(at_most("solve.visits.max." + size_label(size), BoardMode::standard, most,
                                  budget::kMaxVisitsPerSize[which], "visits", seed));
    }
    return metrics;
}

std::vector<Metric> special_board_metrics() {
    const wjs::Dictionary& dictionary = english();
    std::vector<Metric> metrics;
    for (std::size_t i = 0; i < kRichBoards.size(); ++i) {
        const RichBoard& rich = kRichBoards[i];
        const SolveWork work = measure_solve(rich_board(rich), dictionary);
        metrics.push_back(at_most("solve.visits.rich." + std::string(rich.name),
                                  BoardMode::standard, work.visits, budget::kRichVisits[i],
                                  "visits"));
        metrics.push_back(exact("solve.words.rich." + std::string(rich.name), BoardMode::standard,
                                work.words, budget::kRichWords[i], "words"));
    }
    for (std::size_t i = 0; i < kEmptyBoards.size(); ++i) {
        const RichBoard& empty = kEmptyBoards[i];
        const SolveWork work = measure_solve(rich_board(empty), dictionary);
        metrics.push_back(exact("solve.visits.empty." + std::string(empty.name),
                                BoardMode::standard, work.visits, budget::kEmptyVisits[i],
                                "visits"));
        metrics.push_back(exact("solve.words.empty." + std::string(empty.name), BoardMode::standard,
                                work.words, 0, "words"));
    }
    return metrics;
}

std::uint64_t corpus_digest() {
    const wjs::Dictionary& dictionary = english();
    Digest digest;
    for (const std::size_t size : kSizes) {
        for (unsigned index = 0; index < kBoardsPerSize; ++index) {
            digest.add(wjs::solve(random_board(size, index), dictionary));
        }
    }
    for (const RichBoard& rich : kRichBoards) {
        digest.add(wjs::solve(rich_board(rich), dictionary));
    }
    for (const RichBoard& empty : kEmptyBoards) {
        digest.add(wjs::solve(rich_board(empty), dictionary));
    }
    return digest.value();
}

std::vector<Metric> dice_board_metrics() {
    const wjs::Dictionary& dictionary = english();
    std::vector<Metric> metrics;
    for (std::size_t which = 0; which < kSizes.size(); ++which) {
        const std::size_t size = kSizes[which];
        std::uint64_t most = 0;
        unsigned seed = kNoSeed;
        for (unsigned index = 0; index < kBoardsPerSize; ++index) {
            const SolveWork work = measure_solve(dice_board(size, index), dictionary);
            if (work.visits > most) {
                most = work.visits;
                seed = corpus_seed(size, index);
            }
        }
        metrics.push_back(at_most("roll.visits.max." + size_label(size), BoardMode::standard, most,
                                  budget::kDiceMaxVisitsPerSize[which], "visits", seed));
    }
    return metrics;
}

std::uint64_t dice_digest() {
    const wjs::Dictionary& dictionary = english();
    Digest digest;
    for (const std::size_t size : kSizes) {
        for (unsigned index = 0; index < kBoardsPerSize; ++index) {
            digest.add(wjs::solve(dice_board(size, index), dictionary));
        }
    }
    return digest.value();
}

namespace {

// What seeding did over the kSeedsPerSize seeds of one size.
struct SeedTotals {
    std::uint64_t tries = 0;
    std::uint64_t solves = 0;
    std::uint64_t visits = 0;
    std::uint64_t most_tries = 0;
    std::uint64_t most_visits = 0;
    unsigned most_visits_seed = kNoSeed;
    // Boards that landed outside the range they were asked for without being
    // called a fallback, which the seeding promises it never does.
    std::uint64_t out_of_range = 0;
    std::uint64_t fallbacks = 0;
};

struct SeedCorpus {
    std::array<SeedTotals, kSizes.size()> sizes{};
    std::uint64_t digest = 0;
};

// The whole phonics corpus, made once: the metrics and the digest are two
// readings of the same seedings, and a seeding is the slow part of this tier.
[[nodiscard]] const SeedCorpus& seed_corpus() {
    static const SeedCorpus corpus = [] {
        const wjs::PhaseIndex& phase = phonics_phase();
        const wjs::SeedOptions defaults;
        SeedCorpus made;
        Digest digest;
        for (std::size_t which = 0; which < kSizes.size(); ++which) {
            const std::size_t size = kSizes[which];
            SeedTotals& totals = made.sizes[which];
            for (unsigned index = 0; index < kSeedsPerSize; ++index) {
                const unsigned seed = corpus_seed(size, index);
                const SeedRun run = measure_seed(phase, size, seed);
                const wjs::SeededBoard& board = run.board;

                totals.tries += board.tries;
                totals.solves += board.solves;
                totals.visits += run.visits;
                totals.most_tries = std::max<std::uint64_t>(totals.most_tries, board.tries);
                if (run.visits > totals.most_visits) {
                    totals.most_visits = run.visits;
                    totals.most_visits_seed = seed;
                }
                if (board.fallback) {
                    ++totals.fallbacks;
                } else if (board.viable.size() < defaults.min_viable ||
                           board.viable.size() > defaults.max_viable) {
                    ++totals.out_of_range;
                }
                digest.add(board);
            }
        }
        made.digest = digest.value();
        return made;
    }();
    return corpus;
}

}  // namespace

std::vector<Metric> seeding_metrics() {
    constexpr BoardMode mode = BoardMode::phonics;
    const wjs::PhaseIndex& phase = phonics_phase();
    const SeedCorpus& corpus = seed_corpus();

    std::vector<Metric> metrics;
    // The corpus is the words of the proxy phase: if the word list moved, this
    // is what says so before any seeding figure does.
    metrics.push_back(
        exact("seed.phase.words", mode, phase.words().size(), budget::kPhonicsWords, "words"));
    for (std::size_t which = 0; which < kSizes.size(); ++which) {
        const std::string label = size_label(kSizes[which]);
        const SeedTotals& totals = corpus.sizes[which];
        const budget::SeedBudget& allowed = budget::kSeedBudgets[which];

        metrics.push_back(at_most("seed.tries.total." + label, mode, totals.tries,
                                  allowed.tries_total, "verifications"));
        metrics.push_back(at_most("seed.solves.total." + label, mode, totals.solves,
                                  allowed.solves_total, "solves"));
        metrics.push_back(at_most("seed.visits.total." + label, mode, totals.visits,
                                  allowed.visits_total, "visits"));
        metrics.push_back(at_most("seed.visits.max." + label, mode, totals.most_visits,
                                  allowed.visits_max, "visits", totals.most_visits_seed));
        // The algorithm's own cap on verifications (SeedOptions::max_tries), not
        // a quarter above anything: no seeding may spend more.
        metrics.push_back(at_most("seed.tries.max." + label, mode, totals.most_tries,
                                  budget::kSeedMaxTries, "verifications"));
        metrics.push_back(at_most("seed.fallbacks." + label, mode, totals.fallbacks,
                                  allowed.fallbacks, "boards"));
        metrics.push_back(
            exact("seed.out_of_range." + label, mode, totals.out_of_range, 0, "boards"));
    }
    return metrics;
}

std::uint64_t seeding_digest() {
    return seed_corpus().digest;
}

std::vector<Metric> binder_metrics() {
    std::vector<Metric> metrics;
    for (std::size_t i = 0; i < kBindPaths.size(); ++i) {
        const BindPath& path = kBindPaths[i];
        const std::vector<std::string> tiles = bind_tiles(path);
        const BindRun run = measure_bind(phonics_binder(), tiles);
        metrics.push_back(at_most(
            "bind.comparisons." + std::string(path.name) + "." + std::to_string(tiles.size()),
            BoardMode::phonics, run.comparisons, budget::kBindComparisons[i], "comparisons"));
    }
    return metrics;
}

std::vector<Metric> load_timing_metrics() {
    const RelativeTime time = time_against_calibration(
        [] {
            const auto loaded = wjs::Dictionary::load_file(std::string(kDictionaryPath));
            return loaded ? loaded->size() : 0U;
        },
        budget::kLoadRuns, Weight::heavy);
    return {ratio("time.load_dictionary", BoardMode::standard, time, budget::kLoadTime)};
}

std::vector<Metric> solve_timing_metrics() {
    const wjs::Dictionary& dictionary = english();
    std::vector<Metric> metrics;
    const struct {
        std::size_t rich;
        const budget::TimeBudget& allowed;
    } cases[] = {{0, budget::kSolveRich4Time}, {2, budget::kSolveRich6Time}};
    for (const auto& one : cases) {
        const RichBoard& rich = kRichBoards[one.rich];
        const wjs::Board board = rich_board(rich);
        const RelativeTime time = time_against_calibration(
            [&] {
                std::size_t words = 0;
                for (int i = 0; i < budget::kSolvesPerRun; ++i) {
                    words += wjs::solve(board, dictionary).size();
                }
                return words;
            },
            budget::kSolveRuns, Weight::light);
        // The task is kSolvesPerRun solves, so the ratio is for the batch; the
        // metric is one solve's, and the note says what that was.
        Metric metric =
            ratio("time.solve." + std::string(rich.name), BoardMode::standard, time, one.allowed);
        metric.measured = time.ratio / static_cast<double>(budget::kSolvesPerRun);
        metric.note += std::format("; one solve {:.3f} ms",
                                   time.task_ms / static_cast<double>(budget::kSolvesPerRun));
        metrics.push_back(std::move(metric));
    }
    return metrics;
}

}  // namespace wjs::perf
