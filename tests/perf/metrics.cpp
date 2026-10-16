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
#include "scene.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/scan/reader.hpp"
#include "wjs/scan/work.hpp"

namespace wjs::perf {
namespace {

[[nodiscard]] std::string size_label(std::size_t size) {
    return std::format("{}x{}", size, size);
}

[[nodiscard]] Metric exact(std::string name, std::uint64_t measured, std::uint64_t budget,
                           std::string unit) {
    Metric metric;
    metric.name = std::move(name);
    metric.measured = static_cast<double>(measured);
    metric.budget = static_cast<double>(budget);
    metric.bound = Bound::exactly;
    metric.unit = std::move(unit);
    return metric;
}

[[nodiscard]] Metric at_most(std::string name, std::uint64_t measured, std::uint64_t budget,
                             std::string unit, unsigned seed = kNoSeed) {
    Metric metric;
    metric.name = std::move(name);
    metric.measured = static_cast<double>(measured);
    metric.budget = static_cast<double>(budget);
    metric.seed = seed;
    metric.unit = std::move(unit);
    return metric;
}

[[nodiscard]] Metric ratio(std::string name, const RelativeTime& time,
                           const budget::TimeBudget& allowed) {
    Metric metric;
    metric.name = std::move(name);
    metric.measured = time.ratio;
    metric.budget = allowed.ratio;
    metric.unit = "x calibration";
    metric.note =
        std::format("fastest {:.3f} ms against {:.3f} ms (medians {:.3f} / {:.3f})", time.task_ms,
                    time.calibration_ms, time.task_median_ms, time.calibration_median_ms);
    return metric;
}

// The frames the scanner's metrics read: fixed specs of the synthetic corpus
// (tests/scan/synth), drawn from the tuning split so that the test split is not
// read for anything but the target. A 4x4, a 5x5, a 6x6 and a 4x4 under glare.
struct ScanFrame {
    scan::synth::Condition condition;
    std::size_t size;
    unsigned index;
};
constexpr std::array<ScanFrame, 4> kScanFrames = {{
    {scan::synth::Condition::upright, 4, 2},
    {scan::synth::Condition::angled, 5, 14},
    {scan::synth::Condition::loose, 6, 20},
    {scan::synth::Condition::glare, 4, 3},
}};

[[nodiscard]] scan::synth::Rendered draw_scan_frame(const ScanFrame& which) {
    scan::synth::Spec spec;
    spec.condition = which.condition;
    spec.split = scan::synth::Split::tuning;
    spec.size = which.size;
    spec.index = which.index;
    return scan::synth::render(spec);
}

}  // namespace

std::vector<Metric> dictionary_metrics() {
    const wjs::Dictionary& dictionary = english();
    return {
        exact("dictionary.words", dictionary.size(), budget::kDictionaryWords, "words"),
        exact("dictionary.nodes", trie_nodes(dictionary), budget::kDictionaryNodes, "nodes"),
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
        metrics.push_back(at_most("solve.visits.max." + size_label(size), most,
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
        metrics.push_back(at_most("solve.visits.rich." + std::string(rich.name), work.visits,
                                  budget::kRichVisits[i], "visits"));
        metrics.push_back(exact("solve.words.rich." + std::string(rich.name), work.words,
                                budget::kRichWords[i], "words"));
    }
    for (std::size_t i = 0; i < kEmptyBoards.size(); ++i) {
        const RichBoard& empty = kEmptyBoards[i];
        const SolveWork work = measure_solve(rich_board(empty), dictionary);
        metrics.push_back(exact("solve.visits.empty." + std::string(empty.name), work.visits,
                                budget::kEmptyVisits[i], "visits"));
        metrics.push_back(
            exact("solve.words.empty." + std::string(empty.name), work.words, 0, "words"));
    }
    return metrics;
}

std::vector<Metric> scan_metrics() {
    // Draw the frames first, then count only the reads: drawing allocates buffers too.
    std::vector<scan::synth::Rendered> frames;
    for (const ScanFrame& which : kScanFrames) {
        frames.push_back(draw_scan_frame(which));
    }
    // The template bank is built on first use; build it outside the count.
    (void)scan::read_board(frames.front().image.view());

    scan::ScanWork total;
    std::uint64_t answered = 0;
    for (const scan::synth::Rendered& frame : frames) {
        scan::reset_scan_work();
        const auto reading = scan::read_board(frame.image.view());
        answered += reading.has_value() ? 1U : 0U;
        const scan::ScanWork work = scan::scan_work();
        total.reads += work.reads;
        total.frame_pixels += work.frame_pixels;
        total.working_pixels += work.working_pixels;
        total.warp_pixels += work.warp_pixels;
        total.blobs += work.blobs;
        total.dice_candidates += work.dice_candidates;
        total.cells_read += work.cells_read;
        total.comparisons += work.comparisons;
        total.buffers += work.buffers;
        total.largest_buffers = std::max(total.largest_buffers, work.largest_buffers);
    }
    return {
        exact("scan.reads", total.reads, budget::kScanReads, "reads"),
        exact("scan.answered", answered, budget::kScanReads, "boards"),
        exact("scan.frame_pixels", total.frame_pixels, budget::kScanFramePixels, "pixels"),
        exact("scan.working_pixels", total.working_pixels, budget::kScanWorkingPixels, "pixels"),
        at_most("scan.warp_pixels", total.warp_pixels, budget::kScanWarpPixels, "pixels"),
        at_most("scan.blobs", total.blobs, budget::kScanBlobs, "blobs"),
        at_most("scan.dice_candidates", total.dice_candidates, budget::kScanDiceCandidates,
                "blobs"),
        at_most("scan.cells_read", total.cells_read, budget::kScanCellsRead, "cells"),
        at_most("scan.comparisons", total.comparisons, budget::kScanComparisons, "comparisons"),
        at_most("scan.buffers", total.buffers, budget::kScanBuffers, "buffers"),
        at_most("scan.buffers.max", total.largest_buffers, budget::kScanBuffersMax, "buffers"),
    };
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

std::vector<Metric> load_timing_metrics() {
    const RelativeTime time = time_against_calibration(
        [] {
            const auto loaded = wjs::Dictionary::load_file(std::string(kDictionaryPath));
            return loaded ? loaded->size() : 0U;
        },
        budget::kLoadRuns, Weight::heavy);
    return {ratio("time.load_dictionary", time, budget::kLoadTime)};
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
        Metric metric = ratio("time.solve." + std::string(rich.name), time, one.allowed);
        metric.measured = time.ratio / static_cast<double>(budget::kSolvesPerRun);
        metric.note += std::format("; one solve {:.3f} ms",
                                   time.task_ms / static_cast<double>(budget::kSolvesPerRun));
        metrics.push_back(std::move(metric));
    }
    return metrics;
}

std::vector<Metric> scan_timing_metrics() {
    std::vector<Metric> metrics;
    const struct {
        std::size_t which;
        const budget::TimeBudget& allowed;
    } cases[] = {{0, budget::kScanRead4Time}, {2, budget::kScanRead6Time}};
    for (const auto& one : cases) {
        const ScanFrame& which = kScanFrames[one.which];
        const scan::synth::Rendered frame = draw_scan_frame(which);
        const RelativeTime time = time_against_calibration(
            [&] {
                const auto reading = scan::read_board(frame.image.view());
                return reading ? reading->tiles.size() : 0U;
            },
            budget::kScanRuns, Weight::heavy);
        metrics.push_back(ratio("time.read." + size_label(which.size), time, one.allowed));
    }
    return metrics;
}

}  // namespace wjs::perf
