#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fake_solo_game.hpp"
#include "real_solo_game.hpp"
#include "solo_game_factory.hpp"
#include "solo_game_setup.hpp"
#include "wjs/board.hpp"
#include "wjs/calendar.hpp"
#include "wjs/dictionary.hpp"
#include "wjs/game_session.hpp"
#include "wjs/phase_data.hpp"
#include "wjs/solver.hpp"
#include "wjs/store.hpp"

// The Solo Play port over the library's session (src/app/solo/real_solo_game.*):
// a table of the mapping, run against a tiny phase written here. The session's own
// rules are tested in tests/unit/game_session_test.cpp; what is held down here is
// that each call of the port is the call of the session it is meant to be, that
// nothing the session keeps back (a word that is not today's) is handed to the
// screens, and what is handed to the store.
//
// TEST DATA. Two phases written for these tests (the words of the session's own
// tests, with placeholder sounds) and a word list that holds them and a few real
// words that are in neither. None of it is a phase of the application or content for
// a child; the real phase files have to be written and reviewed by a person
// qualified to do it, and the repository has none.

namespace {

using wjs::BoardSize;
using wjs::Position;
using wjs::SessionState;
using wjs::app::RealSoloGame;
using wjs::app::SoloState;
using wjs::app::WordKind;
using wjs::app::WordTier;

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

// Real words of the word list that no phase has: they must never be named.
constexpr std::array<std::string_view, 9> extra_words = {"tip", "pit", "nap", "ant", "tan",
                                                         "at",  "in",  "it",  "tab"};
constexpr std::array<std::string_view, 11> phase_words = {
    "nip", "pan", "pat", "pin", "sat", "sit", "tap", "pain", "paint", "saint", "stain"};

const wjs::Dictionary& english() {
    static const wjs::Dictionary dictionary = [] {
        wjs::Dictionary words;
        for (const std::string_view word : phase_words) {
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

constexpr wjs::Day today = wjs::day_from_civil(2026, 10, 16);
const auto device_day = [] { return today; };

using Path = std::vector<Position>;

// The board of the cases below, 3 by 3; `B` is the dead letter of both phases (the
// first letter of A-Z that none of their words uses), so the six B tiles are dead
// jewels. The words and one path for each, by hand:
//
//     P A T      PAT    (0,0) (0,1) (0,2)      TAP   (0,2) (0,1) (0,0)
//     B I N      PIN    (0,0) (1,1) (1,2)      NIP   (1,2) (1,1) (0,0)
//     B B B      PAN    (0,0) (0,1) (1,2)
//
// and, in phase B as well, PAIN (0,0) (0,1) (1,1) (1,2) and PAINT, the same and
// then (0,2).
constexpr std::string_view hand_tiles = "P A T/B I N/B B B";
const Path pat{{0, 0}, {0, 1}, {0, 2}};
const Path tap{{0, 2}, {0, 1}, {0, 0}};
const Path pin{{0, 0}, {1, 1}, {1, 2}};
const Path nip{{1, 2}, {1, 1}, {0, 0}};
const Path pan{{0, 0}, {0, 1}, {1, 2}};
const Path pain{{0, 0}, {0, 1}, {1, 1}, {1, 2}};
const Path paint{{0, 0}, {0, 1}, {1, 1}, {1, 2}, {0, 2}};
// Real words of the word list that are not today's, and some that are not words.
const Path tip{{0, 2}, {1, 1}, {0, 0}};  // TIP
const Path at{{0, 1}, {0, 2}};           // AT, two letters
const Path pab{{0, 0}, {0, 1}, {1, 0}};  // PAB, no word
const Path apart{{0, 0}, {0, 2}};        // not touching: not a path
const Path one_tile{{0, 0}};             // a touch to hear a letter

// The options of a phonics session: the game under test plays that board.
wjs::SessionOptions phonics(wjs::SessionOptions options = {}) {
    options.mode = wjs::BoardMode::phonics;
    return options;
}

wjs::SessionData data_of(const wjs::PhaseIndex& phase) {
    return {.dictionary = english(), .phase = &phase};
}

wjs::BoardRecord hand_record(std::string_view phase, std::vector<wjs::Find> finds = {}) {
    wjs::BoardRecord record;
    record.mode = wjs::BoardMode::phonics;
    record.phase = std::string(phase);
    record.size = BoardSize::three;
    record.tiles = std::string(hand_tiles);
    record.viable =
        phase == "A" ? std::vector<std::string>{"NIP", "PAN", "PAT", "PIN", "TAP"}
                     : std::vector<std::string>{"NIP", "PAIN", "PAINT", "PAN", "PAT", "PIN", "TAP"};
    record.finds = std::move(finds);
    return record;
}

wjs::Find jewel(std::string word) {
    return {.word = std::move(word), .assisted = false, .when = today};
}

const wjs::PhaseIndex& phase_named(std::string_view name) {
    return name == "A" ? phase_a() : phase_b();
}

// A session playing the hand board of `phase`, with `finds` already in its tray.
wjs::GameSession hand_session(std::string_view phase, std::vector<wjs::Find> finds = {},
                              const wjs::SessionOptions& options = {}) {
    wjs::SessionSnapshot snapshot;
    snapshot.state = SessionState::playing;
    snapshot.mode = wjs::BoardMode::phonics;
    snapshot.size = BoardSize::three;
    snapshot.seed = 77;
    snapshot.board = hand_record(phase, std::move(finds));
    auto restored =
        wjs::GameSession::restore(snapshot, data_of(phase_named(phase)), phonics(options));
    REQUIRE(restored.has_value());
    return *std::move(restored);
}

wjs::GameSession dealing_session(const wjs::SessionOptions& options = {}) {
    auto created = wjs::GameSession::create(data_of(phase_a()), phonics(options));
    REQUIRE(created.has_value());
    return *std::move(created);
}

// A session that has dealt a board of its own and is in `state`.
wjs::GameSession reach(SessionState state) {
    wjs::GameSession session = dealing_session();
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
    for (const wjs::FoundWord& word : wjs::solve(*session.board(), phase_a().dictionary())) {
        session.trace(word.path, today);
    }
    REQUIRE(session.state() == SessionState::cleared);
    if (state == SessionState::cleared) {
        return session;
    }
    REQUIRE(session.rest());
    return session;
}

// A game over a store of its own.
struct Table {
    wjs::Store store = wjs::Store::in_memory();
    RealSoloGame game;

    // The hand board of `phase`, in play.
    explicit Table(std::string_view phase, std::vector<wjs::Find> finds = {}, bool help = true)
        : game(hand_session(phase, std::move(finds)), help, phase_named(phase), store, device_day) {
    }

    // A session already made, over phase A.
    explicit Table(wjs::GameSession session)
        : game(std::move(session), true, phase_a(), store, device_day) {}
};

std::vector<std::string> words_of(const std::vector<wjs::app::SoloFound>& found) {
    std::vector<std::string> words;
    for (const auto& entry : found) {
        words.push_back(entry.word);
    }
    return words;
}

}  // namespace

// --- the box -----------------------------------------------------------------

TEST_CASE("a new game is the closed box with the options' size and help", "[app][real]") {
    wjs::Store store = wjs::Store::in_memory();
    wjs::SessionOptions options;
    options.size = BoardSize::four;
    options.help = false;
    auto made = RealSoloGame::create(phase_a(), english(), store, options, device_day);
    REQUIRE(made.has_value());
    const RealSoloGame& game = *made;

    CHECK(game.state() == SoloState::box);
    CHECK(game.size() == 4);
    REQUIRE(game.sizes().size() == 2);
    CHECK(game.sizes()[0] == 3);
    CHECK(game.sizes()[1] == 4);
    CHECK_FALSE(game.helpAllowed());
    CHECK(game.board().empty());
    CHECK(game.found().empty());
    CHECK(game.history().empty());
    CHECK_FALSE(game.litTile().has_value());
}

TEST_CASE("a session that cannot be made is the session's error", "[app][real]") {
    wjs::Store store = wjs::Store::in_memory();
    wjs::SessionOptions options;
    options.min_viable = 6;
    options.max_viable = 2;
    const auto made = RealSoloGame::create(phase_a(), english(), store, options, device_day);
    REQUIRE_FALSE(made.has_value());
    CHECK(made.error() == wjs::SessionError::bad_options);
}

TEST_CASE("the size is chosen in the box and nowhere else", "[app][real]") {
    Table in_box(dealing_session());
    in_box.game.chooseSize(4);
    CHECK(in_box.game.size() == 4);
    in_box.game.chooseSize(5);  // no such size
    CHECK(in_box.game.size() == 4);
    in_box.game.chooseSize(3);
    CHECK(in_box.game.size() == 3);

    Table playing("A");
    playing.game.chooseSize(4);
    CHECK(playing.game.size() == 3);
}

// --- shake, settled, the board -----------------------------------------------

TEST_CASE("the shake deals the board before anything moves, and settled lands it", "[app][real]") {
    Table table(dealing_session());
    RealSoloGame& game = table.game;

    REQUIRE(game.shake());
    CHECK(game.state() == SoloState::shaking);
    const auto& board = game.board();
    CHECK(board.rows == 3);
    CHECK(board.columns == 3);
    REQUIRE(board.tiles.size() == 9);
    // The scene's board is the session's, with a dead jewel as "" and the seed of
    // the seeding that made it.
    const wjs::Board* dealt = game.session().board();
    REQUIRE(dealt != nullptr);
    const auto dead = game.session().dead_tiles();
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            const Position position{row, column};
            const bool is_dead = std::ranges::find(dead, position) != dead.end();
            CHECK(board.tile(position) == (is_dead ? std::string{} : dealt->tile(position)));
        }
    }
    REQUIRE(game.session().last_deal().has_value());
    CHECK(board.seed == game.session().last_deal()->seed);
    CHECK(game.found().empty());

    game.settled();
    CHECK(game.state() == SoloState::playing);
    CHECK(game.session().state() == SessionState::playing);
}

TEST_CASE("settled is the session's still and landed, whichever it is waiting for", "[app][real]") {
    {
        wjs::GameSession session = reach(SessionState::dropping);
        Table table(std::move(session));
        REQUIRE(table.game.state() == SoloState::dropping);
        table.game.settled();
        CHECK(table.game.state() == SoloState::playing);
    }
    {
        Table table(reach(SessionState::playing));
        table.game.settled();  // late, or twice: nothing changes
        CHECK(table.game.state() == SoloState::playing);
    }
    {
        Table table(dealing_session());
        table.game.settled();  // no board yet
        CHECK(table.game.state() == SoloState::box);
    }
}

TEST_CASE("a shake that deals no board answers false and stays in the box", "[app][real]") {
    // The phase plants words of three letters at most; this one has only four.
    const std::string graphemes =
        "# phase: A\na  /x/  clip:a\nb  /x/  clip:b\nc  /x/  clip:c\nd  /x/  clip:d\n";
    const std::string words = "# phase: A\nabcd  a b c d\n";
    wjs::Dictionary dictionary;
    dictionary.insert("abcd");
    std::istringstream graphemes_in{graphemes};
    std::istringstream words_in{words};
    const auto built = wjs::PhaseSet::build({*wjs::parse_graphemes(graphemes_in, "g")},
                                            {*wjs::parse_words(words_in, "w")}, dictionary);
    REQUIRE(built.has_value());
    wjs::Store store = wjs::Store::in_memory();
    auto made = RealSoloGame::create(*built->find("A"), dictionary, store, {}, device_day);
    REQUIRE(made.has_value());

    CHECK_FALSE(made->shake());
    CHECK(made->state() == SoloState::box);
    CHECK(made->board().empty());
}

TEST_CASE("every state of the session is the port's, by the design's names", "[app][real]") {
    struct Row {
        SessionState session;
        SoloState solo;
    };
    const std::array rows = {
        Row{SessionState::box, SoloState::box},
        Row{SessionState::shaking, SoloState::shaking},
        Row{SessionState::dropping, SoloState::dropping},
        Row{SessionState::playing, SoloState::playing},
        Row{SessionState::cleared, SoloState::cleared},
        Row{SessionState::resting, SoloState::resting},
    };
    for (const Row& row : rows) {
        INFO(wjs::to_string(row.session));
        Table table(reach(row.session));
        CHECK(table.game.state() == row.solo);
        // The screens' ids are the design's without the prefix.
        CHECK("solo." + std::string(wjs::app::to_string(row.solo)) ==
              std::string(wjs::to_string(row.session)));
    }
}

TEST_CASE("a game over a session in play shows its board, dead jewels and tray", "[app][real]") {
    Table table("B", {jewel("PAT"), jewel("PAIN")});
    const RealSoloGame& game = table.game;

    CHECK(game.state() == SoloState::playing);
    REQUIRE(game.board().tiles.size() == 9);
    const std::vector<std::string> expected = {"P", "A", "T", "", "I", "N", "", "", ""};
    CHECK(game.board().tiles == expected);
    CHECK(game.board().seed == 0);  // a restored session has dealt nothing
    REQUIRE(game.found().size() == 2);
    CHECK(game.found()[0].word == "pat");
    CHECK(game.found()[0].tier == WordTier::spark);
    CHECK(game.found()[0].stars == 1);
    CHECK(game.found()[1].word == "pain");
    CHECK(game.found()[1].tier == WordTier::power);
    CHECK(game.found()[1].stars == 2);
}

// --- a traced path -----------------------------------------------------------

TEST_CASE("each answer of a trace is the port's, and only a word of today's is named",
          "[app][real]") {
    struct Row {
        std::string_view what;
        const Path* path;
        WordKind kind;
        std::string_view word;  // lower-case, for a word of today's only
        WordTier tier;
        int stars;
        bool already;
        bool sticker;
    };
    // In order, on one board: a word found stays found.
    const std::array rows = {
        Row{"a spark word", &pat, WordKind::viable, "pat", WordTier::spark, 1, false, false},
        Row{"the same word again", &pat, WordKind::viable, "pat", WordTier::spark, 1, true, false},
        Row{"a word the other way round", &tap, WordKind::viable, "tap", WordTier::spark, 1, false,
            false},
        Row{"a power word and its sticker", &pain, WordKind::viable, "pain", WordTier::power, 2,
            false, true},
        Row{"a mega word", &paint, WordKind::viable, "paint", WordTier::mega, 5, false, false},
        Row{"a real word that is not today's", &tip, WordKind::realNotTodays, "", WordTier::spark,
            0, false, false},
        Row{"a real word of two letters", &at, WordKind::realNotTodays, "", WordTier::spark, 0,
            false, false},
        Row{"letters that are no word", &pab, WordKind::notAWord, "", WordTier::spark, 0, false,
            false},
        Row{"tiles that do not touch", &apart, WordKind::notAWord, "", WordTier::spark, 0, false,
            false},
        Row{"a single tile", &one_tile, WordKind::notAWord, "", WordTier::spark, 0, false, false},
    };

    Table table("B");
    for (const Row& row : rows) {
        INFO(row.what);
        const auto outcome = table.game.submit(*row.path);
        CHECK(outcome.kind == row.kind);
        CHECK(outcome.found.word == row.word);
        CHECK(outcome.found.tier == row.tier);
        CHECK(outcome.found.stars == row.stars);
        CHECK(outcome.alreadyFound == row.already);
        CHECK(outcome.sticker == row.sticker);
        CHECK_FALSE(outcome.cleared);
        CHECK_FALSE(outcome.newBest);
    }
    // Only the finds made it to the tray, in the order found.
    CHECK(words_of(table.game.found()) == std::vector<std::string>{"pat", "tap", "pain", "paint"});
}

TEST_CASE("a path is answered not a word, and changes nothing, outside play", "[app][real]") {
    Table table(dealing_session());
    CHECK(table.game.submit(pat).kind == WordKind::notAWord);
    CHECK(table.game.found().empty());
    CHECK(table.store.data().boards.empty());
}

TEST_CASE("the last word clears the board; celebrated rests it and Again closes the box",
          "[app][real]") {
    Table table("A");
    RealSoloGame& game = table.game;

    // The same call out of `playing` does nothing.
    game.celebrated();
    game.again();
    CHECK(game.state() == SoloState::playing);

    for (const Path* path : {&pat, &tap, &pin, &nip}) {
        const auto outcome = game.submit(*path);
        REQUIRE(outcome.kind == WordKind::viable);
        CHECK_FALSE(outcome.cleared);
    }
    const auto last = game.submit(pan);
    CHECK(last.kind == WordKind::viable);
    CHECK(last.cleared);
    // The first board of a size has no best to beat.
    CHECK_FALSE(last.newBest);
    CHECK(game.state() == SoloState::cleared);
    CHECK(game.found().size() == 5);

    game.again();  // the celebration is not over
    CHECK(game.state() == SoloState::cleared);
    game.celebrated();
    CHECK(game.state() == SoloState::resting);
    game.again();
    CHECK(game.state() == SoloState::box);
}

// --- what the store is handed ------------------------------------------------

TEST_CASE("each find is handed to the store with the day", "[app][real]") {
    Table table("A");
    REQUIRE(table.game.submit(pat).kind == WordKind::viable);

    const wjs::StoreData& data = table.store.data();
    REQUIRE(data.boards.size() == 1);
    CHECK(data.boards[0].phase == "A");
    CHECK(data.boards[0].size == 3);
    CHECK(data.boards[0].found == 1);
    CHECK(data.boards[0].viable_total == 5);
    REQUIRE(data.finds.size() == 1);
    CHECK(data.finds[0].word == "pat");
    CHECK(data.finds[0].stars == 1);
    CHECK(data.finds[0].day == today);
    CHECK_FALSE(data.finds[0].start_tile_used);
    REQUIRE(data.vault.size() == 1);
    CHECK(data.vault[0].word == "pat");

    // Found again is nothing new.
    REQUIRE(table.game.submit(pat).alreadyFound);
    CHECK(table.store.data().finds.size() == 1);
}

TEST_CASE("a clear that beats the best of its size is the new best, once, on the last word",
          "[app][real]") {
    wjs::Store store = wjs::Store::in_memory();
    // An earlier board of this size, cleared for one star: the best to beat.
    wjs::BoardSnapshot earlier;
    earlier.phase = "B";
    earlier.size = 3;
    earlier.tiles = "X Y Z/X Y Z/X Y Z";
    earlier.viable = {"PAN"};
    earlier.finds = {{.word = "pan", .assisted = false, .day = today - 1, .stars = 1}};
    REQUIRE(store.record(earlier, today - 1).stored);
    REQUIRE(store.best_stars(3) == 1);

    RealSoloGame game(hand_session("B"), true, phase_b(), store, device_day);
    // Seven words, five of them 1 star, PAIN 2 and PAINT 5: twelve stars.
    const std::array<const Path*, 7> all = {&pat, &tap, &pin, &nip, &pan, &pain, &paint};
    for (std::size_t i = 0; i < all.size(); ++i) {
        INFO(i);
        const auto outcome = game.submit(*all[i]);
        REQUIRE(outcome.kind == WordKind::viable);
        CHECK(outcome.cleared == (i + 1 == all.size()));
        CHECK(outcome.newBest == (i + 1 == all.size()));
    }
    CHECK(store.best_stars(3) == 12);
}

TEST_CASE("a sticker is for the first time a word is found on this device", "[app][real]") {
    wjs::Store store = wjs::Store::in_memory();
    wjs::BoardSnapshot earlier;
    earlier.phase = "B";
    earlier.size = 3;
    earlier.tiles = "X Y Z/X Y Z/X Y Z";
    earlier.viable = {"PAIN"};
    earlier.finds = {{.word = "pain", .assisted = false, .day = today - 1, .stars = 2}};
    REQUIRE(store.record(earlier, today - 1).stored);

    RealSoloGame game(hand_session("B"), true, phase_b(), store, device_day);
    const auto again = game.submit(pain);
    CHECK(again.kind == WordKind::viable);
    CHECK_FALSE(again.alreadyFound);  // not on this board's tray
    CHECK_FALSE(again.sticker);       // but on this device's vault

    // A tier with no sticker never gets one, new or not.
    CHECK_FALSE(game.submit(pat).sticker);
}

TEST_CASE("the lid hands the board left to the store, found or not", "[app][real]") {
    SECTION("a board with a find and one with none") {
        Table table("A");
        table.game.submit(pat);
        table.game.lidOn();
        CHECK(table.game.state() == SoloState::box);
        CHECK(table.store.data().boards.size() == 1);

        // The board is kept after the lid goes on, so the screens never read air.
        CHECK(table.game.board().tiles.size() == 9);

        // Another board, left at once.
        wjs::Store& store = table.store;
        RealSoloGame second(hand_session("B"), true, phase_b(), store, device_day);
        second.lidOn();
        CHECK(second.state() == SoloState::box);
        CHECK(store.data().boards.size() == 2);
        CHECK(store.data().boards[1].found == 0);
    }
    SECTION("a board the child never saw is not a board") {
        Table table(dealing_session());
        REQUIRE(table.game.shake());
        table.game.lidOn();
        CHECK(table.game.state() == SoloState::box);
        CHECK(table.store.data().boards.empty());
    }
    SECTION("the lid does nothing in the box or after the last word") {
        Table table("A");
        for (const Path* path : {&pat, &tap, &pin, &nip, &pan}) {
            table.game.submit(*path);
        }
        REQUIRE(table.game.state() == SoloState::cleared);
        table.game.lidOn();
        CHECK(table.game.state() == SoloState::cleared);
        table.game.celebrated();
        table.game.again();
        table.game.lidOn();
        CHECK(table.game.state() == SoloState::box);
        CHECK(table.store.data().boards.size() == 1);
    }
}

TEST_CASE("the history is the store's boards, newest first, with their finds", "[app][real]") {
    wjs::Store store = wjs::Store::in_memory();
    {
        RealSoloGame first(hand_session("A"), true, phase_a(), store, device_day);
        first.submit(pat);
        first.submit(tap);
        first.lidOn();
    }
    {
        RealSoloGame second(hand_session("B"), true, phase_b(), store, device_day);
        second.submit(pain);
        second.lidOn();

        const auto history = second.history();
        REQUIRE(history.size() == 2);

        // Newest first: the phase B board, whose dead letter this game knows.
        const std::vector<std::string> own = {"P", "A", "T", "", "I", "N", "", "", ""};
        CHECK(history[0].board.rows == 3);
        CHECK(history[0].board.columns == 3);
        CHECK(history[0].board.tiles == own);
        REQUIRE(history[0].found.size() == 1);
        CHECK(history[0].found[0].word == "pain");
        CHECK(history[0].found[0].tier == WordTier::power);
        CHECK(history[0].found[0].stars == 2);

        // The phase A board, from a game of another phase: its letters as they are.
        const std::vector<std::string> other = {"P", "A", "T", "B", "I", "N", "B", "B", "B"};
        CHECK(history[1].board.tiles == other);
        CHECK(words_of(history[1].found) == std::vector<std::string>{"pat", "tap"});
    }
    // The phase A game reads its own board with the dead jewels blank.
    const RealSoloGame third(hand_session("A"), true, phase_a(), store, device_day);
    const auto history = third.history();
    REQUIRE(history.size() == 2);
    const std::vector<std::string> mine = {"P", "A", "T", "", "I", "N", "", "", ""};
    CHECK(history[1].board.tiles == mine);
}

// --- help ------------------------------------------------------------------

TEST_CASE("the start tile is the session's, lit until its word is found", "[app][real]") {
    Table table("A");
    RealSoloGame& game = table.game;
    CHECK(game.helpAllowed());
    CHECK_FALSE(game.litTile().has_value());  // asking lights nothing

    // The shortest word nearest the top left: PAN, which starts at the top left.
    const auto tile = game.startTile();
    REQUIRE(tile.has_value());
    CHECK(*tile == Position{0, 0});
    CHECK(game.litTile() == tile);
    CHECK(game.session().lit_tile() == tile);

    // Another word found: the same tile stays lit.
    game.submit(tap);
    CHECK(game.litTile() == tile);
    // Its own word found: gone.
    game.submit(pan);
    CHECK_FALSE(game.litTile().has_value());

    // An assisted find reaches the store as one.
    REQUIRE(table.store.data().finds.size() == 2);
    CHECK_FALSE(table.store.data().finds[0].start_tile_used);
    CHECK(table.store.data().finds[1].start_tile_used);
}

TEST_CASE("with the parent's help switch off there is no start tile", "[app][real]") {
    wjs::Store store = wjs::Store::in_memory();
    wjs::SessionOptions options;
    options.help = false;
    RealSoloGame game(hand_session("A", {}, options), false, phase_a(), store, device_day);

    CHECK_FALSE(game.helpAllowed());
    CHECK_FALSE(game.startTile().has_value());
    CHECK_FALSE(game.litTile().has_value());
}

// --- the grapheme blocks ---------------------------------------------------

TEST_CASE("the blocks of a path are the binder's, over the phase's graphemes", "[app][real]") {
    Table table("B");
    // P, then the digraph AI as one block of two tiles, then N.
    const auto blocks = table.game.blocks(pain);
    REQUIRE(blocks.size() == 3);
    CHECK(blocks[0].first == 0);
    CHECK(blocks[0].tiles == 1);
    CHECK(blocks[1].first == 1);
    CHECK(blocks[1].tiles == 2);
    CHECK(blocks[1].grapheme == "ai");
    CHECK(blocks[2].first == 3);
    CHECK(blocks[2].tiles == 1);

    // Phase A has no digraph: every tile is a block of its own.
    Table single("A");
    CHECK(single.game.blocks(pain).size() == 4);

    // No board, no blocks; a position off the board is not read.
    Table closed(dealing_session());
    CHECK(closed.game.blocks(pat).empty());
    const Path off{{0, 0}, {5, 5}};
    CHECK(table.game.blocks(off).empty());
}

// --- the program's choice of game --------------------------------------------

TEST_CASE("the application plays the stand-in until it is told what to play", "[app][real]") {
    CHECK(dynamic_cast<wjs::app::FakeSoloGame*>(wjs::app::make_solo_game().get()) != nullptr);

    wjs::Store store = wjs::Store::in_memory();
    {
        const wjs::app::ScopedSoloGameMaker installed([&store] {
            return std::make_unique<RealSoloGame>(hand_session("A"), true, phase_a(), store,
                                                  device_day);
        });
        const auto game = wjs::app::make_solo_game();
        CHECK(dynamic_cast<RealSoloGame*>(game.get()) != nullptr);
        CHECK(game->state() == SoloState::playing);
    }
    CHECK(dynamic_cast<wjs::app::FakeSoloGame*>(wjs::app::make_solo_game().get()) != nullptr);
}

// --- loading what the game needs -----------------------------------------------

namespace {

class TempDirectory {
public:
    TempDirectory() {
        static int counter = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("wjs_solo_setup_test_" + std::to_string(stamp) + "_" + std::to_string(++counter));
        std::filesystem::create_directories(path_);
    }
    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    ~TempDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

void write(const std::filesystem::path& file, std::string_view text) {
    std::filesystem::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary);
    out << text;
}

// The phases and the word list of this file, as a directory holds them.
void write_data(const TempDirectory& directory) {
    write(directory.path() / "phonics" / "phase-a.graphemes.txt", graphemes_a);
    write(directory.path() / "phonics" / "phase-a.words.txt", words_a);
    write(directory.path() / "phonics" / "phase-b.graphemes.txt", graphemes_b);
    write(directory.path() / "phonics" / "phase-b.words.txt", words_b);
    std::string list;
    for (const auto word : phase_words) {
        list += std::string(word) + "\n";
    }
    for (const auto word : extra_words) {
        list += std::string(word) + "\n";
    }
    write(directory.path() / "en.txt", list);
}

wjs::app::SoloGameOptions options_in(const TempDirectory& directory) {
    wjs::app::SoloGameOptions options;
    options.phase_directory = directory.path() / "phonics";
    options.dictionary = directory.path() / "en.txt";
    options.seed = 5;
    options.today = device_day;
    return options;
}

}  // namespace

TEST_CASE("the setup loads a phase from a directory and makes a game from it", "[app][real]") {
    const TempDirectory directory;
    write_data(directory);

    auto opened = wjs::app::SoloGameSetup::open(options_in(directory));
    REQUIRE(opened.has_value());
    auto& setup = **opened;

    // The files carry no review header: test data, and said so.
    CHECK_FALSE(setup.reviewed());
    CHECK(setup.unreviewed_files().size() == 4);
    // With no phase named, the settings' phase: "A" on a new device.
    CHECK(setup.phase().phase() == "A");
    CHECK(setup.store_status() == wjs::LoadStatus::created);

    // Two games of one fixed seed deal the same board.
    const auto first = setup.make();
    const auto second = setup.make();
    REQUIRE(first->shake());
    REQUIRE(second->shake());
    CHECK(first->board().tiles == second->board().tiles);
    CHECK(first->board().seed == second->board().seed);
    CHECK(first->state() == SoloState::shaking);
}

TEST_CASE("the setup makes each game from the parent's settings", "[app][real]") {
    const TempDirectory directory;
    write_data(directory);
    const auto file = directory.path() / "store.wjs";
    {
        auto opened = wjs::Store::open(file);
        wjs::Settings settings = opened.store.settings();
        settings.help = false;
        settings.board_size = 4;
        settings.phase = "B";
        opened.store.update_settings(settings);
    }

    auto options = options_in(directory);
    options.store = file;
    auto opened = wjs::app::SoloGameSetup::open(options);
    REQUIRE(opened.has_value());
    CHECK((*opened)->store_status() == wjs::LoadStatus::loaded);
    CHECK((*opened)->phase().phase() == "B");

    const auto game = (*opened)->make();
    CHECK_FALSE(game->helpAllowed());
    CHECK(game->size() == 4);

    // A phase named on the command line wins over the settings.
    options.phase = "A";
    auto named = wjs::app::SoloGameSetup::open(options);
    REQUIRE(named.has_value());
    CHECK((*named)->phase().phase() == "A");
}

TEST_CASE("a game made by the setup writes its finds to the store's file", "[app][real]") {
    const TempDirectory directory;
    write_data(directory);
    auto options = options_in(directory);
    options.store = directory.path() / "kept" / "store.wjs";

    {
        auto opened = wjs::app::SoloGameSetup::open(options);
        REQUIRE(opened.has_value());
        const auto game = (*opened)->make();
        REQUIRE(game->shake());
        game->settled();
        // Every word the solver finds on the dealt board, traced along its path.
        const auto* real = dynamic_cast<RealSoloGame*>(game.get());
        REQUIRE(real != nullptr);
        const auto solved = wjs::solve(*real->session().board(), (*opened)->phase().dictionary());
        REQUIRE_FALSE(solved.empty());
        CHECK(game->submit(solved.front().path).kind == WordKind::viable);
        game->lidOn();
    }

    // The next run reads the board back from the file.
    auto reopened = wjs::app::SoloGameSetup::open(options);
    REQUIRE(reopened.has_value());
    CHECK((*reopened)->store_status() == wjs::LoadStatus::loaded);
    const auto game = (*reopened)->make();
    const auto history = game->history();
    REQUIRE(history.size() == 1);
    CHECK(history[0].found.size() == 1);
}

TEST_CASE("the setup says why it cannot load", "[app][real]") {
    using Catch::Matchers::ContainsSubstring;
    const TempDirectory directory;
    write_data(directory);

    SECTION("no word list") {
        auto options = options_in(directory);
        options.dictionary = directory.path() / "missing.txt";
        const auto opened = wjs::app::SoloGameSetup::open(options);
        REQUIRE_FALSE(opened.has_value());
        CHECK_THAT(opened.error(), ContainsSubstring("could not be read"));
        CHECK_THAT(opened.error(), ContainsSubstring("--dictionary"));
    }
    SECTION("no calendar") {
        auto options = options_in(directory);
        options.today = nullptr;
        const auto opened = wjs::app::SoloGameSetup::open(options);
        REQUIRE_FALSE(opened.has_value());
        CHECK_THAT(opened.error(), ContainsSubstring("calendar"));
    }
    SECTION("no phase directory") {
        auto options = options_in(directory);
        options.phase_directory = directory.path() / "nowhere";
        const auto opened = wjs::app::SoloGameSetup::open(options);
        REQUIRE_FALSE(opened.has_value());
        CHECK_THAT(opened.error(), ContainsSubstring("no such directory"));
    }
    SECTION("a file with a problem names it") {
        write(directory.path() / "phonics" / "phase-a.words.txt", "# phase: A\npat  p a x\n");
        const auto opened = wjs::app::SoloGameSetup::open(options_in(directory));
        REQUIRE_FALSE(opened.has_value());
        CHECK_THAT(opened.error(), ContainsSubstring("phase-a.words.txt"));
    }
    SECTION("a phase that is not there") {
        auto options = options_in(directory);
        options.phase = "Z";
        const auto opened = wjs::app::SoloGameSetup::open(options);
        REQUIRE_FALSE(opened.has_value());
        CHECK_THAT(opened.error(), ContainsSubstring("no phase Z"));
        CHECK_THAT(opened.error(), ContainsSubstring("A B"));
    }
    SECTION("a phase that uses every letter cannot make a board") {
        std::string graphemes = "# phase: A\n";
        std::string words = "# phase: A\n";
        std::string list;
        const std::array<std::string_view, 9> triples = {"abc", "def", "ghi", "jkl", "mno",
                                                         "pqr", "stu", "vwx", "yza"};
        for (char letter = 'a'; letter <= 'z'; ++letter) {
            graphemes += std::string(1, letter) + "  /x/  clip:" + std::string(1, letter) + "\n";
        }
        for (const std::string_view word : triples) {
            words += std::string(word) + "  ";
            words += std::string(1, word[0]) + " " + std::string(1, word[1]) + " " +
                     std::string(1, word[2]) + "\n";
            list += std::string(word) + "\n";
        }
        const TempDirectory every;
        write(every.path() / "phonics" / "phase-a.graphemes.txt", graphemes);
        write(every.path() / "phonics" / "phase-a.words.txt", words);
        write(every.path() / "en.txt", list);
        const auto opened = wjs::app::SoloGameSetup::open(options_in(every));
        REQUIRE_FALSE(opened.has_value());
        CHECK_THAT(opened.error(), ContainsSubstring("cannot be played"));
    }
}
