// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/lexicon.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <utility>

#include "ascii.hpp"

namespace sns {

namespace {

[[nodiscard]] bool is_word(std::string_view word) {
    return !word.empty() && std::ranges::all_of(word, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    });
}

[[nodiscard]] std::string lower(std::string_view word) {
    std::string result(word);
    std::ranges::transform(result, result.begin(), [](char c) { return ascii::to_lower(c); });
    return result;
}

[[nodiscard]] std::size_t index_of(Category category) noexcept {
    return static_cast<std::size_t>(category);
}

// The trailing carriage return of a file written on Windows is not part of the word.
[[nodiscard]] std::string_view trimmed(std::string_view line) noexcept {
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
        line.remove_suffix(1);
    }
    return line;
}

}  // namespace

std::string_view to_string(Category category) noexcept {
    switch (category) {
        case Category::animals:
            return "animals";
        case Category::colors:
            return "colors";
        case Category::shapes:
            return "shapes";
        case Category::food:
            return "food";
        case Category::body:
            return "body";
        case Category::family:
            return "family";
        case Category::nature:
            return "nature";
        case Category::home:
            return "home";
        case Category::clothes:
            return "clothes";
        case Category::transport:
            return "transport";
    }
    return "";
}

std::string_view to_string(LexiconError error) noexcept {
    switch (error) {
        case LexiconError::cannot_open:
            return "a file of the lexicon could not be opened";
        case LexiconError::read_failed:
            return "a file of the lexicon could not be read to its end";
    }
    return "";
}

bool Lexicon::add(Category category, std::string_view word) {
    if (!is_word(word)) {
        return false;
    }
    Entry& entry = words_[lower(word)];
    const auto bit = static_cast<std::uint16_t>(1U << index_of(category));
    if ((entry.category_bits & bit) == 0) {
        entry.category_bits = static_cast<std::uint16_t>(entry.category_bits | bit);
        ++counts_[index_of(category)];
    }
    return true;
}

bool Lexicon::add_core(std::string_view word) {
    if (!is_word(word)) {
        return false;
    }
    Entry& entry = words_[lower(word)];
    if (!entry.core) {
        entry.core = true;
        ++core_count_;
    }
    return true;
}

std::expected<Lexicon, LexiconError> Lexicon::load_directory(
    const std::filesystem::path& directory) {
    Lexicon lexicon;
    const auto read = [](const std::filesystem::path& path,
                         auto&& add) -> std::expected<void, LexiconError> {
        std::ifstream in(path);
        if (!in) {
            return std::unexpected(LexiconError::cannot_open);
        }
        std::string line;
        while (std::getline(in, line)) {
            const std::string_view word = trimmed(line);
            if (!word.empty()) {
                static_cast<void>(add(word));
            }
        }
        if (in.bad()) {
            return std::unexpected(LexiconError::read_failed);
        }
        return {};
    };

    if (auto core = read(directory / "core-words.txt",
                         [&lexicon](std::string_view word) { return lexicon.add_core(word); });
        !core) {
        return std::unexpected(core.error());
    }
    for (const Category category : categories) {
        const std::filesystem::path path =
            directory / "categories" / (std::string(to_string(category)) + ".txt");
        if (auto result =
                read(path, [&lexicon, category](
                               std::string_view word) { return lexicon.add(category, word); });
            !result) {
            return std::unexpected(result.error());
        }
    }
    return lexicon;
}

std::size_t Lexicon::size(Category category) const noexcept {
    return counts_[index_of(category)];
}

bool Lexicon::is_core(std::string_view word) const {
    const auto found = words_.find(lower(word));
    return found != words_.end() && found->second.core;
}

bool Lexicon::contains(std::string_view word) const {
    return words_.contains(lower(word));
}

std::vector<Category> Lexicon::categories_of(std::string_view word) const {
    std::vector<Category> result;
    const auto found = words_.find(lower(word));
    if (found == words_.end()) {
        return result;
    }
    for (const Category category : categories) {
        if ((found->second.category_bits & (1U << index_of(category))) != 0) {
            result.push_back(category);
        }
    }
    return result;
}

std::optional<Category> category_of(std::string_view word, const Lexicon& lexicon) {
    const std::vector<Category> all = lexicon.categories_of(word);
    if (all.empty()) {
        return std::nullopt;
    }
    return all.front();
}

std::optional<CategoryHint> category_hint(std::span<const FoundWord> viable,
                                          const FoundWords& found, const Lexicon& lexicon) {
    const FoundWord* best = nullptr;
    std::optional<Category> best_category;
    for (const FoundWord& candidate : viable) {
        if (candidate.word.empty() || found.contains(candidate.word)) {
            continue;
        }
        const std::optional<Category> category = category_of(candidate.word, lexicon);
        if (!category) {
            continue;
        }
        const bool better =
            best == nullptr || candidate.word.size() < best->word.size() ||
            (candidate.word.size() == best->word.size() && candidate.word < best->word);
        if (better) {
            best = &candidate;
            best_category = category;
        }
    }
    if (best == nullptr) {
        return std::nullopt;
    }
    return CategoryHint{.category = *best_category,
                        .letters = best->word.size(),
                        .first_letter = ascii::to_upper(best->word.front())};
}

}  // namespace sns
