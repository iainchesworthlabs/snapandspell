// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include "sns/game_rules.hpp"
#include "sns/lexicon.hpp"
#include "sns/solver.hpp"

namespace {

sns::FoundWord viable(const std::string& word) {
    return sns::FoundWord{.word = word, .path = {sns::Position{0, 0}}, .score = 1};
}

sns::Lexicon small() {
    sns::Lexicon lexicon;
    REQUIRE(lexicon.add(sns::Category::animals, "cat"));
    REQUIRE(lexicon.add(sns::Category::animals, "COW"));
    REQUIRE(lexicon.add(sns::Category::colors, "red"));
    // In two categories: the first of the list names it.
    REQUIRE(lexicon.add(sns::Category::nature, "star"));
    REQUIRE(lexicon.add(sns::Category::shapes, "star"));
    REQUIRE(lexicon.add_core("sat"));
    return lexicon;
}

}  // namespace

TEST_CASE("a lexicon keeps words in lower case, once, with their categories", "[lexicon]") {
    const sns::Lexicon lexicon = small();
    CHECK(lexicon.size() == 5);
    CHECK(lexicon.size(sns::Category::animals) == 2);
    CHECK(lexicon.size(sns::Category::shapes) == 1);
    CHECK(lexicon.core_size() == 1);
    CHECK(lexicon.contains("CAT"));
    CHECK(lexicon.contains("cow"));
    CHECK_FALSE(lexicon.contains("dog"));
    CHECK(lexicon.is_core("SAT"));
    CHECK_FALSE(lexicon.is_core("cat"));
    CHECK(lexicon.categories_of("sat").empty());
    // The order of `categories`, not the order added: nature was added first.
    CHECK(lexicon.categories_of("star") ==
          std::vector<sns::Category>{sns::Category::shapes, sns::Category::nature});
}

TEST_CASE("a lexicon refuses what is not a word and counts a repeat once", "[lexicon]") {
    sns::Lexicon lexicon;
    CHECK_FALSE(lexicon.add(sns::Category::animals, ""));
    CHECK_FALSE(lexicon.add(sns::Category::animals, "ice-cream"));
    CHECK_FALSE(lexicon.add_core("it's"));
    CHECK(lexicon.add(sns::Category::animals, "cat"));
    CHECK(lexicon.add(sns::Category::animals, "Cat"));
    CHECK(lexicon.size() == 1);
    CHECK(lexicon.size(sns::Category::animals) == 1);
}

TEST_CASE("a category hint names the kind, the length and the first letter, never the word",
          "[lexicon][hint]") {
    const sns::Lexicon lexicon = small();
    const std::vector<sns::FoundWord> words = {viable("RED"), viable("CAT"), viable("SAT"),
                                               viable("COW"), viable("STAR")};
    sns::FoundWords found;

    // The shortest of the words in the lexicon, the first alphabetically of those: CAT before COW
    // and RED; SAT is in the tier alone and has no category to name.
    auto hint = sns::category_hint(words, found, lexicon);
    REQUIRE(hint.has_value());
    CHECK(hint->category == sns::Category::animals);
    CHECK(hint->letters == 3);
    CHECK(hint->first_letter == 'C');

    // The same again, and a find of another word does not move it.
    CHECK(sns::category_hint(words, found, lexicon)->first_letter == 'C');
    REQUIRE(found.insert("RED"));
    CHECK(sns::category_hint(words, found, lexicon)->first_letter == 'C');

    // When it is found the next is named.
    REQUIRE(found.insert("CAT"));
    hint = sns::category_hint(words, found, lexicon);
    REQUIRE(hint.has_value());
    CHECK(hint->category == sns::Category::animals);
    CHECK(hint->first_letter == 'C');
    REQUIRE(found.insert("COW"));
    hint = sns::category_hint(words, found, lexicon);
    REQUIRE(hint.has_value());
    CHECK(hint->category == sns::Category::shapes);
    CHECK(hint->letters == 4);
    CHECK(hint->first_letter == 'S');

    // Nothing left in the lexicon, no hint.
    REQUIRE(found.insert("STAR"));
    CHECK_FALSE(sns::category_hint(words, found, lexicon).has_value());
    CHECK_FALSE(sns::category_hint({}, sns::FoundWords{}, lexicon).has_value());
}

TEST_CASE("the category of a found word is the first it is in", "[lexicon]") {
    const sns::Lexicon lexicon = small();
    CHECK(sns::category_of("cat", lexicon) == sns::Category::animals);
    CHECK(sns::category_of("STAR", lexicon) == sns::Category::shapes);
    CHECK_FALSE(sns::category_of("sat", lexicon).has_value());
    CHECK_FALSE(sns::category_of("dog", lexicon).has_value());
}

TEST_CASE("the shipped lexicon loads, with its tier and its ten categories", "[lexicon][data]") {
    const std::filesystem::path data = SNS_DATA_DIRECTORY;
    const auto lexicon = sns::Lexicon::load_directory(data / "lexicon");
    REQUIRE(lexicon.has_value());
    CHECK(lexicon->core_size() > 100);
    for (const sns::Category category : sns::categories) {
        CAPTURE(sns::to_string(category));
        CHECK(lexicon->size(category) > 10);
    }
    CHECK(lexicon->categories_of("cat") == std::vector<sns::Category>{sns::Category::animals});
    CHECK(lexicon->is_core("about"));
    CHECK_FALSE(lexicon->is_core("cat"));
}

TEST_CASE("a lexicon directory that is not there is an error that says so", "[lexicon]") {
    const auto lexicon = sns::Lexicon::load_directory("no-such-directory-for-the-lexicon");
    REQUIRE_FALSE(lexicon.has_value());
    CHECK(lexicon.error() == sns::LexiconError::cannot_open);
    CHECK_FALSE(sns::to_string(lexicon.error()).empty());
}
