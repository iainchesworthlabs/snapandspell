#include "wjs/scan/reader.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#include "scratch.hpp"
#include "wjs/profiling.hpp"
#include "wjs/scan/board_finder.hpp"
#include "wjs/scan/glyph_reader.hpp"
#include "wjs/scan/image.hpp"
#include "wjs/scan/stroke_font.hpp"
#include "wjs/scan/work.hpp"

namespace wjs::scan {

std::string_view to_string(ReadError error) noexcept {
    switch (error) {
        case ReadError::invalid_frame:
            return "the frame is empty or its buffer is too short";
        case ReadError::board_not_found:
            return "no board was found in the frame";
        case ReadError::size_not_found:
            return "a board was found but its cells could not be counted";
        case ReadError::no_letters:
            return "a board was found but no letter on it could be seen";
    }
    return "unknown read error";
}

namespace {

// The half-side of a cell's patch, as a share of a cell: 0.34 holds a glyph of the
// stroke font, which is 55% of a die, with room to turn.
constexpr double kPatchOfCell = 0.34;
constexpr double kStrokeFontGlyphRatio = 0.55;

TileReading to_tile(const GlyphRead& read, const ReadOptions& options) {
    TileReading tile;
    if (!read.has_ink || read.found == 0) {
        return tile;  // no letters, confidence 0, doubtful
    }
    const auto& font = stroke_font();
    const GlyphScore& first = read.best[0];
    const double second_score = read.found > 1 ? read.best[1].score : -1.0;
    const double margin = first.score - second_score;

    tile.letters = std::string(font[first.glyph].letters);
    tile.rotation_degrees = first.rotation_degrees;
    tile.score = first.score;
    tile.margin = margin;
    tile.doubtful = first.score < options.min_score || margin < options.min_margin;
    tile.confidence = std::clamp(
        0.5 * std::min(first.score / options.min_score, margin / options.min_margin), 0.0, 1.0);
    for (std::size_t k = 0; k < tile.alternatives.size(); ++k) {
        if (k + 1 < read.found) {
            tile.alternatives[k] = {std::string(font[read.best[k + 1].glyph].letters),
                                    read.best[k + 1].score};
        }
    }
    return tile;
}

}  // namespace

std::expected<BoardReading, ReadError> read_board(const Frame& frame, const ReadOptions& options) {
    if (!frame.valid()) {
        return std::unexpected(ReadError::invalid_frame);
    }

    ScanWork& tally = detail::scan_tally();
    const std::uint64_t buffers_before = tally.buffers;
    ++tally.reads;
    tally.frame_pixels += static_cast<std::uint64_t>(frame.width) * frame.height;

    // Every way out of here records how many buffers this read took.
    const auto finish = [&]() {
        tally.largest_buffers = std::max(tally.largest_buffers, tally.buffers - buffers_before);
    };

    // The picture chain, into this thread's scratch so that a warm read allocates
    // nothing (src/scan/src/scratch.hpp): the luma of the frame, the analysis
    // picture it is averaged down to, and the working picture the board is looked
    // for in. A picture that already fits is used as it is - a pointer, where it
    // was a whole copy before.
    detail::Scratch& scratch = detail::thread_scratch();

    int to_analysis = 1;
    int to_working = 1;
    {
        auto zone = profiling::zone<"scan.decode_frame">();
        detail::luma_into(scratch.luma, frame);
    }
    detail::shrink_into(scratch.analysis, scratch.luma, options.analysis_size, to_analysis);
    const GrayImage* analysis = to_analysis > 1 ? &scratch.analysis : &scratch.luma;
    {
        auto zone = profiling::zone<"scan.convert_working">();
        detail::shrink_into(scratch.working, *analysis, options.working_size, to_working);
    }
    const GrayImage* working = to_working > 1 ? &scratch.working : analysis;
    tally.working_pixels +=
        static_cast<std::uint64_t>(working->width) * static_cast<std::uint64_t>(working->height);

    const std::optional<BoardFind> found = detail::find_board_using(*working, 8, scratch);
    if (!found) {
        finish();
        return std::unexpected(ReadError::board_not_found);
    }

    // The outline in the analysis picture's pixels, which the board is drawn from.
    Quad outline{};
    for (std::size_t k = 0; k < 4; ++k) {
        outline[k] = found->corners[k] * static_cast<double>(to_working);
    }
    {
        auto zone = profiling::zone<"scan.warp_board">();
        detail::warp_into(scratch.board, *analysis, outline, options.board_side);
    }
    const GrayImage& board = scratch.board;

    // The board's column and row profiles, computed once and shared by the cell
    // count and the cell centres (each computed them for itself before).
    const BoardProfiles profiles = profiles_of(board);
    std::optional<CellCount> count;
    if (options.size) {
        count = CellCount{*options.size, 1.0};
    } else {
        count = count_cells(board, profiles, options.min_size, options.max_size, found->dice,
                            found->size_estimate);
    }
    if (!count) {
        finish();
        return std::unexpected(ReadError::size_not_found);
    }

    BoardReading reading;
    reading.rows = count->size;
    reading.columns = count->size;
    reading.size_confidence = count->confidence;
    reading.dice_found = found->dice;
    const double to_frame = static_cast<double>(to_working) * static_cast<double>(to_analysis);
    for (std::size_t k = 0; k < 4; ++k) {
        reading.corners[k] = found->corners[k] * to_frame;
    }

    {
        [[maybe_unused]] auto zone = profiling::zone<"scan.read_tiles">();
        const GlyphReader& reader = GlyphReader::standard();
        const double cell =
            static_cast<double>(options.board_side) / static_cast<double>(count->size);
        const double half_side =
            kPatchOfCell * cell * (options.glyph_height_ratio / kStrokeFontGlyphRatio);
        const CellCentres centres = locate_cells(board, profiles, count->size);

        // The glyph height, from a sample of the cells read at trial heights: the
        // height at which they match best. Coarse (a quarter either way), then fine.
        // The sample cells are read again when the board's tiles are read at the
        // winning height; the winning trial's reads are kept and used then, so
        // each sample cell is read once at that height, not twice.
        double scale = 1.0;
        std::vector<std::size_t> sample_cells;
        std::vector<GlyphRead> sample_reads;
        if (options.estimate_glyph_height) {
            const std::size_t cells = count->size * count->size;
            const std::size_t step = std::max<std::size_t>(1, cells / 8);
            for (std::size_t k = step / 2; k < cells; k += step) {
                sample_cells.push_back(k);
            }
            const auto fit_at = [&](double trial) {
                double total = 0.0;
                int seen = 0;
                std::vector<GlyphRead> reads;
                reads.reserve(sample_cells.size());
                for (const std::size_t k : sample_cells) {
                    const Point centre{centres.x[k % count->size], centres.y[k / count->size]};
                    const GlyphRead read =
                        reader.read(board, centre, half_side * trial, options.min_contrast);
                    reads.push_back(read);
                    if (read.has_ink) {
                        total += read.best[0].score;
                        ++seen;
                    }
                }
                return std::pair{seen == 0 ? -1.0 : total / seen, std::move(reads)};
            };
            double best_fit = -1e300;
            for (const double trial : {0.8, 1.0, 1.25}) {
                auto [fit, reads] = fit_at(trial);
                if (fit > best_fit) {
                    best_fit = fit;
                    scale = trial;
                    sample_reads = std::move(reads);
                }
            }
            const double middle = scale;
            for (const double trial : {middle / 1.1, middle * 1.1}) {
                auto [fit, reads] = fit_at(trial);
                if (fit > best_fit) {
                    best_fit = fit;
                    scale = trial;
                    sample_reads = std::move(reads);
                }
            }
        }
        reading.glyph_height_ratio = options.glyph_height_ratio * scale;
        reading.tiles.reserve(count->size * count->size);
        // The sample cells' reads at the winning height, for the tile loop to
        // take instead of reading those cells again.
        std::vector<std::optional<GlyphRead>> cached(count->size * count->size);
        for (std::size_t i = 0; i < sample_cells.size(); ++i) {
            cached[sample_cells[i]] = sample_reads[i];
        }
        std::size_t blank = 0;
        {
            auto glyph_zone = profiling::zone<"scan.read_glyphs">();
            for (std::size_t row = 0; row < count->size; ++row) {
                for (std::size_t column = 0; column < count->size; ++column) {
                    const std::size_t index = row * count->size + column;
                    const Point centre{centres.x[column], centres.y[row]};
                    const GlyphRead read =
                        cached[index]
                            ? *cached[index]
                            : reader.read(board, centre, half_side * scale, options.min_contrast);
                    reading.tiles.push_back(to_tile(read, options));
                    if (reading.tiles.back().letters.empty()) {
                        ++blank;
                    }
                }
            }
        }
        // A "board" in which most cells are empty is a pattern that is not one.
        if (blank * 2 > reading.tiles.size()) {
            finish();
            return std::unexpected(ReadError::no_letters);
        }
    }

    finish();
    return reading;
}

}  // namespace wjs::scan
