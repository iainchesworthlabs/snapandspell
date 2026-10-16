#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "wjs/board.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/game_session.hpp"
#include "wjs/phase_data.hpp"
#include "wjs/solver.hpp"

// TEST DATA. Two phases written for these tests (the seven and four words the
// seeding tests use, with placeholder sounds) and a dictionary that holds them
// and a few real words that are not in either. None of it is a phase of the
// application or content for a child; the real phase files have to be written
// and reviewed by a person qualified to do it.
//
// Most cases play a board written out here, brought in through restore(): its
// words and paths are known without asking the session or the solver. The
// cases about dealing use the session's own boards.

namespace {

using wjs::BoardSize;
using wjs::Position;
using wjs::SessionState;
using wjs::TraceKind;

constexpr std::string_view graphemes_a =
    "# phase: A\n"
    "a  /t1/  clip:t1\n"
    "i  /t2/  clip:t2\n"
    "n  /t3/  clip:t3\n"
    "p  /t4/  clip:t4\n"
    "s  /t5/  clip:t5\n"
    "t  /t6/  clip:t6\n";
constexpr std::string_view graphemes_b = "# phase: B\nai  /t7/  clip:t7\n";
constexpr std::string_view words_a =
    "# phase: A\n"
    "nip  n i p\n"
    "pan  p a n\n"
    "pat  p a t\n"
    "pin  p i n\n"
    "sat  s a t\n"
    "sit  s i t\n"
    "tap  t a p\n";
constexpr std::string_view words_b =
    "# phase: B\n"
    "pain  p ai n\n"
    "paint  p ai n t\n"
    "saint  s ai n t\n"
    "stain  s t ai n\n";

// Real words of the dictionary that no phase has. `tip` stands for any word a
// child must never be shown: it is in the dictionary, and so must never be named.
constexpr std::array<std::string_view, 9> extra_words = {"tip", "pit", "nap", "ant", "tan",
                                                         "at",  "in",  "it",  "tab"};

const wjs::Dictionary& english() {
    static const wjs::Dictionary dictionary = [] {
        wjs::Dictionary words;
        for (const std::string_view word :
             {"nip", "pan", "pat", "pin", "sat", "sit", "tap", "pain", "paint", "saint", "stain"}) {
            words.insert(word);
        }
        for (const std::string_view word : extra_words) {
            words.insert(word);
        }
        return words;
    }();
    return dictionary;
}

const wjs::PhaseSet& phases() {
    static const wjs::PhaseSet set = [] {
        const auto graphemes = [](std::string_view text, std::string_view name) {
            std::istringstream in{std::string(text)};
            auto parsed = wjs::parse_graphemes(in, name);
            REQUIRE(parsed.has_value());
            return *parsed;
        };
        const auto words = [](std::string_view text, std::string_view name) {
            std::istringstream in{std::string(text)};
            auto parsed = wjs::parse_words(in, name);
            REQUIRE(parsed.has_value());
            return *parsed;
        };
        auto built =
            wjs::PhaseSet::build({graphemes(graphemes_a, "a.g"), graphemes(graphemes_b, "b.g")},
                                 {words(words_a, "a.w"), words(words_b, "b.w")}, english());
        REQUIRE(built.has_value());
        return *std::move(built);
    }();
    return set;
}

const wjs::PhaseIndex& phase_a() {
    return *phases().find("A");
}
const wjs::PhaseIndex& phase_b() {
    return *phases().find("B");
}

wjs::GameSession make(const wjs::PhaseIndex& phase, const wjs::SessionOptions& options = {}) {
    auto created = wjs::GameSession::create(phase, english(), options);
    REQUIRE(created.has_value());
    return *std::move(created);
}

wjs::GameSession make(const wjs::SessionOptions& options = {}) {
    return make(phase_a(), options);
}

using Path = std::vector<Position>;

// The board of the cases below, 3 by 3, in phase A; `B` is the phase's dead
// letter (the first letter of A-Z that no word uses). Its five words and one path
// for each, by hand:
//
//     P A T      PAT  (0,0) (0,1) (0,2)      TAP  (0,2) (0,1) (0,0)
//     B I N      PIN  (0,0) (1,1) (1,2)      NIP  (1,2) (1,1) (0,0)
//     B B B      PAN  (0,0) (0,1) (1,2)
constexpr std::string_view hand_tiles = "P A T/B I N/B B B";
const Path pat{{0, 0}, {0, 1}, {0, 2}};
const Path tap{{0, 2}, {0, 1}, {0, 0}};
const Path pin{{0, 0}, {1, 1}, {1, 2}};
const Path nip{{1, 2}, {1, 1}, {0, 0}};
const Path pan{{0, 0}, {0, 1}, {1, 2}};
// Real words of the dictionary that are not today's, and some that are not words.
const Path tip{{0, 2}, {1, 1}, {0, 0}};  // TIP
const Path at{{0, 1}, {0, 2}};           // AT, two letters
const Path pab{{0, 0}, {0, 1}, {1, 0}};  // PAB, no word
const Path apart{{0, 0}, {0, 2}};        // not touching: not a path

wjs::BoardRecord hand_record(std::vector<wjs::Find> finds = {}) {
    wjs::BoardRecord record;
    record.phase = "A";
    record.size = BoardSize::three;
    record.tiles = std::string(hand_tiles);
    record.viable = {"NIP", "PAN", "PAT", "PIN", "TAP"};
    record.finds = std::move(finds);
    return record;
}

wjs::SessionSnapshot playing_snapshot(std::vector<wjs::Find> finds = {}) {
    wjs::SessionSnapshot snapshot;
    snapshot.state = SessionState::playing;
    snapshot.size = BoardSize::three;
    snapshot.seed = 77;
    snapshot.board = hand_record(std::move(finds));
    return snapshot;
}

wjs::GameSession restore(const wjs::SessionSnapshot& snapshot,
                         const wjs::SessionOptions& options = {}) {
    auto restored = wjs::GameSession::restore(snapshot, phase_a(), english(), options);
    REQUIRE(restored.has_value());
    return *std::move(restored);
}

// A session playing the hand board.
wjs::GameSession hand_session(const wjs::SessionOptions& options = {}) {
    return restore(playing_snapshot(), options);
}

// A session that has dealt a board of its own and is in `state`.
wjs::GameSession reach(SessionState state, const wjs::SessionOptions& options = {}) {
    wjs::GameSession session = make(options);
    if (state == SessionState::box) {
        return session;
    }
    REQUIRE(session.shake().has_value());
    if (state == SessionState::shaking) {
        return session;
    }
    REQUIRE(session.still());
    if (state == SessionState::dropping) {
        return session;
    }
    REQUIRE(session.landed());
    if (state == SessionState::playing) {
        return session;
    }
    // Every word the solver finds on the board, traced along its path.
    for (const wjs::FoundWord& word : wjs::solve(*session.board(), phase_a().dictionary())) {
        session.trace(word.path, 0);
    }
    REQUIRE(session.state() == SessionState::cleared);
    if (state == SessionState::cleared) {
        return session;
    }
    REQUIRE(session.rest());
    REQUIRE(state == SessionState::resting);
    return session;
}

wjs::Find jewel(std::string word, wjs::Stamp when = 0) {
    return {.word = std::move(word), .assisted = false, .when = when};
}

std::vector<std::string> words_of(std::span<const wjs::Find> finds) {
    std::vector<std::string> words;
    for (const wjs::Find& find : finds) {
        words.push_back(find.word);
    }
    return words;
}

}  // namespace

// --- naming ------------------------------------------------------------------

TEST_CASE("the states are named as the design names its screens", "[session]") {
    CHECK(wjs::to_string(SessionState::box) == "solo.box");
    CHECK(wjs::to_string(SessionState::shaking) == "solo.shaking");
    CHECK(wjs::to_string(SessionState::dropping) == "solo.dropping");
    CHECK(wjs::to_string(SessionState::playing) == "solo.playing");
    CHECK(wjs::to_string(SessionState::cleared) == "solo.cleared");
    CHECK(wjs::to_string(SessionState::resting) == "solo.resting");
}

TEST_CASE("only the sizes of the first release have a name", "[session]") {
    CHECK(wjs::board_size_for(3) == BoardSize::three);
    CHECK(wjs::board_size_for(4) == BoardSize::four);
    for (const std::size_t other : {0U, 1U, 2U, 5U, 6U, 16U}) {
        CHECK_FALSE(wjs::board_size_for(other).has_value());
    }
    CHECK(wjs::side(BoardSize::four) == 4U);
}

// --- making a session --------------------------------------------------------

TEST_CASE("a new session is the closed box, with nothing on the table", "[session]") {
    const wjs::GameSession session = make();
    CHECK(session.state() == SessionState::box);
    CHECK(session.size() == BoardSize::three);
    CHECK(session.board() == nullptr);
    CHECK(session.dead_tiles().empty());
    CHECK(session.finds().empty());
    CHECK(session.stars() == 0);
    CHECK_FALSE(session.lit_tile().has_value());
    CHECK_FALSE(session.record().has_value());
    CHECK_FALSE(session.last_deal().has_value());
}

TEST_CASE("the first board's size is the option's", "[session]") {
    wjs::SessionOptions options;
    options.size = BoardSize::four;
    CHECK(make(options).size() == BoardSize::four);
}

TEST_CASE("options that cannot make a board are refused", "[session]") {
    wjs::SessionOptions inverted;
    inverted.min_viable = 5;
    inverted.max_viable = 3;
    wjs::SessionOptions none;
    none.max_seedings = 0;
    wjs::SessionOptions empty_range;
    empty_range.min_viable = 0;
    empty_range.max_viable = 0;
    for (const wjs::SessionOptions& options : {inverted, none, empty_range}) {
        const auto created = wjs::GameSession::create(phase_a(), english(), options);
        REQUIRE_FALSE(created.has_value());
        CHECK(created.error() == wjs::SessionError::bad_options);
    }
}

TEST_CASE("a phase that uses every letter has none to leave out, and is refused", "[session]") {
    // Two words that between them spell the alphabet: no letter is left for a
    // dead jewel. TEST DATA, with a dictionary of its own.
    std::string graphemes = "# phase: A\n";
    for (char letter = 'a'; letter <= 'z'; ++letter) {
        graphemes += std::string(1, letter) + "  /x/  clip:" + std::string(1, letter) + "\n";
    }
    const std::string words =
        "# phase: A\n"
        "abcdefghijklm  a b c d e f g h i j k l m\n"
        "nopqrstuvwxyz  n o p q r s t u v w x y z\n";
    wjs::Dictionary dictionary;
    dictionary.insert("abcdefghijklm");
    dictionary.insert("nopqrstuvwxyz");
    std::istringstream graphemes_in{graphemes};
    std::istringstream words_in{words};
    const auto built = wjs::PhaseSet::build({*wjs::parse_graphemes(graphemes_in, "g")},
                                            {*wjs::parse_words(words_in, "w")}, dictionary);
    REQUIRE(built.has_value());

    const auto created = wjs::GameSession::create(*built->find("A"), dictionary);
    REQUIRE_FALSE(created.has_value());
    CHECK(created.error() == wjs::SessionError::bad_phase);
}

TEST_CASE("a phase with no word of three letters or fewer deals no board and stays in the box",
          "[session]") {
    // The phonics board plants words of three letters at most. TEST DATA.
    const std::string graphemes =
        "# phase: A\na  /x/  clip:a\nb  /x/  clip:b\nc  /x/  clip:c\n"
        "d  /x/  clip:d\n";
    const std::string words = "# phase: A\nabcd  a b c d\n";
    wjs::Dictionary dictionary;
    dictionary.insert("abcd");
    std::istringstream graphemes_in{graphemes};
    std::istringstream words_in{words};
    const auto built = wjs::PhaseSet::build({*wjs::parse_graphemes(graphemes_in, "g")},
                                            {*wjs::parse_words(words_in, "w")}, dictionary);
    REQUIRE(built.has_value());

    auto created = wjs::GameSession::create(*built->find("A"), dictionary);
    REQUIRE(created.has_value());
    const auto shaken = created->shake();
    REQUIRE_FALSE(shaken.has_value());
    CHECK(shaken.error() == wjs::SessionError::no_board);
    CHECK(created->state() == SessionState::box);
    CHECK(created->board() == nullptr);
    CHECK_FALSE(created->record().has_value());
}

// --- every transition --------------------------------------------------------

TEST_CASE("the box leads to shaking, dropping and playing, one event each", "[session]") {
    wjs::GameSession session = make();
    REQUIRE(session.shake().has_value());
    CHECK(session.state() == SessionState::shaking);
    CHECK(session.still());
    CHECK(session.state() == SessionState::dropping);
    CHECK(session.landed());
    CHECK(session.state() == SessionState::playing);
}

TEST_CASE("the board is dealt at the shake, before anything moves", "[session]") {
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        wjs::SessionOptions options;
        options.size = size;
        wjs::GameSession session = make(options);
        CHECK(session.board() == nullptr);
        REQUIRE(session.shake().has_value());
        // In `shaking` already, a finished board.
        REQUIRE(session.board() != nullptr);
        CHECK(session.board()->rows() == wjs::side(size));
        CHECK(session.board()->columns() == wjs::side(size));
        CHECK(session.last_deal().has_value());
    }
}

TEST_CASE("each event applies in its own state and in no other", "[session]") {
    enum class Event { choose_size, shake, still, landed, trace, request_help, rest, again, back };
    struct Row {
        Event event;
        SessionState from;
        SessionState to;
    };
    // The design's flow (docs/design/game.md), one row for each edge. `trace`
    // finds a word, which leaves the state in `playing` on a board with several.
    const std::vector<Row> edges = {
        {Event::choose_size, SessionState::box, SessionState::box},
        {Event::shake, SessionState::box, SessionState::shaking},
        {Event::still, SessionState::shaking, SessionState::dropping},
        {Event::landed, SessionState::dropping, SessionState::playing},
        {Event::trace, SessionState::playing, SessionState::playing},
        {Event::request_help, SessionState::playing, SessionState::playing},
        {Event::rest, SessionState::cleared, SessionState::resting},
        {Event::again, SessionState::resting, SessionState::box},
        {Event::back, SessionState::shaking, SessionState::box},
        {Event::back, SessionState::dropping, SessionState::box},
        {Event::back, SessionState::playing, SessionState::box},
    };
    const auto applies = [&](Event event, SessionState state) {
        return std::ranges::any_of(
            edges, [&](const Row& row) { return row.event == event && row.from == state; });
    };
    const auto next = [&](Event event, SessionState state) {
        for (const Row& row : edges) {
            if (row.event == event && row.from == state) {
                return row.to;
            }
        }
        return state;
    };
    const auto send = [](wjs::GameSession& session, Event event) {
        switch (event) {
            case Event::choose_size:
                return session.choose_size(BoardSize::four);
            case Event::shake:
                return session.shake().has_value();
            case Event::still:
                return session.still();
            case Event::landed:
                return session.landed();
            case Event::trace: {
                // The first word of the board, along its path.
                const auto words = wjs::solve(*session.board(), phase_a().dictionary());
                return session.trace(words.front().path, 1).kind == TraceKind::found;
            }
            case Event::request_help:
                return session.request_help().has_value();
            case Event::rest:
                return session.rest();
            case Event::again:
                return session.again();
            case Event::back:
                return session.back();
        }
        return false;
    };

    constexpr std::array all_events = {Event::choose_size, Event::shake, Event::still,
                                       Event::landed,      Event::trace, Event::request_help,
                                       Event::rest,        Event::again, Event::back};
    for (const SessionState state :
         {SessionState::box, SessionState::shaking, SessionState::dropping, SessionState::playing,
          SessionState::cleared, SessionState::resting}) {
        for (const Event event : all_events) {
            INFO("state " << wjs::to_string(state) << ", event " << static_cast<int>(event));
            wjs::GameSession session = reach(state);
            // `trace` needs a board: in the box there is none to ask the solver
            // about, and the event is not one the box takes.
            if (event == Event::trace && state == SessionState::box) {
                CHECK(session.trace(pat, 1).kind == TraceKind::ignored);
                CHECK(session.state() == SessionState::box);
                continue;
            }
            const auto before = session.snapshot();
            const bool applied = send(session, event);
            CHECK(applied == applies(event, state));
            CHECK(session.state() == next(event, state));
            if (!applied) {
                // A refused event changes nothing.
                CHECK(session.snapshot() == before);
            }
        }
    }
}

TEST_CASE("the size is chosen on the box and nowhere else", "[session]") {
    wjs::GameSession session = make();
    CHECK(session.choose_size(BoardSize::four));
    CHECK(session.size() == BoardSize::four);
    CHECK(session.choose_size(BoardSize::three));
    CHECK(session.size() == BoardSize::three);

    REQUIRE(session.shake().has_value());
    CHECK_FALSE(session.choose_size(BoardSize::four));
    CHECK(session.size() == BoardSize::three);
    CHECK(session.board()->rows() == 3U);
}

TEST_CASE("a shake out of the box is a refused event, not a new deal", "[session]") {
    wjs::GameSession session = reach(SessionState::playing);
    const auto before = session.snapshot();
    const auto shaken = session.shake();
    REQUIRE_FALSE(shaken.has_value());
    CHECK(shaken.error() == wjs::SessionError::wrong_state);
    CHECK(session.snapshot() == before);
}

TEST_CASE("the back control puts the lid back on from shaking, dropping and playing", "[session]") {
    for (const SessionState state :
         {SessionState::shaking, SessionState::dropping, SessionState::playing}) {
        wjs::GameSession session = reach(state);
        CHECK(session.back());
        CHECK(session.state() == SessionState::box);
        CHECK(session.board() == nullptr);
        CHECK(session.finds().empty());
        CHECK(session.stars() == 0);
        // The box can be shaken again.
        CHECK(session.shake().has_value());
    }
}

TEST_CASE("again puts the lid back on and the next shake deals another board", "[session]") {
    wjs::GameSession session = reach(SessionState::resting);
    const std::string first = session.record()->tiles;
    REQUIRE(session.again());
    CHECK(session.state() == SessionState::box);
    CHECK(session.board() == nullptr);
    REQUIRE(session.shake().has_value());
    REQUIRE(session.still());
    REQUIRE(session.landed());
    CHECK(session.finds().empty());
    CHECK(session.stars() == 0);
    // A new board, new tray: the stream has moved on.
    CHECK(session.record()->finds.empty());
    CHECK(session.record()->tiles != first);
}

// --- tracing -----------------------------------------------------------------

TEST_CASE("a viable path is a find, with the stars of its tier", "[session]") {
    wjs::GameSession session = hand_session();
    const wjs::TraceOutcome outcome = session.trace(pat, 4242);
    CHECK(outcome.kind == TraceKind::found);
    REQUIRE(outcome.find.has_value());
    CHECK(outcome.find->word == "PAT");
    CHECK(outcome.find->when == 4242);
    CHECK_FALSE(outcome.find->assisted);
    CHECK(outcome.find->tier() == wjs::Tier::spark);
    CHECK(outcome.stars() == 1);
    CHECK_FALSE(outcome.sticker());
    CHECK_FALSE(outcome.cleared);
    CHECK(session.state() == SessionState::playing);
    CHECK(session.stars() == 1);
    CHECK(words_of(session.finds()) == std::vector<std::string>{"PAT"});
}

TEST_CASE("a word read the other way round is the other word", "[session]") {
    wjs::GameSession session = hand_session();
    CHECK(session.trace(pat, 0).find->word == "PAT");
    CHECK(session.trace(tap, 0).find->word == "TAP");
}

TEST_CASE("a word found again is not a new jewel and earns nothing", "[session]") {
    wjs::GameSession session = hand_session();
    REQUIRE(session.trace(pat, 10).kind == TraceKind::found);
    const wjs::TraceOutcome again = session.trace(pat, 20);
    CHECK(again.kind == TraceKind::found_again);
    REQUIRE(again.find.has_value());
    CHECK(again.find->word == "PAT");
    CHECK(again.find->when == 10);  // the find as it was recorded
    CHECK(again.stars() == 0);
    CHECK_FALSE(again.sticker());
    CHECK(session.stars() == 1);
    CHECK(session.finds().size() == 1U);
}

TEST_CASE("a real word that is not today's gets the neutral cue and no word", "[session]") {
    wjs::GameSession session = hand_session();
    for (const Path* path : {&tip, &at}) {
        const wjs::TraceOutcome outcome = session.trace(*path, 5);
        CHECK(outcome.kind == TraceKind::real_not_todays);
        CHECK_FALSE(outcome.find.has_value());
        CHECK(outcome.stars() == 0);
        CHECK_FALSE(outcome.cleared);
    }
    // Nothing was recorded.
    CHECK(session.finds().empty());
    CHECK(session.stars() == 0);
    CHECK(session.state() == SessionState::playing);
}

TEST_CASE("a real word of two letters is a real word, not 'not a word'", "[session]") {
    // The rules call it not a word because it cannot count (a word under three
    // letters has no tier). The session does not: a child who is told a real
    // word is not one has been taught something false.
    wjs::GameSession session = hand_session();
    CHECK(session.trace(at, 0).kind == TraceKind::real_not_todays);
}

TEST_CASE("anything else fades: not a word, with no word", "[session]") {
    wjs::GameSession session = hand_session();
    for (const Path* path : {&pab, &apart}) {
        const wjs::TraceOutcome outcome = session.trace(*path, 5);
        CHECK(outcome.kind == TraceKind::not_a_word);
        CHECK_FALSE(outcome.find.has_value());
        CHECK(outcome.stars() == 0);
    }
    CHECK(session.finds().empty());
}

TEST_CASE("a path that is not a path is not a word, even if its letters were one", "[session]") {
    wjs::GameSession session = hand_session();
    const Path repeated{{0, 0}, {0, 1}, {0, 0}};           // PAP, a tile twice
    const Path off_board{{0, 0}, {0, 1}, {0, 2}, {0, 3}};  // a tile off the board
    CHECK(session.trace(repeated, 0).kind == TraceKind::not_a_word);
    CHECK(session.trace(off_board, 0).kind == TraceKind::not_a_word);
}

TEST_CASE("a touch on one tile, or nothing, is not a word attempt", "[session]") {
    wjs::GameSession session = hand_session();
    const Path one{{0, 1}};  // A
    for (const Path& path : {Path{}, one}) {
        const wjs::TraceOutcome outcome = session.trace(path, 0);
        CHECK(outcome.kind == TraceKind::ignored);
        CHECK_FALSE(outcome.find.has_value());
    }
}

TEST_CASE("a trace outside `playing` is ignored", "[session]") {
    for (const SessionState state :
         {SessionState::box, SessionState::shaking, SessionState::dropping, SessionState::cleared,
          SessionState::resting}) {
        wjs::GameSession session = reach(state);
        const auto before = session.snapshot();
        const Path along = state == SessionState::box
                               ? pat
                               : wjs::solve(*session.board(), phase_a().dictionary())[0].path;
        CHECK(session.trace(along, 0).kind == TraceKind::ignored);
        CHECK(session.snapshot() == before);
    }
}

TEST_CASE("the tiers pay one, two and five stars and a Power word carries a sticker", "[session]") {
    // Phase B on a board with words of three, four and five letters. 3 by 3,
    // with `B` for the dead letter:
    //
    //     P A I      PAN, PAT, SAT, TAP      three letters
    //     S T N      PAIN                    four letters
    //     B B B      PAINT, SAINT, STAIN     five letters
    wjs::SessionSnapshot snapshot;
    snapshot.state = SessionState::playing;
    wjs::BoardRecord record;
    record.phase = "B";
    record.tiles = "P A I/S T N/B B B";
    record.viable = {"PAIN", "PAINT", "PAN", "PAT", "SAINT", "SAT", "STAIN", "TAP"};
    snapshot.board = record;
    auto restored = wjs::GameSession::restore(snapshot, phase_b(), english());
    REQUIRE(restored.has_value());
    wjs::GameSession& session = *restored;

    const Path pain{{0, 0}, {0, 1}, {0, 2}, {1, 2}};
    const Path paint{{0, 0}, {0, 1}, {0, 2}, {1, 2}, {1, 1}};
    const Path pan_here{{0, 0}, {0, 1}, {1, 2}};

    const auto spark = session.trace(pan_here, 0);
    CHECK(spark.find->tier() == wjs::Tier::spark);
    CHECK(spark.stars() == 1);
    CHECK_FALSE(spark.sticker());

    const auto power = session.trace(pain, 0);
    CHECK(power.find->word == "PAIN");
    CHECK(power.find->tier() == wjs::Tier::power);
    CHECK(power.stars() == 2);
    CHECK(power.sticker());

    const auto mega = session.trace(paint, 0);
    CHECK(mega.find->word == "PAINT");
    CHECK(mega.find->tier() == wjs::Tier::mega);
    CHECK(mega.stars() == 5);
    CHECK_FALSE(mega.sticker());

    CHECK(session.stars() == 1 + 2 + 5);
}

// --- clearing ----------------------------------------------------------------

TEST_CASE("the last viable word clears the board, and nothing before it does", "[session]") {
    wjs::GameSession session = hand_session();
    for (const Path* path : {&pat, &tap, &pin, &nip}) {
        const wjs::TraceOutcome outcome = session.trace(*path, 0);
        CHECK(outcome.kind == TraceKind::found);
        CHECK_FALSE(outcome.cleared);
        CHECK(session.state() == SessionState::playing);
    }
    const wjs::TraceOutcome last = session.trace(pan, 0);
    CHECK(last.kind == TraceKind::found);
    CHECK(last.cleared);
    CHECK(session.state() == SessionState::cleared);
    CHECK(session.finds().size() == 5U);
    CHECK(session.stars() == 5);
    REQUIRE(session.record().has_value());
    CHECK(session.record()->cleared());
}

TEST_CASE("after the last word nothing is traced, and the celebration then the rest follow",
          "[session]") {
    wjs::GameSession session = hand_session();
    for (const Path* path : {&pat, &tap, &pin, &nip, &pan}) {
        session.trace(*path, 0);
    }
    REQUIRE(session.state() == SessionState::cleared);
    CHECK(session.trace(pat, 0).kind == TraceKind::ignored);
    CHECK_FALSE(session.request_help().has_value());
    CHECK_FALSE(session.back());  // the screens leave Solo Play without the session
    CHECK_FALSE(session.again());
    CHECK(session.rest());
    CHECK(session.state() == SessionState::resting);
    CHECK(session.trace(pat, 0).kind == TraceKind::ignored);
    CHECK_FALSE(session.rest());
    CHECK_FALSE(session.back());
    CHECK(session.again());
    CHECK(session.state() == SessionState::box);
}

TEST_CASE("a board dealt by the session can be played to its end by following the solver",
          "[session]") {
    // The solver is what says which words the board holds; the session has to
    // agree with it, word for word, on a board it dealt itself.
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        for (std::uint32_t seed = 1; seed <= 25; ++seed) {
            wjs::SessionOptions options;
            options.size = size;
            options.seed = seed;
            wjs::GameSession session = make(options);
            REQUIRE(session.shake().has_value());
            REQUIRE(session.still());
            REQUIRE(session.landed());

            const auto words = wjs::solve(*session.board(), phase_a().dictionary());
            REQUIRE_FALSE(words.empty());
            std::size_t found = 0;
            for (const wjs::FoundWord& word : words) {
                INFO("size " << wjs::side(size) << ", seed " << seed << ", word " << word.word);
                CHECK(session.state() == SessionState::playing);
                const wjs::TraceOutcome outcome = session.trace(word.path, 0);
                CHECK(outcome.kind == TraceKind::found);
                ++found;
                CHECK(outcome.cleared == (found == words.size()));
            }
            CHECK(session.state() == SessionState::cleared);
            CHECK(session.finds().size() == words.size());
        }
    }
}

// --- the help ----------------------------------------------------------------

TEST_CASE("the help lights the start tile of the shortest word nearest the top left", "[session]") {
    wjs::GameSession session = hand_session();
    // All five words are three letters. The nearest first tile to the top left
    // is (0,0): PAN, PAT, PIN start there; the word that sorts first is PAN. The
    // lit tile is that word's first tile.
    const auto tile = session.request_help();
    REQUIRE(tile.has_value());
    CHECK(*tile == Position{0, 0});
    CHECK(session.lit_tile() == tile);
}

TEST_CASE("the same press gives the same tile until its word is found", "[session]") {
    wjs::GameSession session = hand_session();
    const auto first = session.request_help();
    REQUIRE(first.has_value());
    CHECK(session.request_help() == first);
    // Finding another word does not move it.
    REQUIRE(session.trace(tap, 0).kind == TraceKind::found);
    CHECK(session.lit_tile() == first);
    CHECK(session.request_help() == first);
}

TEST_CASE("a word found while its tile is lit is an assisted find, and the light goes out",
          "[session]") {
    wjs::GameSession session = hand_session();
    REQUIRE(session.request_help().has_value());  // PAN, from (0,0)
    const wjs::TraceOutcome helped = session.trace(pan, 0);
    REQUIRE(helped.find.has_value());
    CHECK(helped.find->word == "PAN");
    CHECK(helped.find->assisted);
    CHECK_FALSE(session.lit_tile().has_value());

    // The next press names the next word, and a word found without it is not
    // assisted.
    REQUIRE(session.request_help().has_value());  // PAT
    const wjs::TraceOutcome unhelped = session.trace(tap, 0);
    CHECK_FALSE(unhelped.find->assisted);
    REQUIRE(session.record().has_value());
    CHECK(session.record()->finds[0].assisted);
    CHECK_FALSE(session.record()->finds[1].assisted);
}

TEST_CASE("the help answers with a tile and never a word", "[session]") {
    // The only thing the session hands out about the help is a position: the
    // child does the finding. The word it is for is in the snapshot, because
    // the light has to come back with the board.
    wjs::GameSession session = hand_session();
    REQUIRE(session.request_help().has_value());
    const auto snapshot = session.snapshot();
    REQUIRE(snapshot.lit.has_value());
    CHECK(*snapshot.lit == "PAN");
}

TEST_CASE("with the parent's switch off the help gives nothing", "[session]") {
    wjs::SessionOptions options;
    options.help = false;
    wjs::GameSession session = hand_session(options);
    CHECK_FALSE(session.request_help().has_value());
    CHECK_FALSE(session.lit_tile().has_value());
}

TEST_CASE("the help follows every word of the board to its end", "[session]") {
    // Ask, trace the word the light is for (the snapshot names it), and ask
    // again: each word is lit once, found assisted, and the board clears.
    const std::map<std::string, const Path*> path_of = {
        {"NIP", &nip}, {"PAN", &pan}, {"PAT", &pat}, {"PIN", &pin}, {"TAP", &tap}};
    wjs::GameSession session = hand_session();
    std::vector<std::string> order;
    while (session.request_help()) {
        const std::string word = *session.snapshot().lit;
        order.push_back(word);
        const wjs::TraceOutcome outcome = session.trace(*path_of.at(word), 0);
        CHECK(outcome.kind == TraceKind::found);
        CHECK(outcome.find->assisted);
    }
    // Shortest first, then nearest the top left, then alphabetical: PAN, PAT, PIN
    // start at the top left tile, then NIP (1,2) is nearer than TAP (0,2).
    CHECK(order == std::vector<std::string>{"PAN", "PAT", "PIN", "TAP", "NIP"});
    CHECK(session.state() == SessionState::cleared);
}

TEST_CASE("there is no help outside `playing`", "[session]") {
    for (const SessionState state :
         {SessionState::box, SessionState::shaking, SessionState::dropping, SessionState::cleared,
          SessionState::resting}) {
        wjs::GameSession session = reach(state);
        CHECK_FALSE(session.request_help().has_value());
        CHECK_FALSE(session.lit_tile().has_value());
    }
}

// --- dealing: determinism and the range --------------------------------------

namespace {

// Everything a session shows after a shake, played through: a string two
// sessions can be compared by.
std::string play_one(wjs::GameSession& session) {
    REQUIRE(session.shake().has_value());
    REQUIRE(session.still());
    REQUIRE(session.landed());
    std::string log = session.record()->tiles + "|";
    for (const wjs::FoundWord& word : wjs::solve(*session.board(), phase_a().dictionary())) {
        const auto help = session.request_help();
        if (help) {
            log += std::to_string(help->row) + "," + std::to_string(help->column) + ";";
        }
        const wjs::TraceOutcome outcome = session.trace(word.path, 9);
        log += std::string(wjs::to_string(outcome.kind)) + ":" + outcome.find->word + ":" +
               std::to_string(outcome.stars()) + ";";
    }
    REQUIRE(session.rest());
    REQUIRE(session.again());
    return log;
}

}  // namespace

TEST_CASE("the same seed deals the same boards and gives the same outcomes", "[session]") {
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        wjs::SessionOptions options;
        options.size = size;
        options.seed = 2024;
        wjs::GameSession one = make(options);
        wjs::GameSession two = make(options);
        for (int board = 0; board < 6; ++board) {
            const std::string played = play_one(one);
            CHECK(played == play_one(two));
            CHECK(one.snapshot() == two.snapshot());
            CHECK(one.last_deal() == two.last_deal());
        }
    }
}

TEST_CASE("a session is the same however it is copied", "[session]") {
    wjs::SessionOptions options;
    options.seed = 5;
    wjs::GameSession original = make(options);
    wjs::GameSession copy = original;
    CHECK(play_one(original) == play_one(copy));
    CHECK(original.snapshot() == copy.snapshot());
}

TEST_CASE("different seeds deal different boards", "[session]") {
    std::set<std::string> boards;
    for (std::uint32_t seed = 1; seed <= 40; ++seed) {
        wjs::SessionOptions options;
        options.size = BoardSize::four;
        options.seed = seed;
        wjs::GameSession session = make(options);
        REQUIRE(session.shake().has_value());
        REQUIRE(session.still());
        REQUIRE(session.landed());
        boards.insert(session.record()->tiles);
    }
    CHECK(boards.size() >= 30U);
}

TEST_CASE("one seed is a whole run of boards, and they are not all the same", "[session]") {
    wjs::SessionOptions options;
    options.seed = 99;
    wjs::GameSession session = make(options);
    std::set<std::string> boards;
    for (int board = 0; board < 20; ++board) {
        REQUIRE(session.shake().has_value());
        REQUIRE(session.still());
        REQUIRE(session.landed());
        boards.insert(session.record()->tiles);
        REQUIRE(session.back());
    }
    CHECK(boards.size() >= 12U);
}

TEST_CASE("a board is never dealt with no word on it, and three to five is what it lands",
          "[session]") {
    // Property-style: many seeds, both sizes, the phase's own words. The count
    // the board holds is checked against the solver, not against the session's
    // own list.
    std::size_t in_range = 0;
    std::size_t boards = 0;
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        for (std::uint32_t seed = 1; seed <= 300; ++seed) {
            wjs::SessionOptions options;
            options.size = size;
            options.seed = seed;
            wjs::GameSession session = make(options);
            REQUIRE(session.shake().has_value());
            REQUIRE(session.still());
            REQUIRE(session.landed());

            const auto words = wjs::solve(*session.board(), phase_a().dictionary());
            const wjs::DealReport& deal = *session.last_deal();
            INFO("size " << wjs::side(size) << ", seed " << seed);
            CHECK(words.size() >= 1U);  // never none
            CHECK(deal.viable == words.size());
            CHECK(deal.in_range == (words.size() >= 3U && words.size() <= 5U));
            CHECK(session.record()->viable.size() == words.size());
            CHECK(deal.seedings >= 1U);
            CHECK(deal.seedings <= wjs::SessionOptions{}.max_seedings);
            ++boards;
            if (deal.in_range) {
                ++in_range;
            }
        }
    }
    // The test data is a handful of words, which the seeding lands nearly every
    // time (docs/testing.md, "Seeding: what was measured"); several seedings
    // make a miss rarer still.
    INFO(in_range << " of " << boards << " in range");
    CHECK(in_range == boards);
}

TEST_CASE("the range and the number of seedings are the options'", "[session]") {
    wjs::SessionOptions options;
    options.max_seedings = 1;
    options.min_viable = 2;
    options.max_viable = 2;
    for (std::uint32_t seed = 1; seed <= 40; ++seed) {
        options.seed = seed;
        wjs::GameSession session = make(options);
        REQUIRE(session.shake().has_value());
        const wjs::DealReport& deal = *session.last_deal();
        CHECK(deal.seedings == 1U);
        CHECK(deal.in_range == (deal.viable == 2U));
        CHECK(deal.viable >= 1U);
    }
}

TEST_CASE("a seeding that misses the range is tried again, and the nearest is kept", "[session]") {
    // An impossible range: no board of this phase holds nine words, so every
    // seeding misses. The session spends all its seedings and keeps the board
    // nearest the range, which still holds a word.
    wjs::SessionOptions options;
    options.min_viable = 9;
    options.max_viable = 9;
    options.max_seedings = 4;
    wjs::GameSession session = make(options);
    REQUIRE(session.shake().has_value());
    const wjs::DealReport& deal = *session.last_deal();
    CHECK(deal.seedings == 4U);
    CHECK_FALSE(deal.in_range);
    CHECK(deal.viable >= 1U);
    CHECK(deal.viable < 9U);
}

TEST_CASE("the focus sounds the parent ticked reach the seeding", "[session]") {
    // Boards prefer targets with a grapheme that makes a ticked sound. `n` is
    // /t3/ in this test data, and NIP, PAN and PIN are the words with it: every
    // board holds one.
    wjs::SessionOptions options;
    options.focus_sounds = {"/t3/"};
    for (std::uint32_t seed = 1; seed <= 100; ++seed) {
        options.seed = seed;
        wjs::GameSession session = make(options);
        REQUIRE(session.shake().has_value());
        REQUIRE(session.still());
        REQUIRE(session.landed());
        const std::vector<std::string> viable = session.record()->viable;
        INFO("seed " << seed);
        CHECK(std::ranges::any_of(viable, [](const std::string& word) {
            return word == "NIP" || word == "PAN" || word == "PIN";
        }));
    }
}

TEST_CASE("the dead jewels are the tiles of the letter the phase leaves out", "[session]") {
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        for (std::uint32_t seed = 1; seed <= 50; ++seed) {
            wjs::SessionOptions options;
            options.size = size;
            options.seed = seed;
            wjs::GameSession session = make(options);
            REQUIRE(session.shake().has_value());
            const wjs::Board& board = *session.board();
            std::vector<Position> dead;
            for (std::size_t row = 0; row < board.rows(); ++row) {
                for (std::size_t column = 0; column < board.columns(); ++column) {
                    if (board.tile({row, column}) == "B") {
                        dead.push_back({row, column});
                    }
                }
            }
            const auto listed = session.dead_tiles();
            CHECK(std::ranges::equal(dead, listed));
        }
    }
}

// --- the store ---------------------------------------------------------------

TEST_CASE("the record holds the board, the words on it and the finds", "[session]") {
    wjs::GameSession session = hand_session();
    session.trace(pan, 7);
    session.trace(tap, 8);
    const auto record = session.record();
    REQUIRE(record.has_value());
    CHECK(record->phase == "A");
    CHECK(record->size == BoardSize::three);
    CHECK(record->tiles == "P A T/B I N/B B B");
    CHECK(record->viable == std::vector<std::string>{"NIP", "PAN", "PAT", "PIN", "TAP"});
    REQUIRE(record->finds.size() == 2U);
    CHECK(record->finds[0].word == "PAN");
    CHECK(record->finds[0].when == 7);
    CHECK(record->finds[1].word == "TAP");
    CHECK(record->stars() == 2);
    CHECK_FALSE(record->cleared());
}

TEST_CASE("the record of a board that is left is still there to be saved", "[session]") {
    wjs::GameSession session = reach(SessionState::playing);
    const auto tiles = session.record()->tiles;
    const auto word = wjs::solve(*session.board(), phase_a().dictionary())[0];
    session.trace(word.path, 3);
    REQUIRE(session.back());
    REQUIRE(session.record().has_value());
    CHECK(session.record()->tiles == tiles);
    CHECK(session.record()->finds.size() == 1U);
    CHECK_FALSE(session.record()->cleared());

    // Until the next shake begins another board.
    REQUIRE(session.shake().has_value());
    CHECK_FALSE(session.record().has_value());
    REQUIRE(session.back());
    // A board backed out of before the dice landed was never seen: no record.
    CHECK_FALSE(session.record().has_value());
}

TEST_CASE("a board that was cleared says so in its record", "[session]") {
    wjs::GameSession session = reach(SessionState::resting);
    REQUIRE(session.record().has_value());
    CHECK(session.record()->cleared());
    CHECK(session.record()->finds.size() == session.record()->viable.size());
}

TEST_CASE("a snapshot brings a session back as it was, in each state", "[session]") {
    for (const SessionState state :
         {SessionState::box, SessionState::playing, SessionState::resting}) {
        wjs::GameSession session = reach(state);
        if (state == SessionState::playing) {
            const auto words = wjs::solve(*session.board(), phase_a().dictionary());
            session.trace(words[0].path, 12);
            session.request_help();
        }
        const wjs::SessionSnapshot snapshot = session.snapshot();
        CHECK(snapshot.state == state);

        auto restored = wjs::GameSession::restore(snapshot, phase_a(), english());
        REQUIRE(restored.has_value());
        CHECK(restored->state() == state);
        CHECK(restored->snapshot() == snapshot);
        CHECK(restored->stars() == session.stars());
        CHECK(words_of(restored->finds()) == words_of(session.finds()));
        CHECK(restored->lit_tile() == session.lit_tile());
        CHECK(std::ranges::equal(restored->dead_tiles(), session.dead_tiles()));

        // From the box the two go on the same way: the same next board. (After
        // a board the original remembers its targets and a restored session does
        // not, so the one after may differ; the snapshot does not keep them.)
        if (state == SessionState::box) {
            REQUIRE(restored->shake().has_value());
            REQUIRE(session.shake().has_value());
            CHECK(restored->snapshot() == session.snapshot());
            REQUIRE(restored->still());
            REQUIRE(session.still());
            REQUIRE(restored->landed());
            REQUIRE(session.landed());
            CHECK(restored->record()->tiles == session.record()->tiles);
        }
    }
}

TEST_CASE("a snapshot in shaking or dropping is the box, with the seed moved on", "[session]") {
    for (const SessionState state : {SessionState::shaking, SessionState::dropping}) {
        wjs::GameSession session = reach(state);
        const wjs::SessionSnapshot snapshot = session.snapshot();
        CHECK(snapshot.state == SessionState::box);
        CHECK_FALSE(snapshot.board.has_value());
        CHECK_FALSE(snapshot.lit.has_value());
        CHECK(snapshot.seed != wjs::SessionOptions{}.seed);
    }
}

TEST_CASE("a snapshot in cleared is saved as resting", "[session]") {
    wjs::GameSession session = reach(SessionState::cleared);
    CHECK(session.snapshot().state == SessionState::resting);
}

TEST_CASE("a restored session keeps the lit tile and the finds' flags", "[session]") {
    wjs::GameSession session = hand_session();
    REQUIRE(session.request_help().has_value());
    session.trace(pan, 5);  // assisted
    session.trace(tap, 6);  // not
    REQUIRE(session.request_help().has_value());
    const auto lit = session.lit_tile();

    auto restored = wjs::GameSession::restore(session.snapshot(), phase_a(), english());
    REQUIRE(restored.has_value());
    CHECK(restored->lit_tile() == lit);
    REQUIRE(restored->finds().size() == 2U);
    CHECK(restored->finds()[0].assisted);
    CHECK_FALSE(restored->finds()[1].assisted);
    CHECK(restored->finds()[0].when == 5);
    // The light goes out when its word is found, as it did before.
    const auto next = restored->request_help();
    REQUIRE(next.has_value());
}

TEST_CASE("restore reads the size and the seed from the snapshot, not the options", "[session]") {
    wjs::SessionOptions options;
    options.size = BoardSize::four;
    options.seed = 1;
    wjs::SessionSnapshot snapshot;
    snapshot.size = BoardSize::three;
    snapshot.seed = 123456;
    const wjs::GameSession session = restore(snapshot, options);
    CHECK(session.size() == BoardSize::three);
    CHECK(session.snapshot().seed == 123456U);
}

TEST_CASE("a snapshot of another phase is refused", "[session]") {
    auto snapshot = playing_snapshot();
    snapshot.board->phase = "B";
    const auto restored = wjs::GameSession::restore(snapshot, phase_a(), english());
    REQUIRE_FALSE(restored.has_value());
    CHECK(restored.error() == wjs::SessionError::wrong_phase);
}

TEST_CASE("a board the phase no longer reads the same way is refused", "[session]") {
    // The record says four words; the phase on the board finds five.
    auto snapshot = playing_snapshot();
    snapshot.board->viable = {"PAN", "PAT", "PIN", "TAP"};
    const auto restored = wjs::GameSession::restore(snapshot, phase_a(), english());
    REQUIRE_FALSE(restored.has_value());
    CHECK(restored.error() == wjs::SessionError::board_changed);

    // A board with no word in it at all is no board to play.
    auto none = playing_snapshot();
    none.board->tiles = "B B B/B B B/B B B";
    none.board->viable = {};
    const auto empty = wjs::GameSession::restore(none, phase_a(), english());
    REQUIRE_FALSE(empty.has_value());
    CHECK(empty.error() == wjs::SessionError::board_changed);
}

TEST_CASE("a snapshot that no session makes is refused", "[session]") {
    const auto refused = [](const wjs::SessionSnapshot& snapshot) {
        const auto restored = wjs::GameSession::restore(snapshot, phase_a(), english());
        return restored.has_value() ? std::nullopt : std::optional(restored.error());
    };
    const auto bad = std::optional(wjs::SessionError::bad_snapshot);

    SECTION("a state the store never holds") {
        for (const SessionState state :
             {SessionState::shaking, SessionState::dropping, SessionState::cleared}) {
            auto snapshot = playing_snapshot();
            snapshot.state = state;
            CHECK(refused(snapshot) == bad);
        }
    }
    SECTION("a board in the box, or none in play") {
        auto boxed = playing_snapshot();
        boxed.state = SessionState::box;
        CHECK(refused(boxed) == bad);
        auto missing = playing_snapshot();
        missing.board.reset();
        CHECK(refused(missing) == bad);
        auto resting = playing_snapshot();
        resting.state = SessionState::resting;
        resting.board.reset();
        CHECK(refused(resting) == bad);
    }
    SECTION("a light in the box") {
        wjs::SessionSnapshot snapshot;
        snapshot.lit = "PAT";
        CHECK(refused(snapshot) == bad);
    }
    SECTION("tiles that are not the size of the board") {
        auto snapshot = playing_snapshot();
        snapshot.board->tiles = "P A/B I";
        CHECK(refused(snapshot) == bad);
        auto garbled = playing_snapshot();
        garbled.board->tiles = "P A T/B I";
        CHECK(refused(garbled) == bad);
        auto unreadable = playing_snapshot();
        unreadable.board->tiles = "P A 7/B I N/B B B";
        CHECK(refused(unreadable) == bad);
    }
    SECTION("a record of one size on a snapshot of another") {
        auto snapshot = playing_snapshot();
        snapshot.size = BoardSize::four;
        CHECK(refused(snapshot) == bad);
    }
    SECTION("a find that is not on the board") {
        // SAT is a word of the phase, but not one this board holds.
        auto snapshot = playing_snapshot({jewel("SAT")});
        CHECK(refused(snapshot) == bad);
        auto dictionary_only = playing_snapshot({jewel("TIP")});
        CHECK(refused(dictionary_only) == bad);
    }
    SECTION("a word found twice") {
        auto snapshot = playing_snapshot({jewel("PAT"), jewel("pat", 1)});
        CHECK(refused(snapshot) == bad);
    }
    SECTION("a board that is playing but has every word found, or resting with some left") {
        auto all = playing_snapshot(
            {jewel("NIP"), jewel("PAN"), jewel("PAT"), jewel("PIN"), jewel("TAP")});
        CHECK(refused(all) == bad);
        auto resting = playing_snapshot({jewel("PAT")});
        resting.state = SessionState::resting;
        CHECK(refused(resting) == bad);
    }
    SECTION("a light on a word already found, one not on the board, or in a board at rest") {
        auto found = playing_snapshot({jewel("PAT")});
        found.lit = "PAT";
        CHECK(refused(found) == bad);
        auto away = playing_snapshot();
        away.lit = "SAT";
        CHECK(refused(away) == bad);
        auto at_rest = playing_snapshot(
            {jewel("NIP"), jewel("PAN"), jewel("PAT"), jewel("PIN"), jewel("TAP")});
        at_rest.state = SessionState::resting;
        at_rest.lit = "PAT";
        CHECK(refused(at_rest) == bad);
    }
}

TEST_CASE("a find in the snapshot may be in either case and comes back upper-case", "[session]") {
    const wjs::GameSession session = restore(playing_snapshot({jewel("pat", 3)}));
    REQUIRE(session.finds().size() == 1U);
    CHECK(session.finds()[0].word == "PAT");
    CHECK(session.finds()[0].when == 3);
}

// --- what the session never says ---------------------------------------------

TEST_CASE("no path of the board is answered wrongly, or names a word that is not today's",
          "[session]") {
    // Every simple path of two to nine tiles on the hand board, found by walking
    // the grid here and spelling each path from the tiles, and the answer held to
    // what the two word lists say of the spelling. A fresh session for each path,
    // so that one find does not change the answer to the next.
    const std::array<std::string, 9> tile = {"P", "A", "T", "B", "I", "N", "B", "B", "B"};
    const std::set<std::string> today = {"NIP", "PAN", "PAT", "PIN", "TAP"};

    std::size_t paths = 0;
    std::size_t named = 0;
    Path path;
    std::array<bool, 9> used{};
    std::string spelled;

    const auto check = [&] {
        wjs::GameSession session = hand_session();
        const wjs::TraceOutcome outcome = session.trace(path, 0);
        INFO("path of " << path.size() << " tiles spelling " << spelled);
        ++paths;
        if (today.contains(spelled)) {
            CHECK(outcome.kind == TraceKind::found);
            REQUIRE(outcome.find.has_value());
            CHECK(outcome.find->word == spelled);
            ++named;
            return;
        }
        // Nothing but a word of today's is ever named.
        CHECK_FALSE(outcome.find.has_value());
        if (english().contains(spelled)) {
            // A real word, of any length, is never "not a word".
            CHECK(outcome.kind == TraceKind::real_not_todays);
        } else {
            CHECK(outcome.kind == TraceKind::not_a_word);
        }
        // And nothing was kept.
        CHECK(session.finds().empty());
        CHECK(session.stars() == 0);
        const auto saved = session.snapshot();
        CHECK(saved.board->finds.empty());
        CHECK_FALSE(saved.lit.has_value());
    };

    const auto walk = [&](auto&& self, std::size_t row, std::size_t column) -> void {
        const std::size_t index = row * 3 + column;
        used[index] = true;
        path.push_back({row, column});
        spelled += tile[index];
        if (path.size() >= 2) {
            check();
        }
        for (std::size_t next_row = row == 0 ? 0 : row - 1;
             next_row <= std::min<std::size_t>(row + 1, 2); ++next_row) {
            for (std::size_t next_column = column == 0 ? 0 : column - 1;
                 next_column <= std::min<std::size_t>(column + 1, 2); ++next_column) {
                if (!used[next_row * 3 + next_column]) {
                    self(self, next_row, next_column);
                }
            }
        }
        spelled.pop_back();
        path.pop_back();
        used[index] = false;
    };
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            walk(walk, row, column);
        }
    }
    // 10,305 simple paths on a 3 by 3 board, 9 of them one tile long.
    CHECK(paths == 10'296U);
    CHECK(named >= 5U);
}

TEST_CASE("a word that is only in the dictionary is never handed out", "[session]") {
    // `tip` and `tab` stand for words a child must never meet. Trace them, ask
    // for help, clear the board, and read everything the session holds.
    wjs::GameSession session = hand_session();
    const Path tab{{0, 2}, {0, 1}, {1, 0}};  // T A B, a real word in this dictionary
    CHECK(session.trace(tip, 0).kind == TraceKind::real_not_todays);
    CHECK(session.trace(tab, 0).kind == TraceKind::real_not_todays);
    for (const Path* path : {&pan, &pat, &pin, &nip, &tap}) {
        session.request_help();
        const wjs::TraceOutcome outcome = session.trace(*path, 0);
        CHECK(outcome.kind == TraceKind::found);
        CHECK_FALSE(outcome.find->word == "TIP");
    }
    REQUIRE(session.state() == SessionState::cleared);

    const std::set<std::string> outside = {"TIP", "TAB", "PIT", "NAP", "ANT",
                                           "TAN", "AT",  "IN",  "IT"};
    const wjs::SessionSnapshot snapshot = session.snapshot();
    REQUIRE(snapshot.board.has_value());
    for (const wjs::Find& find : snapshot.board->finds) {
        CHECK_FALSE(outside.contains(find.word));
    }
    for (const std::string& word : snapshot.board->viable) {
        CHECK_FALSE(outside.contains(word));
    }
    for (const wjs::Find& find : session.finds()) {
        CHECK_FALSE(outside.contains(find.word));
    }
    CHECK_FALSE(snapshot.lit.has_value());
}

TEST_CASE("everything a dealt board holds is a word of the phase", "[session]") {
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        for (std::uint32_t seed = 1; seed <= 60; ++seed) {
            wjs::SessionOptions options;
            options.size = size;
            options.seed = seed;
            wjs::GameSession session = make(options);
            REQUIRE(session.shake().has_value());
            REQUIRE(session.still());
            REQUIRE(session.landed());
            for (const std::string& word : session.record()->viable) {
                INFO("word " << word);
                CHECK(phase_a().dictionary().contains(word));
            }
        }
    }
}
