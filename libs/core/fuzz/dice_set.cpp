// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdlib>

#include "fuzz_input.hpp"
#include "wjs/dice.hpp"

// A dice file (data/dice/<n>x<n>.txt). A set that parses has dice of one to max_faces faces, and a
// set with a square number of dice rolls a board of its side.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto set = wjs::DiceSet::parse(wjs::test::fuzz_text(data, size));
    if (!set) {
        return 0;
    }
    if (set->size() == 0) {
        std::abort();
    }
    for (std::size_t index = 0; index < set->size(); ++index) {
        const auto& faces = set->die(index);
        if (faces.empty() || faces.size() > wjs::DiceSet::max_faces) {
            std::abort();
        }
    }
    const std::size_t side = set->side();
    if (side * side == set->size()) {
        const auto rolled = wjs::roll_with_placements(*set, static_cast<std::uint32_t>(size));
        if (rolled && (rolled->board.rows() != side || rolled->placements.size() != set->size())) {
            std::abort();
        }
    }
    return 0;
}
