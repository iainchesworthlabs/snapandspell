// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <span>
#include <string>
#include <string_view>

namespace sns {

// The interface language the device's preferred languages come to.
struct LanguageChoice {
    // One of `languages` (sns/settings.hpp); "en" when nothing matched.
    std::string code;
    // The preferred tag that decided it, as the device wrote it ("de-DE"), or
    // empty when none matched and English is the fallback.
    std::string matched;
};

// Which of the eight languages to use, given the device's preferred languages
// in the order the person ranked them (BCP 47 or POSIX tags: "en-GB", "de_DE",
// "zh-Hant-TW"). The rules are the project standard's, 7.2:
//
//  - Each tag is matched by its language alone, in order; the first one that
//    has a catalogue wins, and a language we do not have is skipped, not an
//    end to the search. Any en-* is English.
//  - "iw" and "ji" are the old codes of Hebrew and Yiddish.
//  - "zh" is Simplified Chinese (zh_CN) unless the tag says Traditional (Hant,
//    TW, HK or MO); a Traditional tag has no catalogue here and is skipped.
//  - Nothing matched is English.
//
// A platform's own translator search is not used for this: its input differs
// from one system to the next (docs/design/application.md, "Languages").
[[nodiscard]] LanguageChoice choose_language(std::span<const std::string> preferred);

// The same rules for a single tag.
[[nodiscard]] LanguageChoice choose_language(std::string_view tag);

}  // namespace sns
