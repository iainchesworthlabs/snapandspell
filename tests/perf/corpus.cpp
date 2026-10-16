#include "corpus.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>

namespace wjs::perf {
namespace {

// How many times each letter A to Z occurs in the words of data/dictionaries/
// en.txt (ENABLE, 172,823 words, 1,570,540 letters). A fixed table and not a
// count of the file, so that the corpus does not move if the word list does:
// a changed list is a changed dictionary.words budget and a person's decision.
constexpr std::array<std::uint64_t, 26> letter_counts = {
    118'845, 28'836,  64'179, 53'136, 180'795, 19'335,  42'424, 36'493, 141'464,
    2'497,   13'313,  83'427, 44'531, 107'521, 103'536, 46'141, 2'535,  111'063,
    149'454, 104'967, 51'299, 15'363, 11'689,  4'610,   25'637, 7'450};

constexpr std::uint64_t letter_total =
    std::accumulate(letter_counts.begin(), letter_counts.end(), std::uint64_t{0});

// The letter a draw selects: the draw is made a fraction of a million, scaled
// to the letter total, and the first letter whose running count passes it wins.
// Integer arithmetic throughout, so the same draw is the same letter everywhere.
[[nodiscard]] char letter_for(std::uint32_t draw) noexcept {
    const std::uint64_t scaled = (std::uint64_t{draw} % 1'000'000U) * letter_total / 1'000'000U;
    std::uint64_t running = 0;
    for (std::size_t i = 0; i < letter_counts.size(); ++i) {
        running += letter_counts[i];
        if (running > scaled) {
            return static_cast<char>('A' + i);
        }
    }
    return 'Z';
}

[[nodiscard]] wjs::Board parse_or_die(std::string_view text) {
    auto board = wjs::Board::parse(text);
    // A corpus board that does not parse is a mistake in this file, and every
    // case would fail on it; there is nothing to recover to.
    if (!board) {
        std::abort();
    }
    return *std::move(board);
}

}  // namespace

wjs::Board random_board(std::size_t size, unsigned index) {
    std::mt19937 rng(corpus_seed(size, index));
    std::string text;
    for (std::size_t row = 0; row < size; ++row) {
        if (row != 0) {
            text += '/';
        }
        for (std::size_t column = 0; column < size; ++column) {
            if (column != 0) {
                text += ' ';
            }
            text += letter_for(static_cast<std::uint32_t>(rng() % 1'000'000U));
        }
    }
    return parse_or_die(text);
}

wjs::Board rich_board(const RichBoard& rich) {
    return parse_or_die(rich.text);
}

}  // namespace wjs::perf
