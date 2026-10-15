#pragma once

#include <string_view>

namespace wjs {

enum class ScoringScheme {
    // The scoring printed in the Boggle rules: 3 or 4 letters score 1, 5 score
    // 2, 6 score 3, 7 score 5, and 8 or more score 11.
    standard_boggle,

    // The scheme of the original solver: the mean of 100 / (the percentage
    // frequency of each letter in English), so a word made of rare letters
    // scores higher than one made of common letters.
    letter_rarity,
};

// The score of a word of letters A-Z, in either case.
[[nodiscard]] int score_word(std::string_view word, ScoringScheme scheme) noexcept;

}  // namespace wjs
