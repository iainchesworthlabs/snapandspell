#pragma once

// The boards every budget is measured on: the same boards on every machine,
// compiler and standard library, so that a count is a count of one thing.

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

#include "wjs/board.hpp"

namespace wjs::perf {

// The random boards: sizes 3x3 to 6x6, kBoardsPerSize of each. Board `index`
// of a size is drawn from std::mt19937 seeded 7919 * index + size, one draw
// for each tile in row-major order, taken modulo (never through
// std::uniform_int_distribution, whose algorithm is each standard library's
// own, so that the same seed is a different board on MSVC, libstdc++ and
// libc++). The letter is chosen by those draws against ENABLE's letter counts,
// so that the boards hold the letters English does in the proportions it does.
// It is a test corpus, not a set of dice.
inline constexpr std::array<std::size_t, 4> kSizes = {3, 4, 5, 6};
inline constexpr unsigned kBoardsPerSize = 200;

[[nodiscard]] constexpr unsigned corpus_seed(std::size_t size, unsigned index) noexcept {
    return 7919U * index + static_cast<unsigned>(size);
}

[[nodiscard]] wjs::Board random_board(std::size_t size, unsigned index);

// Boards built to hold many words (letters chosen for yield; not dice sets):
// where a search costs the most, and where the result is largest.
struct RichBoard {
    std::string_view name;
    std::size_t size;
    std::string_view text;
};

inline constexpr std::array<RichBoard, 4> kRichBoards = {{
    {"4x4 rich", 4, "S E R S/P A T G/L I N E/S E R S"},
    {"5x5 rich", 5, "S E R S E/P A T G L/L I N E S/S E R S A/T E N I L"},
    {"6x6 rich", 6, "S E R S E A/P A T G L R/L I N E S T/S E R S A I/T E N I L E/R A S T E S"},
    {"6x6 mix", 6, "S T A R E S/E A R T S E/R E S A T R/A S T E R A/T R E S A S/S A R T E E"},
}};

[[nodiscard]] wjs::Board rich_board(const RichBoard& rich);

// Boards no word can be traced on (every tile the same letter): the search's
// cost where it finds nothing.
inline constexpr std::array<RichBoard, 2> kEmptyBoards = {{
    {"4x4 all E", 4, "E E E E/E E E E/E E E E/E E E E"},
    {"6x6 all A", 6, "A A A A A A/A A A A A A/A A A A A A/A A A A A A/A A A A A A/A A A A A A"},
}};

}  // namespace wjs::perf
