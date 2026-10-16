#include "wjs/grapheme_binder.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace wjs {
namespace {

[[nodiscard]] constexpr char to_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

[[nodiscard]] std::string lower(std::string_view text) {
    std::string result(text);
    std::ranges::transform(result, result.begin(), to_lower);
    return result;
}

}  // namespace

GraphemeBinder::GraphemeBinder(std::span<const Grapheme> graphemes) {
    for (const Grapheme& grapheme : graphemes) {
        if (grapheme.letters.size() > 1) {
            graphemes_.push_back(lower(grapheme.letters));
        }
    }
    std::ranges::sort(graphemes_, [](const std::string& a, const std::string& b) {
        return a.size() != b.size() ? a.size() > b.size() : a < b;
    });
    graphemes_.erase(std::ranges::unique(graphemes_).begin(), graphemes_.end());
}

GraphemeBinder::GraphemeBinder(const PhaseIndex& index)
    : GraphemeBinder(std::span<const Grapheme>(index.graphemes())) {}

std::vector<GraphemeBinder::Block> GraphemeBinder::bind(std::span<const std::string> tiles) const {
    // The letters of the whole path, and where each tile's letters start in it.
    std::string text;
    std::vector<std::size_t> starts;
    starts.reserve(tiles.size() + 1);
    for (const std::string& tile : tiles) {
        starts.push_back(text.size());
        text += lower(tile);
    }
    starts.push_back(text.size());

    std::vector<Block> blocks;
    std::size_t at = 0;
    while (at < tiles.size()) {
        Block block;
        block.first = at;

        for (const std::string& grapheme : graphemes_) {
            if (text.compare(starts[at], grapheme.size(), grapheme) != 0) {
                continue;
            }
            // It has to end where a tile does: a block is whole tiles.
            const std::size_t end = starts[at] + grapheme.size();
            const auto boundary = std::ranges::find(
                starts.begin() + static_cast<std::ptrdiff_t>(at) + 1, starts.end(), end);
            if (boundary == starts.end()) {
                continue;
            }
            const auto tile_count = static_cast<std::size_t>(boundary - starts.begin()) - at;
            if (tile_count > 1) {
                block.tiles = tile_count;
                block.grapheme = grapheme;
                break;
            }
        }

        blocks.push_back(std::move(block));
        at += blocks.back().tiles;
    }
    return blocks;
}

std::vector<GraphemeBinder::Block> GraphemeBinder::bind(const Board& board,
                                                        std::span<const Position> path) const {
    std::vector<std::string> tiles;
    tiles.reserve(path.size());
    for (const Position at : path) {
        tiles.push_back(board.tile(at));
    }
    return bind(tiles);
}

}  // namespace wjs
