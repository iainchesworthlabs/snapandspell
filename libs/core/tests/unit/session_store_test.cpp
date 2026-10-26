// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "sns/dice.hpp"
#include "sns/dice_sets.hpp"
#include "sns/dictionary.hpp"
#include "sns/game_session.hpp"
#include "sns/phase_data.hpp"
#include "sns/session_store.hpp"
#include "sns/solver.hpp"
#include "sns/store.hpp"

// TEST DATA. A phase written for these tests with placeholder sounds (the seven
// words the game session's own tests use): not a phase of the application and not
// content for a child; the real phase files have to be written and reviewed by a
// person qualified to do it.

namespace {

using sns::BoardSize;
using sns::SessionState;

constexpr std::string_view graphemes_a =
    "# phase: A\n"
    "a  /t1/  clip:t1\n"
    "i  /t2/  clip:t2\n"
    "n  /t3/  clip:t3\n"
    "p  /t4/  clip:t4\n"
    "s  /t5/  clip:t5\n"
    "t  /t6/  clip:t6\n";
constexpr std::string_view words_a =
    "# phase: A\n"
    "nip  n i p\n"
    "pan  p a n\n"
    "pat  p a t\n"
    "pin  p i n\n"
    "sat  s a t\n"
    "sit  s i t\n"
    "tap  t a p\n";

const sns::Dictionary& english() {
    static const sns::Dictionary dictionary = [] {
        sns::Dictionary words;
        for (const std::string_view word : {"nip", "pan", "pat", "pin", "sat", "sit", "tap"}) {
            REQUIRE(words.insert(word));
        }
        return words;
    }();
    return dictionary;
}

const sns::PhaseIndex& phase_a() {
    static const sns::PhaseSet set = [] {
        std::istringstream graphemes_in{std::string(graphemes_a)};
        std::istringstream words_in{std::string(words_a)};
        auto graphemes = sns::parse_graphemes(graphemes_in, "a.g");
        auto words = sns::parse_words(words_in, "a.w");
        REQUIRE(graphemes.has_value());
        REQUIRE(words.has_value());
        auto built = sns::PhaseSet::build({*graphemes}, {*words}, english());
        REQUIRE(built.has_value());
        return *std::move(built);
    }();
    return *set.find("A");
}

using Path = std::vector<sns::Position>;

// The board of the cases below, 3 by 3; `B` is the phase's dead letter. Its five
// words, and a path for each:
//
//     P A T      PAT  (0,0) (0,1) (0,2)      TAP  (0,2) (0,1) (0,0)
//     B I N      PIN  (0,0) (1,1) (1,2)      NIP  (1,2) (1,1) (0,0)
//     B B B      PAN  (0,0) (0,1) (1,2)
constexpr std::string_view hand_tiles = "P A T/B I N/B B B";
const Path pat{{0, 0}, {0, 1}, {0, 2}};
const Path pin{{0, 0}, {1, 1}, {1, 2}};

constexpr sns::Day friday = 20742;  // 2026-10-16

sns::BoardRecord hand_record(std::vector<sns::Find> finds = {}) {
    sns::BoardRecord record;
    record.mode = sns::BoardMode::phonics;
    record.phase = "A";
    record.size = BoardSize::three;
    record.tiles = std::string(hand_tiles);
    record.viable = {"NIP", "PAN", "PAT", "PIN", "TAP"};
    record.finds = std::move(finds);
    return record;
}

sns::SessionSnapshot playing_snapshot(std::vector<sns::Find> finds = {}) {
    sns::SessionSnapshot snapshot;
    snapshot.state = SessionState::playing;
    snapshot.mode = sns::BoardMode::phonics;
    snapshot.size = BoardSize::three;
    snapshot.seed = 77;
    snapshot.board = hand_record(std::move(finds));
    return snapshot;
}

// The session these cases play is the phonics one, over the phase above.
sns::SessionData phonics_data() {
    return {.dictionary = english(), .phase = &phase_a()};
}

sns::SessionOptions phonics_options() {
    sns::SessionOptions options;
    options.mode = sns::BoardMode::phonics;
    return options;
}

sns::GameSession restore(const sns::SessionSnapshot& snapshot) {
    auto restored = sns::GameSession::restore(snapshot, phonics_data(), phonics_options());
    REQUIRE(restored.has_value());
    return *std::move(restored);
}

sns::Find jewel(std::string word, bool assisted = false, sns::Stamp when = friday) {
    return {.word = std::move(word), .assisted = assisted, .when = when};
}

}  // namespace

// ---------------------------------------------------------------------------
// A board

TEST_CASE("a board record is a snapshot with the same fields", "[session-store]") {
    const auto snapshot =
        sns::snapshot_of(hand_record({jewel("PAT"), jewel("PIN", true, friday + 1)}));

    REQUIRE(snapshot.has_value());
    CHECK(snapshot->phase == "A");
    CHECK(snapshot->size == 3);
    CHECK(snapshot->tiles == hand_tiles);
    CHECK(snapshot->viable == std::vector<std::string>{"NIP", "PAN", "PAT", "PIN", "TAP"});
    CHECK(snapshot->viable_total == 5);
    REQUIRE(snapshot->finds.size() == 2);
    CHECK(snapshot->finds[0].word == "PAT");
    CHECK_FALSE(snapshot->finds[0].assisted);
    CHECK(snapshot->finds[0].day == friday);
    CHECK(snapshot->finds[1].assisted);
    CHECK(snapshot->finds[1].day == friday + 1);
}

TEST_CASE("the stars of a find are its reward, taken when it goes to the store",
          "[session-store]") {
    // Three letters are a Spark (1 star), four a Power word (2), five and over a Mega (5).
    sns::BoardRecord record = hand_record({jewel("PAT"), jewel("PAIN"), jewel("PAINT")});

    const auto snapshot = sns::snapshot_of(record);

    REQUIRE(snapshot.has_value());
    REQUIRE(snapshot->finds.size() == 3);
    CHECK(snapshot->finds[0].stars == record.finds[0].reward().stars);
    CHECK(snapshot->finds[1].stars == record.finds[1].reward().stars);
    CHECK(snapshot->finds[2].stars == record.finds[2].reward().stars);
    CHECK(snapshot->finds[0].stars == 1);
    CHECK(snapshot->finds[1].stars == 2);
    CHECK(snapshot->finds[2].stars == 5);
}

TEST_CASE("a moment that is not a day cannot go to the store", "[session-store]") {
    CHECK_FALSE(
        sns::snapshot_of(hand_record({jewel("PAT", false, std::int64_t{1} << 40)})).has_value());
    CHECK_FALSE(
        sns::snapshot_of(hand_record({jewel("PAT", false, -(std::int64_t{1} << 40))})).has_value());
    // The ends of a day are days.
    CHECK(sns::snapshot_of(hand_record({jewel("PAT", false, INT32_MAX)})).has_value());
    CHECK(sns::snapshot_of(hand_record({jewel("PAT", false, INT32_MIN)})).has_value());
}

TEST_CASE("a record goes to the store and back as it was", "[session-store]") {
    for (const sns::BoardRecord& record :
         {hand_record(), hand_record({jewel("PAT")}),
          hand_record({jewel("TAP", true, 5), jewel("NIP"), jewel("PAN", false, 6)})}) {
        const auto snapshot = sns::snapshot_of(record);
        REQUIRE(snapshot.has_value());

        const auto back = sns::record_of(*snapshot);

        REQUIRE(back.has_value());
        CHECK(*back == record);
    }
}

TEST_CASE("a board through the store comes back in the session's own form", "[session-store]") {
    // The store writes tiles with commas and words in lower case; the session's
    // restore compares exactly, so the way back puts both right.
    auto store = sns::Store::in_memory();
    const sns::BoardRecord record = hand_record({jewel("PAT"), jewel("PIN")});
    sns::SavedSession saved;
    saved.state = sns::SavedState::playing;
    saved.size = 3;
    saved.seed = 5;
    saved.board = sns::snapshot_of(record);
    REQUIRE(store.save_session(saved));
    REQUIRE(store.session().has_value());
    CHECK(store.session()->board->tiles == "P,A,T/B,I,N/B,B,B");
    CHECK(store.session()->board->viable.front() == "nip");

    const auto back = sns::record_of(*store.session()->board);

    REQUIRE(back.has_value());
    CHECK(back->tiles == hand_tiles);
    CHECK(back->viable == record.viable);
    CHECK(*back == record);
}

TEST_CASE(
    "the words come back in upper case and in alphabetical order whatever order they were kept",
    "[session-store]") {
    sns::BoardSnapshot snapshot = *sns::snapshot_of(hand_record());
    snapshot.viable = {"tap", "Pin", "nip", "PAT", "pan"};

    const auto record = sns::record_of(snapshot);

    REQUIRE(record.has_value());
    CHECK(record->viable == std::vector<std::string>{"NIP", "PAN", "PAT", "PIN", "TAP"});
}

TEST_CASE("a board the session cannot take back is refused", "[session-store]") {
    const sns::BoardSnapshot good = *sns::snapshot_of(hand_record());

    sns::BoardSnapshot seven = good;  // the session is played at 3x3 to 6x6
    seven.size = 7;
    sns::BoardSnapshot zero = good;
    zero.size = 0;
    sns::BoardSnapshot negative = good;
    negative.size = -3;
    sns::BoardSnapshot unlisted = good;  // a board the store held no list of
    unlisted.viable.clear();
    sns::BoardSnapshot partial = good;  // a list that is not the whole of what the board held
    partial.viable_total = 60;
    sns::BoardSnapshot bad_total = good;
    bad_total.viable_total = -1;

    for (const sns::BoardSnapshot& snapshot :
         {seven, zero, negative, unlisted, partial, bad_total}) {
        CHECK_FALSE(sns::record_of(snapshot).has_value());
    }
    // The total may be left as 0: the list is then taken to be all of it.
    sns::BoardSnapshot no_total = good;
    no_total.viable_total = 0;
    CHECK(sns::record_of(no_total).has_value());
}

// ---------------------------------------------------------------------------
// The game in progress

TEST_CASE("a session snapshot is a saved session with the same fields", "[session-store]") {
    sns::SessionSnapshot snapshot = playing_snapshot({jewel("PAT")});
    snapshot.lit = "PIN";

    const auto saved = sns::saved_session_of(snapshot);

    REQUIRE(saved.has_value());
    CHECK(saved->state == sns::SavedState::playing);
    CHECK(saved->size == 3);
    CHECK(saved->seed == 77);
    REQUIRE(saved->board.has_value());
    CHECK(saved->board->finds.size() == 1);
    CHECK(saved->lit == std::optional<std::string>{"pin"});  // lower case, as the store keeps it
}

TEST_CASE("each state the session saves goes to the store and back as it was", "[session-store]") {
    sns::SessionSnapshot box;
    box.state = SessionState::box;
    box.size = BoardSize::four;
    box.seed = 9;

    sns::SessionSnapshot playing = playing_snapshot({jewel("PAT")});
    playing.lit = "PIN";

    sns::SessionSnapshot resting =
        playing_snapshot({jewel("PAT"), jewel("PIN"), jewel("PAN"), jewel("NIP"), jewel("TAP")});
    resting.state = SessionState::resting;

    for (const sns::SessionSnapshot& snapshot : {box, playing, resting}) {
        const auto saved = sns::saved_session_of(snapshot);
        REQUIRE(saved.has_value());

        auto store = sns::Store::in_memory();
        REQUIRE(store.save_session(saved));
        const auto back = sns::session_snapshot_of(*store.session());

        REQUIRE(back.has_value());
        CHECK(*back == snapshot);
    }
}

TEST_CASE("a state the session does not save has no saved session", "[session-store]") {
    for (const SessionState state :
         {SessionState::shaking, SessionState::dropping, SessionState::cleared}) {
        sns::SessionSnapshot snapshot = playing_snapshot();
        snapshot.state = state;
        CHECK_FALSE(sns::saved_session_of(snapshot).has_value());
    }
}

TEST_CASE("a saved session the session cannot take back is refused", "[session-store]") {
    sns::SavedSession good = *sns::saved_session_of(playing_snapshot());

    sns::SavedSession size_seven = good;
    size_seven.size = 7;
    size_seven.board->size = 7;
    sns::SavedSession unlisted = good;
    unlisted.board->viable.clear();

    CHECK(sns::session_snapshot_of(good).has_value());
    CHECK_FALSE(sns::session_snapshot_of(size_seven).has_value());
    CHECK_FALSE(sns::session_snapshot_of(unlisted).has_value());
}

// ---------------------------------------------------------------------------
// With a real session

TEST_CASE("a game in progress goes through the store's file and is brought back by the session",
          "[session-store]") {
    sns::GameSession session = restore(playing_snapshot());
    REQUIRE(session.trace(pat, friday).kind == sns::TraceKind::found);
    REQUIRE(session.request_help().has_value());
    const sns::SessionSnapshot before = session.snapshot();
    REQUIRE(before.lit.has_value());

    // To the store, written to text, read back, and to the session.
    const auto saved = sns::saved_session_of(before);
    REQUIRE(saved.has_value());
    auto store = sns::Store::in_memory();
    REQUIRE(store.save_session(saved));
    const auto parsed = sns::parse_store(sns::serialize_store(store.data()));
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->data.session.has_value());
    const auto after = sns::session_snapshot_of(*parsed->data.session);

    REQUIRE(after.has_value());
    CHECK(*after == before);
    auto restored = sns::GameSession::restore(*after, phonics_data(), phonics_options());
    REQUIRE(restored.has_value());
    CHECK(restored->state() == SessionState::playing);
    CHECK(restored->stars() == session.stars());
    REQUIRE(restored->finds().size() == 1);
    CHECK(restored->finds()[0].word == "PAT");
    CHECK(restored->finds()[0].when == friday);
    CHECK(restored->lit_tile() == session.lit_tile());
}

TEST_CASE("a board the session dealt itself comes back through the store", "[session-store]") {
    auto created = sns::GameSession::create(phonics_data(), phonics_options());
    REQUIRE(created.has_value());
    sns::GameSession session = *std::move(created);
    REQUIRE(session.shake().has_value());
    REQUIRE(session.still());
    REQUIRE(session.landed());
    // One word found by tracing the first the solver reports.
    const auto words = sns::solve(*session.board(), phase_a().dictionary());
    REQUIRE_FALSE(words.empty());
    static_cast<void>(session.trace(words.front().path, friday));
    const sns::SessionSnapshot before = session.snapshot();

    const auto saved = sns::saved_session_of(before);
    REQUIRE(saved.has_value());
    auto store = sns::Store::in_memory();
    REQUIRE(store.save_session(saved));
    const auto after = sns::session_snapshot_of(*store.session());

    REQUIRE(after.has_value());
    CHECK(*after == before);
    CHECK(sns::GameSession::restore(*after, phonics_data(), phonics_options()).has_value());
}

TEST_CASE("the board of a session is recorded once however often it is sent", "[session-store]") {
    sns::GameSession session = restore(playing_snapshot());
    auto store = sns::Store::in_memory();

    const auto send = [&] {
        const auto record = session.record();
        REQUIRE(record.has_value());
        const auto snapshot = sns::snapshot_of(*record);
        REQUIRE(snapshot.has_value());
        return store.record(*snapshot, friday);
    };

    static_cast<void>(session.trace(pat, friday));
    const auto first = send();
    CHECK(first.stored);
    CHECK(first.stars_added == 1);
    CHECK(first.new_words == std::vector<std::string>{"pat"});
    CHECK(send().stars_added == 0);  // the same board again adds nothing

    static_cast<void>(session.trace(pin, friday + 1));
    const auto second = send();
    CHECK(second.stars_added == 1);
    CHECK(second.new_words == std::vector<std::string>{"pin"});

    CHECK(store.data().stars == static_cast<std::uint64_t>(session.stars()));
    REQUIRE(store.data().boards.size() == 1);
    CHECK(store.data().boards.front().viable_total == 5);
    CHECK_FALSE(store.data().boards.front().cleared);
    // Each find on its own day.
    REQUIRE(store.data().finds.size() == 2);
    CHECK(store.data().finds[0].day == friday);
    CHECK(store.data().finds[1].day == friday + 1);
}

TEST_CASE("clearing a session's board is a cleared board in the store, and a best",
          "[session-store]") {
    sns::GameSession session = restore(playing_snapshot());
    auto store = sns::Store::in_memory();
    const auto send = [&] { return store.record(*sns::snapshot_of(*session.record()), friday); };

    for (const sns::FoundWord& word : sns::solve(*session.board(), phase_a().dictionary())) {
        static_cast<void>(session.trace(word.path, friday));
        send();
    }

    REQUIRE(session.state() == SessionState::cleared);
    CHECK(store.data().boards.front().cleared);
    CHECK(store.best_stars(3) == session.stars());
    REQUIRE(store.data().days.size() == 1);
    CHECK(store.data().days.front().boards_cleared == 1);
}

// ---------------------------------------------------------------------------
// The standard board

namespace {

// The same board, with `X` where the phonics board has its dead letter: a
// standard board has no dead jewel. The words of this file's dictionary on it
// are the five that the phase has.
constexpr std::string_view standard_tiles = "P A T/X I N/X X X";

const sns::DiceSets& test_dice() {
    static const sns::DiceSets dice = [] {
        std::vector<sns::DiceSet> sets;
        for (std::size_t side = 3; side <= 6; ++side) {
            std::string text;
            for (std::size_t die = 0; die < side * side; ++die) {
                text += "p a t i n s x\n";
            }
            auto set = sns::DiceSet::parse(text);
            REQUIRE(set.has_value());
            sets.push_back(*std::move(set));
        }
        auto made = sns::DiceSets::make(std::move(sets));
        REQUIRE(made.has_value());
        return *std::move(made);
    }();
    return dice;
}

sns::SessionData standard_data() {
    return {.dictionary = english(), .dice = &test_dice()};
}

sns::BoardRecord standard_record(std::vector<sns::Find> finds = {}) {
    sns::BoardRecord record;
    record.mode = sns::BoardMode::standard;
    record.size = BoardSize::three;
    record.tiles = std::string(standard_tiles);
    record.viable_total = 5;
    record.finds = std::move(finds);
    return record;
}

sns::SessionSnapshot standard_snapshot(std::vector<sns::Find> finds = {}) {
    sns::SessionSnapshot snapshot;
    snapshot.state = SessionState::playing;
    snapshot.mode = sns::BoardMode::standard;
    snapshot.size = BoardSize::three;
    snapshot.seed = 77;
    snapshot.board = standard_record(std::move(finds));
    return snapshot;
}

sns::GameSession restore_standard(const sns::SessionSnapshot& snapshot) {
    auto restored = sns::GameSession::restore(snapshot, standard_data(), {});
    REQUIRE(restored.has_value());
    return *std::move(restored);
}

}  // namespace

TEST_CASE("a standard board's record is a snapshot with no phase and the count of its words",
          "[session-store][standard]") {
    const auto snapshot =
        sns::snapshot_of(standard_record({jewel("PAT"), jewel("PIN", true, friday + 1)}));

    REQUIRE(snapshot.has_value());
    CHECK(snapshot->phase.empty());
    CHECK(snapshot->size == 3);
    CHECK(snapshot->tiles == standard_tiles);
    // The store lists none of a standard board's words and keeps how many there were.
    CHECK(snapshot->viable.empty());
    CHECK(snapshot->viable_total == 5);
    REQUIRE(snapshot->finds.size() == 2);
    CHECK(snapshot->finds[1].assisted);
}

TEST_CASE("a record whose mode and phase disagree cannot go to the store",
          "[session-store][standard]") {
    // The store tells the two boards by the phase, so these would be kept as the other one.
    sns::BoardRecord standard_with_phase = standard_record();
    standard_with_phase.phase = "A";
    sns::BoardRecord phonics_without_phase = hand_record();
    phonics_without_phase.phase.clear();

    CHECK_FALSE(sns::snapshot_of(standard_with_phase).has_value());
    CHECK_FALSE(sns::snapshot_of(phonics_without_phase).has_value());
}

TEST_CASE("a standard record goes to the store and back as it was", "[session-store][standard]") {
    for (const sns::BoardRecord& record :
         {standard_record(), standard_record({jewel("PAT")}),
          standard_record({jewel("TAP", true, 5), jewel("NIP"), jewel("PAN", false, 6)})}) {
        const auto snapshot = sns::snapshot_of(record);
        REQUIRE(snapshot.has_value());

        const auto back = sns::record_of(*snapshot);

        REQUIRE(back.has_value());
        CHECK(back->mode == sns::BoardMode::standard);
        CHECK(*back == record);
    }
}

TEST_CASE("a standard board with neither a count nor a list of its words cannot come back",
          "[session-store][standard]") {
    sns::BoardSnapshot snapshot = *sns::snapshot_of(standard_record());
    CHECK(sns::record_of(snapshot).has_value());

    snapshot.viable_total = 0;
    CHECK_FALSE(sns::record_of(snapshot).has_value());
    snapshot.viable_total = -1;
    CHECK_FALSE(sns::record_of(snapshot).has_value());

    // A list and no count: the list is taken to be all of it, as the store does.
    snapshot.viable_total = 0;
    snapshot.viable = {"NIP", "PAN", "PAT"};
    const auto listed = sns::record_of(snapshot);
    REQUIRE(listed.has_value());
    CHECK(listed->viable_total == 3U);
    CHECK(listed->viable.empty());
}

TEST_CASE("each state of a standard game goes to the saved session and back as it was",
          "[session-store][standard]") {
    sns::SessionSnapshot playing = standard_snapshot({jewel("PAT")});
    playing.lit = "PIN";

    sns::SessionSnapshot resting =
        standard_snapshot({jewel("PAT"), jewel("PIN"), jewel("PAN"), jewel("NIP"), jewel("TAP")});
    resting.state = SessionState::resting;

    for (const sns::SessionSnapshot& snapshot : {playing, resting}) {
        const auto saved = sns::saved_session_of(snapshot);
        REQUIRE(saved.has_value());
        // No phase, no list of words, and how many there were.
        REQUIRE(saved->board.has_value());
        CHECK(saved->board->phase.empty());
        CHECK(saved->board->viable.empty());
        CHECK(saved->board->viable_total == 5);

        const auto back = sns::session_snapshot_of(*saved);

        REQUIRE(back.has_value());
        CHECK(back->mode == sns::BoardMode::standard);
        CHECK(*back == snapshot);
    }
}

TEST_CASE("a game in the box has no board to say which mode it was", "[session-store][standard]") {
    // It comes back with the default, and restore plays the mode it is given.
    sns::SessionSnapshot box;
    box.state = SessionState::box;
    box.mode = sns::BoardMode::phonics;
    box.size = BoardSize::five;
    box.seed = 9;
    const auto saved = sns::saved_session_of(box);
    REQUIRE(saved.has_value());
    auto store = sns::Store::in_memory();
    REQUIRE(store.save_session(saved));
    const auto back = sns::session_snapshot_of(*store.session());
    REQUIRE(back.has_value());
    CHECK(back->mode == sns::BoardMode::standard);
    CHECK(back->size == BoardSize::five);
    CHECK(back->seed == 9U);
}

TEST_CASE("the store keeps a standard game in progress by the count of its words",
          "[session-store][standard]") {
    // A standard board holds tens to hundreds of words and lists none of them:
    // the saved game carries how many there were, which is what the session
    // checks the board against when it comes back.
    const auto saved = sns::saved_session_of(standard_snapshot({jewel("PAT")}));
    REQUIRE(saved.has_value());
    auto store = sns::Store::in_memory();

    REQUIRE(store.save_session(saved));
    REQUIRE(store.session().has_value());
    REQUIRE(store.session()->board.has_value());
    CHECK(store.session()->board->phase.empty());
    CHECK(store.session()->board->viable.empty());
    CHECK(store.session()->board->viable_total == 5);

    // Through the store's file and back: the snapshot is the one that was saved.
    const auto parsed = sns::parse_store(sns::serialize_store(store.data()));
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->data.session.has_value());
    const auto after = sns::session_snapshot_of(*parsed->data.session);

    REQUIRE(after.has_value());
    CHECK(after->mode == sns::BoardMode::standard);
    CHECK(*after == standard_snapshot({jewel("PAT")}));
    auto restored = sns::GameSession::restore(*after, standard_data(), {});
    REQUIRE(restored.has_value());
    CHECK(restored->mode() == sns::BoardMode::standard);
    CHECK(restored->state() == SessionState::playing);
    CHECK(restored->finds().size() == 1);
    CHECK(restored->finds()[0].word == "PAT");
}

TEST_CASE("a standard game's saved session is brought back by the session",
          "[session-store][standard]") {
    sns::GameSession session = restore_standard(standard_snapshot());
    REQUIRE(session.trace(pat, friday).kind == sns::TraceKind::found);
    REQUIRE(session.request_help().has_value());
    const sns::SessionSnapshot before = session.snapshot();
    REQUIRE(before.lit.has_value());

    const auto saved = sns::saved_session_of(before);
    REQUIRE(saved.has_value());
    const auto after = sns::session_snapshot_of(*saved);

    REQUIRE(after.has_value());
    CHECK(*after == before);
    auto restored = sns::GameSession::restore(*after, standard_data(), {});
    REQUIRE(restored.has_value());
    CHECK(restored->mode() == sns::BoardMode::standard);
    CHECK(restored->state() == SessionState::playing);
    CHECK(restored->stars() == session.stars());
    REQUIRE(restored->finds().size() == 1);
    CHECK(restored->finds()[0].word == "PAT");
    CHECK(restored->finds()[0].when == friday);
    CHECK(restored->lit_tile() == session.lit_tile());

    // Under the other switch the same saved game is refused, and the app starts again.
    const auto switched = sns::GameSession::restore(*after, phonics_data(), phonics_options());
    REQUIRE_FALSE(switched.has_value());
    CHECK(switched.error() == sns::SessionError::wrong_mode);
}

TEST_CASE("a standard board the session dealt itself comes back from its saved session",
          "[session-store][standard]") {
    sns::SessionOptions options;
    options.seed = 3;
    auto created = sns::GameSession::create(standard_data(), options);
    REQUIRE(created.has_value());
    sns::GameSession session = *std::move(created);
    REQUIRE(session.shake().has_value());
    REQUIRE(session.still());
    REQUIRE(session.landed());
    const auto words = sns::solve(*session.board(), english());
    REQUIRE_FALSE(words.empty());
    static_cast<void>(session.trace(words.front().path, friday));
    const sns::SessionSnapshot before = session.snapshot();

    const auto saved = sns::saved_session_of(before);
    REQUIRE(saved.has_value());
    const auto after = sns::session_snapshot_of(*saved);

    REQUIRE(after.has_value());
    CHECK(*after == before);
    CHECK(sns::GameSession::restore(*after, standard_data(), {}).has_value());
}

TEST_CASE("a standard board is recorded once however often it is sent, and clearing it is a best",
          "[session-store][standard]") {
    sns::GameSession session = restore_standard(standard_snapshot());
    auto store = sns::Store::in_memory();
    const auto send = [&] { return store.record(*sns::snapshot_of(*session.record()), friday); };

    static_cast<void>(session.trace(pat, friday));
    const auto first = send();
    CHECK(first.stored);
    CHECK(first.stars_added == 1);
    CHECK(send().stars_added == 0);  // the same board again adds nothing
    REQUIRE(store.data().boards.size() == 1);
    // The history keeps the board by its count and none of its words.
    CHECK(store.data().boards.front().phase.empty());
    CHECK(store.data().boards.front().tiles == "P,A,T/X,I,N/X,X,X");
    CHECK(store.data().boards.front().viable_total == 5);
    CHECK(store.data().boards.front().viable.empty());
    CHECK_FALSE(store.data().boards.front().cleared);

    for (const sns::FoundWord& word : sns::solve(*session.board(), english())) {
        static_cast<void>(session.trace(word.path, friday));
        send();
    }
    REQUIRE(session.state() == SessionState::cleared);
    CHECK(store.data().boards.front().cleared);
    CHECK(store.best_stars(3) == session.stars());
}
