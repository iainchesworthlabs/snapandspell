#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

#include "wjs/board.hpp"

// The dice of the standard board: a set of dice, each with faces, and a roll
// that puts them on a board. A face is a tile of one to three letters (the
// "Qu" face of a Boggle die is the two-letter tile QU).
//
// A set is a text file, one die a line, faces separated by spaces or commas,
// `#` lines being the header (data/dice/<n>x<n>.txt):
//
//     # dice for a 4x4 board ...
//     # source-sha256: <hash of the word list the counts came from>
//     E A R I T N
//     S O L Q C D
//     ...
//
// The library holds no set of its own. The project's sets are made by
// tools/data/make_dice.ps1 from the dictionary's letter counts.

namespace wjs {

enum class DiceError {
    empty,        // no dice
    bad_die,      // a line holds no faces, or more than max_faces
    bad_face,     // a face is not one to three letters A-Z
    not_square,   // the number of dice is not a square, so no n by n board
    wrong_count,  // asked for a board of a size the set has not the dice for
    unreadable,   // the file could not be read
};

[[nodiscard]] std::string_view to_string(DiceError error) noexcept;

class DiceSet {
public:
    static constexpr std::size_t max_faces = 12;

    // Faces are stored upper-case.
    [[nodiscard]] static std::expected<DiceSet, DiceError> parse(std::string_view text);
    [[nodiscard]] static std::expected<DiceSet, DiceError> load(std::istream& in);
    [[nodiscard]] static std::expected<DiceSet, DiceError> load_file(
        const std::filesystem::path& path);

    // How many dice.
    [[nodiscard]] std::size_t size() const noexcept { return dice_.size(); }

    // The faces of die `index`, which must be below size().
    [[nodiscard]] const std::vector<std::string>& die(std::size_t index) const {
        return dice_[index];
    }

    // The side of the square board these dice make, or zero when their number
    // is not a square.
    [[nodiscard]] std::size_t side() const noexcept;

private:
    explicit DiceSet(std::vector<std::vector<std::string>> dice) : dice_(std::move(dice)) {}

    std::vector<std::vector<std::string>> dice_;
};

// Shakes the dice onto a square board: they are put in a random order, a die to
// a tile, and each shows one of its faces. A pure function of the set and the
// seed: std::mt19937 taken modulo and a shuffle of the library's own, so a seed
// is the same board on every compiler.
[[nodiscard]] std::expected<Board, DiceError> roll(const DiceSet& dice, std::uint32_t seed);

}  // namespace wjs
