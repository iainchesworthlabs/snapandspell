// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "sns/cli_arguments.hpp"
#include "sns/scoring.hpp"

namespace {

using Args = std::vector<std::string>;

}  // namespace

TEST_CASE("a board row is a board, and the defaults are the program's", "[cli]") {
    const auto parsed = sns::parse_cli_arguments(Args{"TAPE", "SORT"});
    REQUIRE(parsed.has_value());
    CHECK(parsed->board_text == "TAPE\nSORT\n");
    CHECK_FALSE(parsed->dictionary.has_value());
    CHECK(parsed->repeat == 1);
    CHECK_FALSE(parsed->help);
    CHECK_FALSE(parsed->version);
}

TEST_CASE("every option takes its value, in its short or its long form", "[cli]") {
    const auto parsed = sns::parse_cli_arguments(
        Args{"-d", "words.txt", "--min-length", "4", "-s", "rarity", "--repeat", "3", "Qu I T"});
    REQUIRE(parsed.has_value());
    CHECK(parsed->dictionary == std::filesystem::path{"words.txt"});
    CHECK(parsed->options.min_length == 4);
    CHECK(parsed->options.scoring == sns::ScoringScheme::letter_rarity);
    CHECK(parsed->repeat == 3);
    CHECK(parsed->board_text == "Qu I T\n");
}

TEST_CASE("help and version are flags, and the board may be missing", "[cli]") {
    const auto help = sns::parse_cli_arguments(Args{"-h"});
    REQUIRE(help.has_value());
    CHECK(help->help);
    CHECK(help->board_text.empty());
    const auto version = sns::parse_cli_arguments(Args{"--version"});
    REQUIRE(version.has_value());
    CHECK(version->version);
}

TEST_CASE("a lone dash is a board row, not an option", "[cli]") {
    const auto parsed = sns::parse_cli_arguments(Args{"-"});
    REQUIRE(parsed.has_value());
    CHECK(parsed->board_text == "-\n");
}

TEST_CASE("a mistake is an error that names it", "[cli]") {
    SECTION("an option with no value") {
        const auto parsed = sns::parse_cli_arguments(Args{"TAPE", "-m"});
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error() == "-m needs a value");
    }
    SECTION("a length that is not a number") {
        const auto parsed = sns::parse_cli_arguments(Args{"-m", "three"});
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error() == "'three' is not a length");
    }
    SECTION("a length with something after the number") {
        const auto parsed = sns::parse_cli_arguments(Args{"-m", "3x"});
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error() == "'3x' is not a length");
    }
    SECTION("a length too big for its type") {
        const auto parsed = sns::parse_cli_arguments(Args{"-m", "99999999999999999999"});
        REQUIRE_FALSE(parsed.has_value());
    }
    SECTION("a repeat of nothing") {
        const auto parsed = sns::parse_cli_arguments(Args{"-r", "0"});
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error() == "'0' is not a count of one or more");
    }
    SECTION("a scoring scheme that is not one") {
        const auto parsed = sns::parse_cli_arguments(Args{"-s", "bogus"});
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error() == "'bogus' is not a scoring scheme (boggle or rarity)");
    }
    SECTION("an option that is not one") {
        const auto parsed = sns::parse_cli_arguments(Args{"--nope"});
        REQUIRE_FALSE(parsed.has_value());
        CHECK(parsed.error() == "unknown option '--nope'");
    }
}

TEST_CASE("an option's value is its value even when it looks like an option", "[cli]") {
    const auto parsed = sns::parse_cli_arguments(Args{"-d", "--help", "TAPE"});
    REQUIRE(parsed.has_value());
    CHECK(parsed->dictionary == std::filesystem::path{"--help"});
    CHECK_FALSE(parsed->help);
}
