#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "wjs/board.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/solver.hpp"

namespace {

wjs::Dictionary make_dictionary(std::initializer_list<std::string_view> words) {
    wjs::Dictionary dictionary;
    for (const auto word : words) {
        dictionary.insert(word);
    }
    return dictionary;
}

wjs::Board make_board(std::string_view text) {
    auto board = wjs::Board::parse(text);
    REQUIRE(board.has_value());
    return *board;
}

std::vector<std::string> words_of(const std::vector<wjs::FoundWord>& found) {
    std::vector<std::string> words;
    for (const auto& entry : found) {
        words.push_back(entry.word);
    }
    return words;
}

}  // namespace

// The board used by the first cases:
//
//     C A T
//     O R E
//     D G S
//
// CATS and REST are in the dictionary but cannot be traced: the S is not next
// to the T, and the T is not next to the S that follows the E.
TEST_CASE("a word is found only if its letters touch", "[solver]") {
    const auto board = make_board("CAT/ORE/DGS");
    const auto dictionary = make_dictionary(
        {"CAT", "CAR", "CARE", "ARE", "TAR", "TEA", "DOG", "COD", "ROD", "CATS", "REST", "AT"});

    const auto found = wjs::solve(board, dictionary);

    // Highest score first, then longest, then alphabetical; AT is below the
    // default minimum of three letters.
    const std::vector<std::string> expected = {"CARE", "ARE", "CAR", "CAT", "COD",
                                               "DOG",  "ROD", "TAR", "TEA"};
    CHECK(words_of(found) == expected);
}

TEST_CASE("the minimum length can be lowered", "[solver]") {
    const auto board = make_board("CAT/ORE/DGS");
    const auto dictionary = make_dictionary({"CAT", "AT"});

    wjs::SolveOptions options;
    options.min_length = 2;
    const auto found = wjs::solve(board, dictionary, options);

    const std::vector<std::string> expected = {"CAT", "AT"};
    CHECK(words_of(found) == expected);
}

TEST_CASE("each word reports the tiles that spell it", "[solver]") {
    const auto board = make_board("CAT/ORE/DGS");
    const auto dictionary = make_dictionary({"CARE"});

    const auto found = wjs::solve(board, dictionary);

    REQUIRE(found.size() == 1U);
    const std::vector<wjs::Position> expected = {{0, 0}, {0, 1}, {1, 1}, {1, 2}};
    CHECK(found.front().path == expected);
    CHECK(found.front().score == 1);
}

TEST_CASE("a tile is not used twice in one word", "[solver]") {
    const auto board = make_board("AB/CD");
    const auto dictionary = make_dictionary({"ABA", "ABC"});

    const auto found = wjs::solve(board, dictionary);

    const std::vector<std::string> expected = {"ABC"};
    CHECK(words_of(found) == expected);
}

TEST_CASE("a word that can be traced several ways is reported once", "[solver]") {
    const auto board = make_board("AA/AA");
    const auto dictionary = make_dictionary({"AAA"});

    const auto found = wjs::solve(board, dictionary);

    CHECK(found.size() == 1U);
}

TEST_CASE("the Qu tile spends two letters of the word on one tile", "[solver]") {
    const auto board = make_board("Qu I T");
    const auto dictionary = make_dictionary({"QUIT", "QIT"});

    const auto found = wjs::solve(board, dictionary);

    REQUIRE(found.size() == 1U);
    CHECK(found.front().word == "QUIT");
    CHECK(found.front().path.size() == 3U);
    CHECK(found.front().score == 1);
}

TEST_CASE("the scoring scheme is applied to what is found", "[solver]") {
    const auto board = make_board("CAT/ORE/DGS");
    const auto dictionary = make_dictionary({"CAT"});

    wjs::SolveOptions options;
    options.scoring = wjs::ScoringScheme::letter_rarity;
    const auto found = wjs::solve(board, dictionary, options);

    REQUIRE(found.size() == 1U);
    // (100/2.71 + 100/8.12 + 100/9.10) / 3 = 20.07, truncated.
    CHECK(found.front().score == 20);
}

TEST_CASE("an empty dictionary finds nothing", "[solver]") {
    const auto board = make_board("CAT/ORE/DGS");

    CHECK(wjs::solve(board, wjs::Dictionary{}).empty());
}
