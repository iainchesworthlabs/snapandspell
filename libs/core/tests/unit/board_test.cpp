// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <string>

#include "sns/board.hpp"

TEST_CASE("a row without separators is read one letter per tile", "[board]") {
    const auto board = sns::Board::parse("TAPE\nLORE\nSINK\nMUDS");

    REQUIRE(board.has_value());
    CHECK(board->rows() == 4U);
    CHECK(board->columns() == 4U);
    CHECK(board->tile({0, 0}) == "T");
    CHECK(board->tile({1, 2}) == "R");
    CHECK(board->tile({3, 3}) == "S");
}

TEST_CASE("tiles separated by spaces or commas may hold more than one letter", "[board]") {
    const auto board = sns::Board::parse("qu i t\nA, B, C");

    REQUIRE(board.has_value());
    CHECK(board->rows() == 2U);
    CHECK(board->columns() == 3U);
    CHECK(board->tile({0, 0}) == "QU");
    CHECK(board->tile({0, 2}) == "T");
    CHECK(board->tile({1, 1}) == "B");
}

TEST_CASE("rows may be separated by a slash, and line endings are forgiven", "[board]") {
    const auto slashed = sns::Board::parse("ab/cd");
    REQUIRE(slashed.has_value());
    CHECK(slashed->rows() == 2U);
    CHECK(slashed->tile({1, 0}) == "C");

    const auto windows = sns::Board::parse("AB\r\nCD\r\n");
    REQUIRE(windows.has_value());
    CHECK(windows->rows() == 2U);
    CHECK(windows->columns() == 2U);
}

TEST_CASE("a board that cannot be read says why", "[board]") {
    CHECK(sns::Board::parse("").error() == sns::BoardError::empty);
    CHECK(sns::Board::parse("  \n \n").error() == sns::BoardError::empty);
    CHECK(sns::Board::parse("AB/C").error() == sns::BoardError::ragged);
    CHECK(sns::Board::parse("A1/BC").error() == sns::BoardError::bad_tile);
    CHECK(sns::Board::parse("ABCD E").error() == sns::BoardError::bad_tile);
    CHECK(sns::Board::parse(std::string(17, 'A')).error() == sns::BoardError::too_large);
}

TEST_CASE("contains tells the board from the space around it", "[board]") {
    const auto board = sns::Board::parse("AB/CD");
    REQUIRE(board.has_value());

    CHECK(board->contains({0, 0}));
    CHECK(board->contains({1, 1}));
    CHECK_FALSE(board->contains({2, 0}));
    CHECK_FALSE(board->contains({0, 2}));
}
