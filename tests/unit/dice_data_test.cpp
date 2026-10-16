#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "wjs/board.hpp"
#include "wjs/dice.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/solver.hpp"

// The project's own dice, data/dice/<n>x<n>.txt, made by tools/data/make_dice.ps1
// from the dictionary's letter counts. These cases read the shipped files and
// the shipped dictionary by their paths in this checkout, as the performance
// tier does: they are about the data, not about the code. That the files are
// what the tool makes (and not a stale copy) is data.dice, in ctest.

namespace {

// data/ in this checkout, as a path CMake passes in.
constexpr std::string_view data_directory = WJS_DATA_DIRECTORY;

std::filesystem::path dice_file(std::size_t side) {
    return std::filesystem::path(data_directory) / "dice" /
           (std::to_string(side) + "x" + std::to_string(side) + ".txt");
}

wjs::DiceSet load(std::size_t side) {
    auto set = wjs::DiceSet::load_file(dice_file(side));
    REQUIRE(set.has_value());
    return *std::move(set);
}

const wjs::Dictionary& english() {
    static const wjs::Dictionary dictionary = [] {
        auto loaded = wjs::Dictionary::load_file(std::filesystem::path(data_directory) /
                                                 "dictionaries" / "en.txt");
        REQUIRE(loaded.has_value());
        return *std::move(loaded);
    }();
    return dictionary;
}

}  // namespace

TEST_CASE("each shipped set has the dice for its board and six faces to a die", "[dice][data]") {
    for (const std::size_t side : {3U, 4U, 5U, 6U}) {
        const auto set = load(side);

        INFO("side " << side);
        CHECK(set.size() == side * side);
        CHECK(set.side() == side);
        for (std::size_t i = 0; i < set.size(); ++i) {
            const auto& faces = set.die(i);
            CHECK(faces.size() == 6);
            // Six different faces, and a vowel among them.
            CHECK(std::set<std::string>(faces.begin(), faces.end()).size() == 6);
            const bool vowel = std::ranges::any_of(faces, [](const std::string& face) {
                return face == "A" || face == "E" || face == "I" || face == "O" || face == "U";
            });
            CHECK(vowel);
        }
    }
}

TEST_CASE("boards rolled from the shipped dice hold about the words a random board does",
          "[dice][data]") {
    // The design measured boards of letters drawn by the dictionary's letter
    // counts at 31, 120, 254 and 435 words on average, a 3x3 to a 6x6. Dice, with
    // six different letters to a die, come out close; these bounds are loose
    // enough to hold if the dice are made again and tight enough to catch a set
    // that has gone wrong (all one letter, no vowels).
    struct Expect {
        std::size_t side;
        double low;
        double high;
        double no_word_share_limit;
    };
    const std::vector<Expect> expected = {{3, 20.0, 45.0, 0.08},
                                          {4, 70.0, 150.0, 0.02},
                                          {5, 180.0, 330.0, 0.01},
                                          {6, 350.0, 600.0, 0.01}};

    for (const auto& expect : expected) {
        const auto set = load(expect.side);

        double total = 0;
        std::size_t without_words = 0;
        constexpr std::uint32_t boards = 300;
        for (std::uint32_t seed = 1; seed <= boards; ++seed) {
            const auto board = wjs::roll(set, seed);
            REQUIRE(board.has_value());
            REQUIRE(board->rows() == expect.side);
            const auto found = wjs::solve(*board, english());
            total += static_cast<double>(found.size());
            if (found.empty()) {
                ++without_words;
            }
        }

        INFO("side " << expect.side << " mean " << total / boards);
        CHECK(total / boards >= expect.low);
        CHECK(total / boards <= expect.high);
        CHECK(static_cast<double>(without_words) / boards <= expect.no_word_share_limit);
    }
}

TEST_CASE("a seed rolls the same board from the shipped dice on every compiler", "[dice][data]") {
    // Pinned, as the seeding's boards are: a roll is std::mt19937 taken modulo and a
    // shuffle of the library's own. The dice files are fixed by the tool's seed,
    // so these change only if the dice or the roll do.
    const auto text = [](const wjs::Board& board) {
        std::string result;
        for (std::size_t row = 0; row < board.rows(); ++row) {
            if (row > 0) {
                result += '/';
            }
            for (std::size_t column = 0; column < board.columns(); ++column) {
                if (column > 0) {
                    result += ' ';
                }
                result += board.tile({row, column});
            }
        }
        return result;
    };

    CHECK(text(*wjs::roll(load(3), 1)) == "Z C N/R R V/M A B");
    CHECK(text(*wjs::roll(load(4), 1)) == "E A L N/O D O I/S K C T/H U T O");
}
