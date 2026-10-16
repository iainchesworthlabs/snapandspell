#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "wjs/board.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/game_rules.hpp"
#include "wjs/phase_data.hpp"
#include "wjs/seeding.hpp"
#include "wjs/solver.hpp"

// The game session of Solo Play (Mode B, the phonics board): the state machine
// of docs/design/game.md, from the box with its lid on to the resting screen.
// It is what the screens bind to. It deals a board, takes a traced path and
// answers it, keeps the finds, and reports when the board is cleared.
//
// It holds no Qt, draws nothing, plays nothing and reads no clock. Everything
// that varies is handed in: the seed of the deal (SessionOptions::seed), the
// moment of a find (Stamp, given to trace), and the pace of the presentation,
// which is the screens' (the shake, the drop and the celebration are events
// they report when they end). Given the same options and the same events it
// does the same, on every compiler. It persists nothing: record() and
// snapshot() hand the store plain values, and restore() takes one back.
//
// The words it names. A word is named only when it is one of the active phase's
// index (a find, the record, the snapshot): the dictionary is read to tell a
// real word from a string of letters, and the answer to a real word that is not
// today's is the kind and no word, so no word outside the index is ever handed
// out to show or to read. A real word is never answered "not a word".
//
// Single-threaded. The phase data and the dictionary are held by reference and
// must outlive the session (and any copy of it).

namespace wjs {

// ---------------------------------------------------------------------------
// The states, in the names of docs/design/game.md ("solo.box" and so on)

enum class SessionState {
    box,       // the lid is on; the board size is chosen here and the Shake button is pressed
    shaking,   // the board is dealt and the box rattles
    dropping,  // the lid lifts, the dice fall
    playing,   // the board and the panel: the child traces, taps, asks for the start tile
    cleared,   // the last viable word is found; one quiet celebration
    resting,   // the calm resting screen; Again is the only way on
};

// The id the design gives the screen: "solo.box", "solo.shaking", ... The
// design's `solo.word-found` is not a state: it is the few moments of
// presentation after a find, with the session already in `playing` or `cleared`.
[[nodiscard]] std::string_view to_string(SessionState state) noexcept;

// The board sizes of the first release. The value is the side.
enum class BoardSize : std::size_t {
    three = 3,
    four = 4,
};

[[nodiscard]] constexpr std::size_t side(BoardSize size) noexcept {
    return static_cast<std::size_t>(size);
}

// The size for a side of 3 or 4; nullopt for any other.
[[nodiscard]] constexpr std::optional<BoardSize> board_size_for(std::size_t side_length) noexcept {
    if (side_length == 3) {
        return BoardSize::three;
    }
    if (side_length == 4) {
        return BoardSize::four;
    }
    return std::nullopt;
}

// A note of when a find was made, in whatever unit the store wants (the day, the
// minute). The caller gives it with each trace; the session keeps it and gives it
// back, and never reads, compares or orders it.
using Stamp = std::int64_t;

// ---------------------------------------------------------------------------
// What the session hands out

// A viable word found on a board: one jewel in the tray.
struct Find {
    std::string word;       // upper-case; a word of the active phase's index
    bool assisted = false;  // the start tile had been shown for this word
    Stamp when = 0;

    friend bool operator==(const Find&, const Find&) = default;

    // Derived from the letter count and never stored (docs/design/phonics.md): a
    // word of a phase has three letters or more, so it always has a tier.
    [[nodiscard]] Tier tier() const noexcept {
        return tier_for_letters(word.size()).value_or(Tier::spark);
    }
    [[nodiscard]] Reward reward() const noexcept { return reward_for(tier()); }
};

enum class TraceKind {
    // Nothing happened: the session is not in `playing`, or the path is shorter
    // than two tiles (a touch to hear a letter, or a trace cancelled by lifting
    // outside the board). No cue, and nothing is recorded.
    ignored,
    // A word of today's index that was not found yet: a jewel, the stars of its
    // tier, the sound-it-out sequence.
    found,
    // A word of today's index already in the tray: no jewel and no stars. The
    // screens may sound it out again.
    found_again,
    // A real word that is not one of today's: the neutral cue, a soft tick, no
    // star, and no word (none is named, shown or read).
    real_not_todays,
    // Anything else: the path fades. Never a real word.
    not_a_word,
};

[[nodiscard]] std::string_view to_string(TraceKind kind) noexcept;

struct TraceOutcome {
    TraceKind kind = TraceKind::ignored;

    // For `found`, the find just recorded; for `found_again`, the one recorded
    // earlier. Empty for every other kind.
    std::optional<Find> find;

    // This trace found the last viable word, and `state()` is now `cleared`.
    bool cleared = false;

    // The stars this trace earned: the find's for `found`, otherwise none.
    [[nodiscard]] int stars() const noexcept {
        return kind == TraceKind::found && find ? find->reward().stars : 0;
    }

    // The find's tier carries a sticker. Whether it is the first time the word
    // has been found on this device is the store's to know, as it is for the
    // rules (Reward::sticker).
    [[nodiscard]] bool sticker() const noexcept {
        return kind == TraceKind::found && find && find->reward().sticker;
    }
};

// A board that reached `playing`, as the history and the dashboard keep it
// (docs/design/dashboard.md, "The store"): its letters, the words of the phase
// on it, and the finds with when and whether helped. Plain values, all of them
// strings, numbers and flags, so a store may write them in any format.
struct BoardRecord {
    std::string phase;  // the phase's name, "A"
    BoardSize size = BoardSize::three;

    // The tiles as Board::parse reads them: a space between tiles and a '/'
    // between rows, "S A T/P I N/B B B". A tile that is a dead jewel carries the
    // phase's dead letter (the first letter of A-Z that no word of the phase
    // uses), which the screens draw as a jewel.
    std::string tiles;

    // Every word of the phase on the board, upper-case, in alphabetical order:
    // what the board held, whether or not it was found. "Met" and "found" in the
    // dashboard are counted from this and `finds`.
    std::vector<std::string> viable;

    std::vector<Find> finds;  // in the order found, each word once

    friend bool operator==(const BoardRecord&, const BoardRecord&) = default;

    // Every viable word was found.
    [[nodiscard]] bool cleared() const noexcept {
        return !viable.empty() && finds.size() == viable.size();
    }

    [[nodiscard]] int stars() const noexcept {
        int total = 0;
        for (const Find& find : finds) {
            total += find.reward().stars;
        }
        return total;
    }
};

// What the store keeps to bring a session back (restore): the choice of size,
// where the deal's seed stream has got to, and the board in play. A session in
// `shaking` or `dropping` is saved as the box (the board was never seen);
// `cleared` is saved as `resting` (the celebration is not resumed).
//
// The targets of recent boards (which keep the next board from being made of
// the same words) are not saved: a restored session remembers none.
struct SessionSnapshot {
    SessionState state = SessionState::box;  // box, playing or resting
    BoardSize size = BoardSize::three;       // the size chosen on the box
    std::uint32_t seed = 1;                  // the seed the next board is dealt from

    // Set in `playing` and `resting`, empty in `box`.
    std::optional<BoardRecord> board;

    // The word whose start tile is lit, upper-case: set only in `playing`, and
    // only when the child asked for it and has not found that word yet.
    std::optional<std::string> lit;

    friend bool operator==(const SessionSnapshot&, const SessionSnapshot&) = default;
};

// What the deal of the last board cost, for tests and the performance tier. The
// design wants three to five viable words; the seeding lands that in a share of
// its tries, so the session seeds again, with the next seed of its stream, up to
// SessionOptions::max_seedings times, and keeps the board nearest the range.
struct DealReport {
    std::size_t seedings = 0;  // calls to seed_board, 1 to max_seedings
    std::size_t tries = 0;     // boards verified, over all of them
    std::size_t solves = 0;    // solves made, over all of them
    std::size_t viable = 0;    // words of the phase on the board dealt
    bool in_range = false;     // that is min_viable to max_viable
    std::uint32_t seed = 0;    // the seed of the seeding that made the board

    friend bool operator==(const DealReport&, const DealReport&) = default;
};

// ---------------------------------------------------------------------------
// Making a session

struct SessionOptions {
    // The size of the first board: the parent's default, 3x3 on a new device. The
    // app never moves a child from one size to another; the child chooses on the
    // box, and only the box.
    BoardSize size = BoardSize::three;

    // The seed of the deal. The first board is made from it, and each seeding
    // after it from the one after that, so one seed is a whole run of boards. The
    // app takes it from a random source; a test gives a fixed one.
    std::uint32_t seed = 1;

    // The parent's switch for the start-tile help. Off, request_help gives
    // nothing.
    bool help = true;

    // Sounds the parent has ticked, as the graphemes file writes them: the board
    // prefers targets with a grapheme that makes one (SeedOptions::focus_sounds).
    std::vector<std::string> focus_sounds;

    // What a board has to hold, counted in words of the phase on it.
    std::size_t min_viable = 3;
    std::size_t max_viable = 5;

    // Calls to seed_board for one board before the nearest is taken. Each is up
    // to 20 verifications (a solve each). Must be at least 1.
    std::size_t max_seedings = 5;
};

enum class SessionError {
    wrong_state,    // the event does not apply in this state; nothing changed
    bad_options,    // min_viable above max_viable, or no seedings
    bad_phase,      // the phase cannot make a board of 3x3 and 4x4 (it uses all 26 letters)
    no_board,       // shake: the phase has no word that fits the board, or the board holds none
    bad_snapshot,   // restore: the snapshot is not one a session makes
    wrong_phase,    // restore: the snapshot is of another phase than the one given
    board_changed,  // restore: the phase's words on the board are no longer the ones recorded
};

[[nodiscard]] std::string_view to_string(SessionError error) noexcept;

class GameSession {
public:
    // A session in `box`, with `options.size` chosen. The phase and the
    // dictionary are held by reference. `dictionary` is the English word list: it
    // is read to tell a real word from a string of letters, and never to name one.
    [[nodiscard]] static std::expected<GameSession, SessionError> create(
        const PhaseIndex& phase, const Dictionary& dictionary, const SessionOptions& options = {});

    // A session as the snapshot left it. `options.size` and `options.seed` are
    // ignored: the snapshot has both. The board is solved again against the
    // phase, so a snapshot of another phase is refused (wrong_phase), and so is a
    // board that no longer holds the words the record says (board_changed); a
    // snapshot that is not one snapshot() makes is bad_snapshot.
    [[nodiscard]] static std::expected<GameSession, SessionError> restore(
        const SessionSnapshot& snapshot, const PhaseIndex& phase, const Dictionary& dictionary,
        const SessionOptions& options = {});

    // --- what the screens draw ------------------------------------------------

    [[nodiscard]] SessionState state() const noexcept { return state_; }

    // The size chosen on the box, and the size of the board in play.
    [[nodiscard]] BoardSize size() const noexcept { return size_; }

    // The board on the table: null in `box`, where the lid is on. The board is
    // dealt at the shake, so the animation is handed a finished board.
    [[nodiscard]] const Board* board() const noexcept;

    // Tiles that carry no letter, drawn as jewels, in row order. Usually none:
    // the phonics board fills with letters outside the phase before it makes a
    // tile dead. Empty in `box`.
    [[nodiscard]] std::span<const Position> dead_tiles() const noexcept;

    // The tray: the words found on this board, in the order found. Empty in `box`.
    [[nodiscard]] std::span<const Find> finds() const noexcept;

    // The stars of this board so far.
    [[nodiscard]] int stars() const noexcept;

    // The tile lit by the start-tile help: set by request_help, cleared when its
    // word is found. Null otherwise.
    [[nodiscard]] std::optional<Position> lit_tile() const;

    // --- events ---------------------------------------------------------------
    // Each returns whether it applied. One that does not leaves everything as it
    // was: a screen may send an event late or twice.

    // `solo.box`: choose 3x3 or 4x4.
    bool choose_size(BoardSize size);

    // `solo.box` to `solo.shaking`: the Shake button, a device shake or Space.
    // The board is dealt here, before the shake begins, from the next seed of the
    // stream. An error leaves the session in `box`.
    [[nodiscard]] std::expected<void, SessionError> shake();

    // `solo.shaking` to `solo.dropping`: the shake has been long enough and is
    // over. How long is the screens' (docs/design/board.md).
    bool still();

    // `solo.dropping` to `solo.playing`: the dice have landed and the panel is in.
    bool landed();

    // In `solo.playing`: a traced or tapped path, with the moment it was made.
    // The last viable word moves the session to `cleared`. Anything in any
    // other state is `ignored`.
    TraceOutcome trace(std::span<const Position> path, Stamp when);

    // In `solo.playing`: the start-tile help (docs/design/game.md, "Help"). The
    // tile of the shortest viable word not yet found, nearest the top left; the
    // same one until that word is found. Null when help is off, the session is
    // not in `playing`, or no word is left. The word is not handed out: the
    // child does the finding. A word found while its tile is lit is an assisted
    // find.
    std::optional<Position> request_help();

    // `solo.cleared` to `solo.resting`: the celebration is over (about 3 s on the
    // screen).
    bool rest();

    // `solo.resting` to `solo.box`: Again.
    bool again();

    // The back control: puts the lid back on. `solo.shaking`, `solo.dropping` and
    // `solo.playing` to `solo.box`. A board that was reached in `playing` is kept
    // in record() until the next shake, so it can be saved either way. From
    // `cleared` and `resting` the screens leave Solo Play without the session.
    bool back();

    // --- what the store keeps -------------------------------------------------

    // The board dealt by the last shake, if it reached `playing`: in play, ended or
    // left. Null before the first, and after a shake that was backed out of
    // before the dice landed.
    [[nodiscard]] std::optional<BoardRecord> record() const;

    [[nodiscard]] SessionSnapshot snapshot() const;

    // How the last board was dealt. Empty before the first shake and after a
    // restore.
    [[nodiscard]] const std::optional<DealReport>& last_deal() const noexcept { return report_; }

private:
    GameSession(const PhaseIndex& phase, const Dictionary& dictionary, SessionOptions options);

    [[nodiscard]] BoardRecord make_record() const;
    [[nodiscard]] const Find* find_of(std::string_view word) const;

    const PhaseIndex* phase_;
    const Dictionary* dictionary_;
    SessionOptions options_;

    SessionState state_ = SessionState::box;
    BoardSize size_;
    std::uint32_t seed_;
    RecentTargets recent_;

    // The board dealt by the last shake, kept until the next so that record() can
    // outlive the lid.
    std::optional<Board> board_;
    std::vector<Position> dead_;
    std::vector<FoundWord> viable_;
    FoundWords found_;
    std::vector<Find> finds_;
    int stars_ = 0;
    std::optional<std::string> lit_;
    bool played_ = false;
    std::optional<DealReport> report_;
};

}  // namespace wjs
