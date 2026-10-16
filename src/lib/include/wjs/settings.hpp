#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// What a parent sets (docs/design/dashboard.md, "Settings"). A plain value:
// the store keeps one, the screens edit a copy, and `normalized()` is the one
// place that says which values are allowed, so a hand-edited or damaged file
// and a screen with a bug end up with the same safe settings.
//
// The names of values are the ones in the table of the design. Phase names are
// generic ("A", "B"): what each contains is the reviewer's, and the library
// knows no phase (docs/design/phonics.md).

namespace wjs {

enum class BoardMode {
    standard,  // dice and the full dictionary (the default)
    phonics,   // a phase's words, grapheme blocks, sounding out
};

enum class SoundItOut {
    full,       // each sound, then the word
    word_only,  // the word
    off,
};

enum class CelebrationSounds {
    full,
    quiet,
    off,
};

enum class MotionSetting {
    system,   // follow the system's reduced-motion setting
    reduced,  // reduce, whatever the system says
};

// Each enum is written in the store and in the screens by these names, and
// read back by from_string, which is empty for a name it does not know.
[[nodiscard]] std::string_view to_string(BoardMode value) noexcept;
[[nodiscard]] std::string_view to_string(SoundItOut value) noexcept;
[[nodiscard]] std::string_view to_string(CelebrationSounds value) noexcept;
[[nodiscard]] std::string_view to_string(MotionSetting value) noexcept;

[[nodiscard]] std::optional<BoardMode> board_mode_from_string(std::string_view name) noexcept;
[[nodiscard]] std::optional<SoundItOut> sound_it_out_from_string(std::string_view name) noexcept;
[[nodiscard]] std::optional<CelebrationSounds> celebration_sounds_from_string(
    std::string_view name) noexcept;
[[nodiscard]] std::optional<MotionSetting> motion_from_string(std::string_view name) noexcept;

// The board sizes the parent area offers as a default. The standard board plays
// 3x3 to 6x6 and the child picks on the box; the parent's default is one of the
// two sizes the phonics board is designed for (docs/design/game.md).
inline constexpr int smallest_default_board_size = 3;
inline constexpr int largest_default_board_size = 4;

// The text sizes on offer, as a percentage of the system's: 1, 1.25, 1.5.
inline constexpr int text_size_percents[] = {100, 125, 150};

// The longest a phase, a sound or a font name may be, and the most focus
// sounds there can be. A bound, so that a damaged file cannot make a large
// setting.
inline constexpr std::size_t max_name_length = 16;
inline constexpr std::size_t max_font_name_length = 48;
inline constexpr std::size_t max_focus_sounds = 40;

struct Settings {
    // Help: the start tile (on) or none (off). After the MVP a dial of five
    // stops; the MVP has the two.
    bool help = true;

    BoardMode board = BoardMode::standard;

    // The phonics board's phase, upper case ("A"). Used only by that board.
    std::string phase = "A";

    // The sounds the school is on this week (lower case, as written in the
    // graphemes file), sorted and each once. Only sounds of the phase can be
    // ticked: that is the dashboard's to hold to (restrict_focus_sounds).
    std::vector<std::string> focus_sounds;

    // The board size the box offers first: 3x3 on a new device, then the last
    // used. 3 or 4.
    int board_size = 3;

    SoundItOut sound_it_out = SoundItOut::full;
    CelebrationSounds celebration_sounds = CelebrationSounds::full;
    bool sound = true;
    bool vibration = true;
    bool shake = true;  // shake the device to roll
    MotionSetting motion = MotionSetting::system;

    // 100, 125 or 150: a percentage of the system's text size.
    int text_size_percent = 100;

    // A school font's name, or empty for none. Only a font whose licence
    // permits bundling can ever be named here, and none is bundled yet.
    std::string school_font;

    // The same settings with every value brought to one the table allows:
    // a size, a text scale or a mode that is not on offer becomes the default
    // for it, names are cut to their bounds and made of the characters a name
    // may have, and the focus sounds are sorted and unique.
    [[nodiscard]] Settings normalized() const;

    friend bool operator==(const Settings&, const Settings&) = default;
};

// Whether `name` can be a phase or a sound: one to max_name_length letters,
// digits or underscores (an underscore is a split digraph, "a_e").
[[nodiscard]] bool is_valid_name(std::string_view name) noexcept;

// Whether `name` can be a font's name. Not "-", which the store uses for none.
[[nodiscard]] bool is_valid_font_name(std::string_view name) noexcept;

}  // namespace wjs
