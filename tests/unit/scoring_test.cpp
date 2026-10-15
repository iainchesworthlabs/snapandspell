#include <catch2/catch_test_macros.hpp>
#include <string>

#include "wjs/scoring.hpp"

TEST_CASE("standard Boggle scoring follows the printed rules", "[scoring]") {
    using wjs::ScoringScheme;
    const auto scheme = ScoringScheme::standard_boggle;

    CHECK(wjs::score_word("AT", scheme) == 0);
    CHECK(wjs::score_word("CAT", scheme) == 1);
    CHECK(wjs::score_word("CARE", scheme) == 1);
    CHECK(wjs::score_word("CARES", scheme) == 2);
    CHECK(wjs::score_word("CARESS", scheme) == 3);
    CHECK(wjs::score_word("CARESSE", scheme) == 5);
    CHECK(wjs::score_word("CARESSES", scheme) == 11);
    CHECK(wjs::score_word(std::string(12, 'A'), scheme) == 11);
}

TEST_CASE("letter-rarity scoring rewards rare letters", "[scoring]") {
    using wjs::ScoringScheme;
    const auto scheme = ScoringScheme::letter_rarity;

    // 100 / 12.02, truncated: E is the commonest letter, so it scores least.
    CHECK(wjs::score_word("E", scheme) == 8);
    // 100 / 0.07, truncated.
    CHECK(wjs::score_word("Z", scheme) == 1428);
    // The mean of the two: (8.319 + 1428.571) / 2.
    CHECK(wjs::score_word("EZ", scheme) == 718);
    CHECK(wjs::score_word("ez", scheme) == 718);
    CHECK(wjs::score_word("", scheme) == 0);
}
