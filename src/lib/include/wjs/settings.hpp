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

// The light/dark chrome. System follows the operating system's setting; Light
// and Dark are the parent's choice for the whole program (docs/design/
// application.md, "Colour and contrast"). The board itself is dark in every
// scheme.
enum class ColourScheme {
    system,
    light,
    dark,
};

// Each enum is written in the store and in the screens by these names, and
// read back by from_string, which is empty for a name it does not know.
[[nodiscard]] std::string_view to_string(BoardMode value) noexcept;
[[nodiscard]] std::string_view to_string(SoundItOut value) noexcept;
[[nodiscard]] std::string_view to_string(CelebrationSounds value) noexcept;
[[nodiscard]] std::string_view to_string(MotionSetting value) noexcept;
[[nodiscard]] std::string_view to_string(ColourScheme value) noexcept;

[[nodiscard]] std::optional<BoardMode> board_mode_from_string(std::string_view name) noexcept;
[[nodiscard]] std::optional<SoundItOut> sound_it_out_from_string(std::string_view name) noexcept;
[[nodiscard]] std::optional<CelebrationSounds> celebration_sounds_from_string(
    std::string_view name) noexcept;
[[nodiscard]] std::optional<MotionSetting> motion_from_string(std::string_view name) noexcept;
[[nodiscard]] std::optional<ColourScheme> colour_scheme_from_string(std::string_view name) noexcept;

// The languages the interface is offered in, as the codes the store and the
// screens use: English, French, Spanish, German, Yiddish, Hebrew, Chinese and
// Japanese (docs/audit-0.1.md, "Languages, RTL and i18n"). The board's letters,
// the word list and the phonics data are not translated; the chrome is.
inline constexpr std::string_view languages[] = {"en", "fr", "es", "de", "yi", "he", "zh", "ja"};

// Whether `code` is one of the languages above. Empty is not: it is the system's
// language, which only the screens know.
[[nodiscard]] bool is_valid_language(std::string_view code) noexcept;

// The board sizes the parent area offers as a default. The standard board plays
// 3x3 to 6x6 and the child picks on the box; the parent's default is one of the
// two sizes the phonics board is designed for (docs/design/game.md).
inline constexpr int smallest_default_board_size = 3;
inline constexpr int largest_default_board_size = 4;
// The largest size the standard board is played at (its dice, data/dice).
inline constexpr int largest_standard_board_size = 6;

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
    // used. 3x3 to 6x6 on the standard board, 3x3 and 4x4 on the phonics board
    // (normalized() holds it to the board's).
    int board_size = 3;

    SoundItOut sound_it_out = SoundItOut::full;
    CelebrationSounds celebration_sounds = CelebrationSounds::full;
    bool sound = true;
    bool vibration = true;
    bool shake = true;  // shake the device to roll
    MotionSetting motion = MotionSetting::system;

    // The light/dark chrome: the system's setting, or the parent's choice.
    ColourScheme colour_scheme = ColourScheme::system;

    // 100, 125 or 150: a percentage of the system's text size.
    int text_size_percent = 100;

    // A school font's name, or empty for none. Only a font whose licence
    // permits bundling can ever be named here, and none is bundled yet.
    std::string school_font;

    // The language of the interface, one of `languages`, or empty for the
    // device's own when it is one of them and English otherwise. The board and
    // the words on it are never this: the game is an English word game.
    std::string language;

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
