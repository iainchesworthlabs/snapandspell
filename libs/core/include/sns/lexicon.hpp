// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "sns/game_rules.hpp"
#include "sns/solver.hpp"

// The child-facing layer of the word list (data/README.md, "lexicon/"): an early-reader tier and
// ten categories, written for the project and every word of them a word of the dictionary, so a
// category is a subset of what the solver accepts and never a source of new words. This is the
// reading side: the library loads the files and answers what a word is, and a hint names the
// category of a word the child has not found yet without handing the word over
// (docs/design/phonics.md, "Hints"). Nothing here changes which words count, the solver or the
// dictionary.
//
// The lists are English and have not been reviewed by a teacher or a parent (data/README.md): the
// hint they give is a nudge, never a finding about a child, and a board whose words are in no
// category simply has no category hint.

namespace sns {

enum class Category {
    animals,
    colors,
    shapes,
    food,
    body,
    family,
    nature,
    home,
    clothes,
    transport,
};

inline constexpr std::array<Category, 10> categories = {
    Category::animals, Category::colors, Category::shapes, Category::food,    Category::body,
    Category::family,  Category::nature, Category::home,   Category::clothes, Category::transport,
};

// The category's name as the files and the code write it ("animals": the file animals.txt). Not
// words for a child: the screens word the category in the child's language.
[[nodiscard]] std::string_view to_string(Category category) noexcept;

enum class LexiconError {
    cannot_open,  // a file of the lexicon could not be opened
    read_failed,  // a stream failed part-way through
};

[[nodiscard]] std::string_view to_string(LexiconError error) noexcept;

class Lexicon {
public:
    // Adds a word to a category, and to the lexicon. False, and nothing added, for an empty word or
    // one with anything but ASCII letters. Words are kept in lower case.
    [[nodiscard]] bool add(Category category, std::string_view word);
    // Adds a word to the early-reader tier (data/lexicon/core-words.txt) alone.
    [[nodiscard]] bool add_core(std::string_view word);

    // The lexicon's directory: core-words.txt, and categories/<name>.txt for each category. One
    // word a line, lower case; a blank line is skipped, and so is a line that is not a word.
    [[nodiscard]] static std::expected<Lexicon, LexiconError> load_directory(
        const std::filesystem::path& directory);

    // Distinct words in any category or the tier.
    [[nodiscard]] std::size_t size() const noexcept { return words_.size(); }
    [[nodiscard]] std::size_t size(Category category) const noexcept;
    [[nodiscard]] std::size_t core_size() const noexcept { return core_count_; }

    // Case-insensitive. Whether the word is in the early-reader tier, whether it is in the lexicon
    // at all, and the categories it is in (in the order of `categories`; empty for a word of the
    // tier alone).
    [[nodiscard]] bool is_core(std::string_view word) const;
    [[nodiscard]] bool contains(std::string_view word) const;
    [[nodiscard]] std::vector<Category> categories_of(std::string_view word) const;

private:
    struct Entry {
        std::uint16_t category_bits = 0;  // bit n is categories[n]
        bool core = false;
    };

    std::unordered_map<std::string, Entry> words_;
    std::array<std::size_t, categories.size()> counts_{};
    std::size_t core_count_ = 0;
};

// What a category hint says: "a 3-letter animal starting with C". Never the word.
struct CategoryHint {
    Category category = Category::animals;
    std::size_t letters = 0;  // the word's length, a Qu tile counted as two
    char first_letter = 'A';  // upper-case
};

// The category hint for one viable word not yet found, chosen as the start tile's help is
// (sns/game_rules.hpp): the shortest such word, the first alphabetically of those, so the same
// press gives the same answer and finding another word cannot change it. Only a word of the lexicon
// is hinted at, and when it is in several categories the first of `categories` names it. `viable`
// is viable_words(...) for the board. Nullopt when no word left is in the lexicon.
[[nodiscard]] std::optional<CategoryHint> category_hint(std::span<const FoundWord> viable,
                                                        const FoundWords& found,
                                                        const Lexicon& lexicon);

// The category that names a word the child has just found ("You found an animal!"), the first of
// `categories` the word is in, or nullopt for a word in none.
[[nodiscard]] std::optional<Category> category_of(std::string_view word, const Lexicon& lexicon);

}  // namespace sns
