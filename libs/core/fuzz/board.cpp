// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdlib>

#include "fuzz_input.hpp"
#include "wjs/board.hpp"

// The text of a board, as a person types it or the store keeps it. A board that parses has at least
// one tile, every position it contains has a tile of one to three letters, and nothing outside it
// does.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto board = wjs::Board::parse(wjs::test::fuzz_text(data, size));
    if (!board) {
        return 0;
    }
    if (board->rows() == 0 || board->columns() == 0 || board->rows() > wjs::Board::max_dimension ||
        board->columns() > wjs::Board::max_dimension) {
        std::abort();
    }
    for (std::size_t row = 0; row < board->rows(); ++row) {
        for (std::size_t column = 0; column < board->columns(); ++column) {
            const wjs::Position position{row, column};
            if (!board->contains(position)) {
                std::abort();
            }
            const auto& tile = board->tile(position);
            if (tile.empty() || tile.size() > wjs::Board::max_tile_letters) {
                std::abort();
            }
        }
    }
    if (board->contains(wjs::Position{board->rows(), 0}) ||
        board->contains(wjs::Position{0, board->columns()})) {
        std::abort();
    }
    return 0;
}
