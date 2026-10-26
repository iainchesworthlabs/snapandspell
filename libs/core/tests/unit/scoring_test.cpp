// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <string>

#include "sns/scoring.hpp"

TEST_CASE("standard Boggle scoring follows the printed rules", "[scoring]") {
    using sns::ScoringScheme;
    const auto scheme = ScoringScheme::standard_boggle;

    CHECK(sns::score_word("AT", scheme) == 0);
    CHECK(sns::score_word("CAT", scheme) == 1);
    CHECK(sns::score_word("CARE", scheme) == 1);
    CHECK(sns::score_word("CARES", scheme) == 2);
    CHECK(sns::score_word("CARESS", scheme) == 3);
    CHECK(sns::score_word("CARESSE", scheme) == 5);
    CHECK(sns::score_word("CARESSES", scheme) == 11);
    CHECK(sns::score_word(std::string(12, 'A'), scheme) == 11);
}

TEST_CASE("child-facing word points reward length and rare letters", "[scoring]") {
    // 3 letters: 10 + C3 A1 T1.
    CHECK(sns::word_points("CAT") == 15);
    // 4 letters: 20 + R1 A1 T1 E1.
    CHECK(sns::word_points("RATE") == 24);
    // 6 letters: 55 + S1 P3 L1 A1 S1 H4 + 10 for the sixth letter.
    CHECK(sns::word_points("SPLASH") == 76);
    // 6 letters: 55 + Q10 U1 A1 R1 T1 Z10 + 10 for the sixth letter.
    CHECK(sns::word_points("QUARTZ") == 89);
    // 8 letters: 110 + 10 + 30 for the letters past five.
    CHECK(sns::word_points("CARESSES") == 150);
    CHECK(sns::word_points("at") == 0);
    CHECK(sns::word_points("") == 0);
}

TEST_CASE("letter-rarity scoring rewards rare letters", "[scoring]") {
    using sns::ScoringScheme;
    const auto scheme = ScoringScheme::letter_rarity;

    // 100 / 12.02, truncated: E is the commonest letter, so it scores least.
    CHECK(sns::score_word("E", scheme) == 8);
    // 100 / 0.07, truncated.
    CHECK(sns::score_word("Z", scheme) == 1428);
    // The mean of the two: (8.319 + 1428.571) / 2.
    CHECK(sns::score_word("EZ", scheme) == 718);
    CHECK(sns::score_word("ez", scheme) == 718);
    CHECK(sns::score_word("", scheme) == 0);
}

TEST_CASE("a word of nine letters or more is worth 110 and 20 more for each letter past eight",
          "[scoring]") {
    // 9 letters: 130 + nine A1 + 10 for each letter past five (four).
    CHECK(sns::word_points("AAAAAAAAA") == 130 + 9 + 40);
    // 10 letters: 150 + ten A1 + five.
    CHECK(sns::word_points("AAAAAAAAAA") == 150 + 10 + 50);
    // 12 letters: 190 + twelve A1 + seven.
    CHECK(sns::word_points(std::string(12, 'A')) == 190 + 12 + 70);
    // Each step of length is worth more than the step before, so a longer word
    // never scores below a shorter one of the same letters.
    int before = 0;
    for (std::size_t length = 3; length <= 16; ++length) {
        const int points = sns::word_points(std::string(length, 'E'));
        INFO("length " << length);
        CHECK(points > before);
        before = points;
    }
}

TEST_CASE("each letter is worth its English Scrabble value in a word", "[scoring]") {
    // 3 letters: 10, then two A1 beside the letter tried.
    const int base = 10 + 2;
    const int values[26] = {1, 3, 3, 2,  1, 4, 2, 4, 1, 8, 5, 1, 3,
                            1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10};
    for (int i = 0; i < 26; ++i) {
        const char upper = static_cast<char>('A' + i);
        const char lower = static_cast<char>('a' + i);
        INFO("letter " << upper);
        CHECK(sns::word_points(std::string{upper, 'A', 'A'}) == base + values[i]);
        CHECK(sns::word_points(std::string{lower, 'a', 'a'}) == base + values[i]);
    }
}

TEST_CASE("a character that is not a letter adds no value but counts in the length", "[scoring]") {
    CHECK(sns::word_points("C-T") == 10 + 3 + 1);
    CHECK(sns::word_points("---") == 10);
    CHECK(sns::word_points("AA") == 0);
}

TEST_CASE("standard Boggle scores are the same in either case and for any length of seven or more",
          "[scoring]") {
    const auto scheme = sns::ScoringScheme::standard_boggle;
    CHECK(sns::score_word("caress", scheme) == sns::score_word("CARESS", scheme));
    CHECK(sns::score_word("", scheme) == 0);
    CHECK(sns::score_word("AAAAAAA", scheme) == 5);
    CHECK(sns::score_word("AAAAAAAAA", scheme) == 11);
}
