#include "evaluate.hpp"

#include <algorithm>

#include "wjs/scan/reader.hpp"

namespace wjs::scan::synth {

namespace {

// The truth grid turned `quarters` quarter turns clockwise.
std::vector<std::string> turned(const std::vector<std::string>& grid, std::size_t size,
                                std::size_t quarters) {
    std::vector<std::string> out = grid;
    for (std::size_t q = 0; q < quarters; ++q) {
        std::vector<std::string> next(out.size());
        for (std::size_t i = 0; i < size; ++i) {
            for (std::size_t j = 0; j < size; ++j) {
                next[i * size + j] = out[(size - 1 - j) * size + i];
            }
        }
        out = std::move(next);
    }
    return out;
}

}  // namespace

Tally judge(const std::vector<std::string>& truth, std::size_t size,
            const std::expected<BoardReading, ReadError>& reading,
            std::vector<TileRecord>* records) {
    Tally tally;
    tally.frames = 1;
    if (!reading || reading->rows != size || reading->columns != size) {
        return tally;
    }
    tally.found = 1;

    std::size_t best_quarters = 0;
    std::size_t best_matches = 0;
    for (std::size_t q = 0; q < 4; ++q) {
        const std::vector<std::string> grid = turned(truth, size, q);
        std::size_t matches = 0;
        for (std::size_t i = 0; i < grid.size(); ++i) {
            if (reading->tiles[i].letters == grid[i]) {
                ++matches;
            }
        }
        if (matches > best_matches) {
            best_matches = matches;
            best_quarters = q;
        }
    }
    const std::vector<std::string> grid = turned(truth, size, best_quarters);

    for (std::size_t i = 0; i < grid.size(); ++i) {
        const TileReading& tile = reading->tiles[i];
        const bool correct = tile.letters == grid[i];
        const bool offered = correct || tile.alternatives[0].letters == grid[i] ||
                             tile.alternatives[1].letters == grid[i];
        ++tally.tiles;
        tally.correct += correct ? 1U : 0U;
        tally.flagged += tile.doubtful ? 1U : 0U;
        tally.silent += (!correct && !tile.doubtful) ? 1U : 0U;
        tally.offered += offered ? 1U : 0U;
        if (records != nullptr) {
            records->push_back({correct, offered, !tile.letters.empty(), tile.score, tile.margin,
                                grid[i], tile.letters});
        }
    }
    return tally;
}

Tally evaluate(Condition condition, Split split, const ReadOptions& options,
               std::vector<TileRecord>* records) {
    Tally total;
    for (const Spec& spec : corpus(split)) {
        if (spec.condition != condition) {
            continue;
        }
        const Rendered frame = render(spec);
        const auto reading = read_board(frame.image.view(), options);
        total.add(judge(frame.letters, frame.size, reading, records));
    }
    return total;
}

Tally with_thresholds(const std::vector<TileRecord>& records, double min_score, double min_margin) {
    Tally tally;
    for (const TileRecord& record : records) {
        const bool doubtful =
            !record.has_letters || record.score < min_score || record.margin < min_margin;
        ++tally.tiles;
        tally.correct += record.correct ? 1U : 0U;
        tally.flagged += doubtful ? 1U : 0U;
        tally.silent += (!record.correct && !doubtful) ? 1U : 0U;
        tally.offered += record.offered ? 1U : 0U;
    }
    return tally;
}

}  // namespace wjs::scan::synth
