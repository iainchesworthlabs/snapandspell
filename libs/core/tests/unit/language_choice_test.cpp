// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "sns/language_choice.hpp"
#include "sns/settings.hpp"

namespace {

using Tags = std::vector<std::string>;

[[nodiscard]] std::string code_of(const Tags& preferred) {
    return sns::choose_language(preferred).code;
}

}  // namespace

TEST_CASE("any English tag is English", "[language]") {
    for (const char* tag : {"en", "en-GB", "en_US", "en-Latn-GB", "EN-au", "en-Latn"}) {
        CAPTURE(tag);
        CHECK(sns::choose_language(std::string_view(tag)).code == "en");
        CHECK(sns::choose_language(std::string_view(tag)).matched == tag);
    }
}

TEST_CASE("each language is matched by its language alone", "[language]") {
    CHECK(code_of({"de-DE"}) == "de");
    CHECK(code_of({"de_AT"}) == "de");
    CHECK(code_of({"fr-CA"}) == "fr");
    CHECK(code_of({"es-419"}) == "es");
    CHECK(code_of({"ja-JP"}) == "ja");
    CHECK(code_of({"he-IL"}) == "he");
    CHECK(code_of({"yi"}) == "yi");
}

TEST_CASE("the old codes of Hebrew and Yiddish are those languages", "[language]") {
    CHECK(code_of({"iw-IL"}) == "he");
    CHECK(code_of({"iw"}) == "he");
    CHECK(code_of({"ji"}) == "yi");
}

TEST_CASE("Chinese is Simplified unless the tag says Traditional", "[language]") {
    CHECK(code_of({"zh"}) == "zh_CN");
    CHECK(code_of({"zh-CN"}) == "zh_CN");
    CHECK(code_of({"zh_CN"}) == "zh_CN");
    CHECK(code_of({"zh-Hans"}) == "zh_CN");
    CHECK(code_of({"zh-Hans-CN"}) == "zh_CN");
    CHECK(code_of({"zh-SG"}) == "zh_CN");
    for (const char* tag : {"zh-Hant", "zh-Hant-TW", "zh-TW", "zh_TW", "zh-HK", "zh-Hans-HK",
                            "zh-MO", "zh-Hant-MO"}) {
        CAPTURE(tag);
        CHECK(sns::choose_language(std::string_view(tag)).code == "en");
        CHECK(sns::choose_language(std::string_view(tag)).matched.empty());
    }
}

TEST_CASE("the first preferred language that has a catalogue wins", "[language]") {
    // The phone that listed English (UK), German and French got German: the
    // first of the three is the person's, and it is English.
    const Tags uk_phone = {"en-GB", "de-DE", "fr-FR"};
    CHECK(sns::choose_language(uk_phone).code == "en");
    CHECK(sns::choose_language(uk_phone).matched == "en-GB");

    // The platform's own expansion of one preference is still one preference.
    CHECK(code_of({"en-Latn-GB", "en-GB", "en-Latn", "en", "de"}) == "en");

    // A language with no catalogue is skipped, not an end to the search.
    const Tags skips = {"pt-BR", "ru-RU", "fr-CA", "en-US"};
    CHECK(sns::choose_language(skips).code == "fr");
    CHECK(sns::choose_language(skips).matched == "fr-CA");

    // A Traditional Chinese tag is skipped the same way.
    CHECK(code_of({"zh-Hant-TW", "ja-JP"}) == "ja");
    CHECK(code_of({"zh-Hant-TW", "zh-Hans-CN"}) == "zh_CN");
}

TEST_CASE("nothing matched is English, and says so", "[language]") {
    CHECK(sns::choose_language(Tags{}).code == "en");
    CHECK(sns::choose_language(Tags{}).matched.empty());
    const Tags none = {"pt-BR", "ru", "tlh-QS", "", "-"};
    CHECK(sns::choose_language(none).code == "en");
    CHECK(sns::choose_language(none).matched.empty());
}

TEST_CASE("every code the rules give is one of the eight", "[language]") {
    for (const char* tag : {"en-GB", "de", "fr", "es", "yi", "he", "iw", "ji", "zh", "ja", "xx"}) {
        CAPTURE(tag);
        CHECK(sns::is_valid_language(sns::choose_language(std::string_view(tag)).code));
    }
}

TEST_CASE("every language of the eight is reached by its own tag", "[language]") {
    for (const std::string_view code : sns::languages) {
        CAPTURE(code);
        CHECK(sns::choose_language(code).code == code);
    }
}
