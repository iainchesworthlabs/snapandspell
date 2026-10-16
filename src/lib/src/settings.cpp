#include "wjs/settings.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace wjs {

namespace {

[[nodiscard]] char lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

[[nodiscard]] char upper(char c) noexcept {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

[[nodiscard]] bool is_name_character(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

[[nodiscard]] bool is_font_character(char c) noexcept {
    return is_name_character(c) || c == '-' || c == '.';
}

template <typename Enum>
[[nodiscard]] Enum or_default(Enum value, Enum fallback, Enum last) noexcept {
    return static_cast<int>(value) >= 0 && static_cast<int>(value) <= static_cast<int>(last)
               ? value
               : fallback;
}

}  // namespace

std::string_view to_string(BoardMode value) noexcept {
    switch (value) {
        case BoardMode::standard:
            return "standard";
        case BoardMode::phonics:
            return "phonics";
    }
    return "standard";
}

std::string_view to_string(SoundItOut value) noexcept {
    switch (value) {
        case SoundItOut::full:
            return "full";
        case SoundItOut::word_only:
            return "word";
        case SoundItOut::off:
            return "off";
    }
    return "full";
}

std::string_view to_string(CelebrationSounds value) noexcept {
    switch (value) {
        case CelebrationSounds::full:
            return "full";
        case CelebrationSounds::quiet:
            return "quiet";
        case CelebrationSounds::off:
            return "off";
    }
    return "full";
}

std::string_view to_string(MotionSetting value) noexcept {
    switch (value) {
        case MotionSetting::system:
            return "system";
        case MotionSetting::reduced:
            return "reduced";
    }
    return "system";
}

std::optional<BoardMode> board_mode_from_string(std::string_view name) noexcept {
    if (name == "standard") {
        return BoardMode::standard;
    }
    if (name == "phonics") {
        return BoardMode::phonics;
    }
    return std::nullopt;
}

std::optional<SoundItOut> sound_it_out_from_string(std::string_view name) noexcept {
    if (name == "full") {
        return SoundItOut::full;
    }
    if (name == "word") {
        return SoundItOut::word_only;
    }
    if (name == "off") {
        return SoundItOut::off;
    }
    return std::nullopt;
}

std::optional<CelebrationSounds> celebration_sounds_from_string(std::string_view name) noexcept {
    if (name == "full") {
        return CelebrationSounds::full;
    }
    if (name == "quiet") {
        return CelebrationSounds::quiet;
    }
    if (name == "off") {
        return CelebrationSounds::off;
    }
    return std::nullopt;
}

std::optional<MotionSetting> motion_from_string(std::string_view name) noexcept {
    if (name == "system") {
        return MotionSetting::system;
    }
    if (name == "reduced") {
        return MotionSetting::reduced;
    }
    return std::nullopt;
}

bool is_valid_name(std::string_view name) noexcept {
    return !name.empty() && name.size() <= max_name_length &&
           std::all_of(name.begin(), name.end(), is_name_character);
}

bool is_valid_font_name(std::string_view name) noexcept {
    return !name.empty() && name != "-" && name.size() <= max_font_name_length &&
           std::all_of(name.begin(), name.end(), is_font_character);
}

Settings Settings::normalized() const {
    Settings result = *this;

    result.board = or_default(result.board, BoardMode::standard, BoardMode::phonics);
    result.sound_it_out = or_default(result.sound_it_out, SoundItOut::full, SoundItOut::off);
    result.celebration_sounds =
        or_default(result.celebration_sounds, CelebrationSounds::full, CelebrationSounds::off);
    result.motion = or_default(result.motion, MotionSetting::system, MotionSetting::reduced);

    std::transform(result.phase.begin(), result.phase.end(), result.phase.begin(), upper);
    if (!is_valid_name(result.phase)) {
        result.phase = "A";
    }

    if (result.board_size < smallest_default_board_size ||
        result.board_size > largest_default_board_size) {
        result.board_size = smallest_default_board_size;
    }

    if (std::find(std::begin(text_size_percents), std::end(text_size_percents),
                  result.text_size_percent) == std::end(text_size_percents)) {
        result.text_size_percent = 100;
    }

    if (!is_valid_font_name(result.school_font)) {
        result.school_font.clear();
    }

    for (std::string& tick : result.focus_sounds) {
        std::transform(tick.begin(), tick.end(), tick.begin(), lower);
    }
    std::erase_if(result.focus_sounds,
                  [](const std::string& tick) { return !is_valid_name(tick); });
    std::sort(result.focus_sounds.begin(), result.focus_sounds.end());
    result.focus_sounds.erase(std::unique(result.focus_sounds.begin(), result.focus_sounds.end()),
                              result.focus_sounds.end());
    if (result.focus_sounds.size() > max_focus_sounds) {
        result.focus_sounds.resize(max_focus_sounds);
    }

    return result;
}

}  // namespace wjs
