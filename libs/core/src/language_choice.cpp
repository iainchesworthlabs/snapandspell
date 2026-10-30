// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/language_choice.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ascii.hpp"
#include "sns/settings.hpp"

namespace sns {

namespace {

// The subtags of a tag in lower case, whether it is written with '-' or '_'.
[[nodiscard]] std::vector<std::string> subtags(std::string_view tag) {
    std::vector<std::string> parts;
    std::string current;
    for (const char c : tag) {
        if (c == '-' || c == '_') {
            parts.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(ascii::to_lower(c));
        }
    }
    parts.push_back(std::move(current));
    return parts;
}

// The code in `languages` for a tag, or empty when it has no catalogue.
[[nodiscard]] std::string_view code_for(std::string_view tag) {
    const std::vector<std::string> parts = subtags(tag);
    std::string_view language = parts.front();
    if (language == "iw") {
        language = "he";
    } else if (language == "ji") {
        language = "yi";
    }
    if (language == "zh") {
        // The script says which it is when the tag gives one ("zh-Hans-HK" is Simplified as
        // written in Hong Kong); only a tag with no script is read by its region.
        static constexpr std::array<std::string_view, 4> traditional = {"hant", "tw", "hk", "mo"};
        const auto rest = parts | std::views::drop(1);
        const bool is_simplified = std::ranges::find(rest, "hans") != rest.end();
        const bool is_traditional =
            !is_simplified && std::ranges::any_of(rest, [](const std::string& part) {
                return std::ranges::find(traditional, part) != traditional.end();
            });
        return is_traditional ? std::string_view{} : canonical_language("zh_CN");
    }
    // "zh_CN" is the only code with a region; the others are the language.
    for (const std::string_view code : languages) {
        if (code == language) {
            return code;
        }
    }
    return {};
}

}  // namespace

LanguageChoice choose_language(std::span<const std::string> preferred) {
    for (const std::string& tag : preferred) {
        const std::string_view code = code_for(tag);
        if (!code.empty()) {
            return {.code = std::string(code), .matched = tag};
        }
    }
    return {.code = "en", .matched = {}};
}

LanguageChoice choose_language(std::string_view tag) {
    const std::array<std::string, 1> one = {std::string(tag)};
    return choose_language(one);
}

}  // namespace sns
