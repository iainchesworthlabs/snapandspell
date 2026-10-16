#include "wjs/game_session.hpp"

#include <algorithm>
#include <random>
#include <utility>

#include "wjs/path.hpp"
#include "wjs/profiling.hpp"

namespace wjs {
namespace {

// The seed after `seed`: the first output of std::mt19937 seeded with it, which
// the standard fixes, so a run of boards is the same on every compiler.
[[nodiscard]] std::uint32_t next_seed(std::uint32_t seed) {
    std::mt19937 engine(seed);
    return static_cast<std::uint32_t>(engine());
}

// How far a count of words is from the range: zero inside it.
[[nodiscard]] std::size_t distance_from_range(std::size_t count, std::size_t low,
                                              std::size_t high) noexcept {
    if (count < low) {
        return low - count;
    }
    return count > high ? count - high : 0;
}

// The board as Board::parse reads it back: a space between tiles, '/' between
// rows.
[[nodiscard]] std::string tiles_text(const Board& board) {
    std::string text;
    for (std::size_t row = 0; row < board.rows(); ++row) {
        if (row > 0) {
            text += '/';
        }
        for (std::size_t column = 0; column < board.columns(); ++column) {
            if (column > 0) {
                text += ' ';
            }
            text += board.tile({row, column});
        }
    }
    return text;
}

// Whether two words are the same, whatever their case (ASCII letters).
[[nodiscard]] bool same_word(std::string_view lhs, std::string_view rhs) noexcept {
    const auto upper = [](char c) {
        return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    };
    return std::ranges::equal(lhs, rhs, [&](char a, char b) { return upper(a) == upper(b); });
}

[[nodiscard]] std::vector<std::string> sorted_words(std::span<const FoundWord> viable) {
    std::vector<std::string> words;
    words.reserve(viable.size());
    for (const FoundWord& found : viable) {
        words.push_back(found.word);
    }
    std::ranges::sort(words);
    return words;
}

// The options of a board of this size, with the session's range and focus.
[[nodiscard]] std::expected<SeedOptions, SeedError> seed_options(const PhaseIndex& phase,
                                                                 const SessionOptions& options,
                                                                 BoardSize size) {
    auto seed = phonics_board_options(phase, side(size));
    if (seed) {
        seed->min_viable = options.min_viable;
        seed->max_viable = options.max_viable;
        seed->focus_sounds = options.focus_sounds;
    }
    return seed;
}

}  // namespace

std::string_view to_string(SessionState state) noexcept {
    switch (state) {
        case SessionState::box:
            return "solo.box";
        case SessionState::shaking:
            return "solo.shaking";
        case SessionState::dropping:
            return "solo.dropping";
        case SessionState::playing:
            return "solo.playing";
        case SessionState::cleared:
            return "solo.cleared";
        case SessionState::resting:
            return "solo.resting";
    }
    return "unknown session state";
}

std::string_view to_string(TraceKind kind) noexcept {
    switch (kind) {
        case TraceKind::ignored:
            return "ignored";
        case TraceKind::found:
            return "found";
        case TraceKind::found_again:
            return "found again";
        case TraceKind::real_not_todays:
            return "a real word, not one of today's";
        case TraceKind::not_a_word:
            return "not a word";
    }
    return "unknown trace kind";
}

std::string_view to_string(SessionError error) noexcept {
    switch (error) {
        case SessionError::wrong_state:
            return "the event does not apply in this state";
        case SessionError::bad_options:
            return "the range of words is empty, or there are no seedings";
        case SessionError::bad_phase:
            return "the phase cannot make a board";
        case SessionError::no_board:
            return "no board could be dealt";
        case SessionError::bad_snapshot:
            return "the snapshot is not one a session makes";
        case SessionError::wrong_phase:
            return "the snapshot is of another phase";
        case SessionError::board_changed:
            return "the phase's words on the board are not the ones recorded";
    }
    return "unknown session error";
}

GameSession::GameSession(const PhaseIndex& phase, const Dictionary& dictionary,
                         SessionOptions options)
    : phase_(&phase),
      dictionary_(&dictionary),
      options_(std::move(options)),
      size_(options_.size),
      seed_(options_.seed) {}

std::expected<GameSession, SessionError> GameSession::create(const PhaseIndex& phase,
                                                             const Dictionary& dictionary,
                                                             const SessionOptions& options) {
    if (options.min_viable > options.max_viable || options.max_viable == 0 ||
        options.max_seedings == 0) {
        return std::unexpected(SessionError::bad_options);
    }
    // Both sizes have to be makeable, so that a size chosen on the box never
    // fails for a reason that was there from the start.
    for (const BoardSize size : {BoardSize::three, BoardSize::four}) {
        if (!seed_options(phase, options, size)) {
            return std::unexpected(SessionError::bad_phase);
        }
    }
    return GameSession(phase, dictionary, options);
}

const Board* GameSession::board() const noexcept {
    return (state_ == SessionState::box || !board_) ? nullptr : &*board_;
}

std::span<const Position> GameSession::dead_tiles() const noexcept {
    return state_ == SessionState::box ? std::span<const Position>{} : std::span(dead_);
}

std::span<const Find> GameSession::finds() const noexcept {
    return state_ == SessionState::box ? std::span<const Find>{} : std::span(finds_);
}

int GameSession::stars() const noexcept {
    return state_ == SessionState::box ? 0 : stars_;
}

std::optional<Position> GameSession::lit_tile() const {
    if (!lit_) {
        return std::nullopt;
    }
    for (const FoundWord& word : viable_) {
        if (word.word == *lit_ && !word.path.empty()) {
            return word.path.front();
        }
    }
    return std::nullopt;
}

bool GameSession::choose_size(BoardSize size) {
    if (state_ != SessionState::box) {
        return false;
    }
    size_ = size;
    return true;
}

std::expected<void, SessionError> GameSession::shake() {
    if (state_ != SessionState::box) {
        return std::unexpected(SessionError::wrong_state);
    }
    auto zone = profiling::zone<"board.deal">();

    const auto seed_option = seed_options(*phase_, options_, size_);
    if (!seed_option) {
        return std::unexpected(SessionError::bad_phase);
    }

    // Seed again with the next seed of the stream until a board lands in range,
    // and keep the nearest if none does. The deal is a pure function of the
    // stream, the size and the recent targets, so nothing is changed until it is
    // done: a refusal leaves the session in the box.
    std::optional<SeededBoard> best;
    std::size_t best_distance = 0;
    DealReport report;
    std::uint32_t seed = seed_;
    for (std::size_t seeding = 0; seeding < options_.max_seedings; ++seeding) {
        auto seeded = seed_board(*phase_, *seed_option, recent_, seed);
        if (!seeded) {
            return std::unexpected(SessionError::no_board);
        }
        ++report.seedings;
        report.tries += seeded->tries;
        report.solves += seeded->solves;
        const std::size_t distance =
            distance_from_range(seeded->viable.size(), options_.min_viable, options_.max_viable);
        const std::uint32_t used = seed;
        seed = next_seed(seed);
        if (!best || distance < best_distance) {
            best_distance = distance;
            report.seed = used;
            best = *std::move(seeded);
        }
        if (distance == 0) {
            break;
        }
    }
    // A board that holds no word is never dealt. The seeding plants its targets,
    // so this is a seeding that failed to keep them, which it does not do.
    if (!best || best->viable.empty()) {
        return std::unexpected(SessionError::no_board);
    }

    report.viable = best->viable.size();
    report.in_range = best_distance == 0;
    zone.Value(report.seedings);

    recent_.record(best->targets);
    seed_ = seed;
    board_ = std::move(best->board);
    dead_ = std::move(best->dead);
    viable_ = std::move(best->viable);
    found_ = FoundWords{};
    finds_.clear();
    stars_ = 0;
    lit_.reset();
    played_ = false;
    report_ = report;
    state_ = SessionState::shaking;
    return {};
}

bool GameSession::still() {
    if (state_ != SessionState::shaking) {
        return false;
    }
    state_ = SessionState::dropping;
    return true;
}

bool GameSession::landed() {
    if (state_ != SessionState::dropping) {
        return false;
    }
    state_ = SessionState::playing;
    played_ = true;
    return true;
}

const Find* GameSession::find_of(std::string_view word) const {
    const auto it = std::ranges::find(finds_, word, &Find::word);
    return it == finds_.end() ? nullptr : &*it;
}

TraceOutcome GameSession::trace(std::span<const Position> path, Stamp when) {
    // A touch on one tile is to hear its letter, and a trace cancelled by lifting
    // outside the board is nothing: neither is a word attempt.
    if (state_ != SessionState::playing || path.size() < 2) {
        return {};
    }
    auto zone = profiling::zone<"input.submit">();
    zone.Value(path.size());

    const WordSets words = WordSets::phonics(*dictionary_, *phase_);
    const Classification classified = classify_path(*board_, path, words);
    switch (classified.state) {
        case WordState::viable: {
            if (const Find* earlier = find_of(classified.word)) {
                return {.kind = TraceKind::found_again, .find = *earlier, .cleared = false};
            }
            // The rules say a viable path's word is one the solver finds, so it is
            // among viable_; insert says whether it is new, and it is.
            found_.insert(classified.word);
            Find find{.word = classified.word, .assisted = false, .when = when};
            if (lit_ && *lit_ == find.word) {
                find.assisted = true;
                lit_.reset();
            }
            stars_ += find.reward().stars;
            finds_.push_back(find);

            const bool cleared = is_cleared(viable_, found_);
            if (cleared) {
                state_ = SessionState::cleared;
                lit_.reset();
            }
            return {.kind = TraceKind::found, .find = std::move(find), .cleared = cleared};
        }
        case WordState::real_not_todays:
            return {.kind = TraceKind::real_not_todays, .find = std::nullopt, .cleared = false};
        case WordState::not_a_word:
            break;
    }

    // The rules call a word of fewer than three letters "not a word", because it
    // cannot count. It can still be a real one ("at"), and a child who is told a
    // real word is not one has been taught something false, so a real word is
    // answered as a real word that is not today's, whatever its length. Nothing
    // is named either way.
    if (!classified.fault) {
        const auto spelled = validate_path(*board_, path);
        if (spelled && dictionary_->contains(*spelled)) {
            return {.kind = TraceKind::real_not_todays, .find = std::nullopt, .cleared = false};
        }
    }
    return {.kind = TraceKind::not_a_word, .find = std::nullopt, .cleared = false};
}

std::optional<Position> GameSession::request_help() {
    if (state_ != SessionState::playing || !options_.help) {
        return std::nullopt;
    }
    const auto help = start_tile_help(viable_, found_);
    if (!help) {
        return std::nullopt;
    }
    lit_ = help->word;
    return help->tile;
}

bool GameSession::rest() {
    if (state_ != SessionState::cleared) {
        return false;
    }
    state_ = SessionState::resting;
    return true;
}

bool GameSession::again() {
    if (state_ != SessionState::resting) {
        return false;
    }
    state_ = SessionState::box;
    return true;
}

bool GameSession::back() {
    if (state_ != SessionState::shaking && state_ != SessionState::dropping &&
        state_ != SessionState::playing) {
        return false;
    }
    lit_.reset();
    state_ = SessionState::box;
    return true;
}

BoardRecord GameSession::make_record() const {
    BoardRecord record;
    record.phase = phase_->phase();
    record.size = size_;
    record.tiles = tiles_text(*board_);
    record.viable = sorted_words(viable_);
    record.finds = finds_;
    return record;
}

std::optional<BoardRecord> GameSession::record() const {
    if (!played_) {
        return std::nullopt;
    }
    return make_record();
}

SessionSnapshot GameSession::snapshot() const {
    SessionSnapshot snapshot;
    snapshot.size = size_;
    snapshot.seed = seed_;
    switch (state_) {
        case SessionState::box:
        case SessionState::shaking:
        case SessionState::dropping:
            // A board the child has not seen is not kept: the seed has moved on,
            // so the next shake deals another.
            snapshot.state = SessionState::box;
            break;
        case SessionState::playing:
            snapshot.state = SessionState::playing;
            snapshot.board = make_record();
            snapshot.lit = lit_;
            break;
        case SessionState::cleared:
        case SessionState::resting:
            snapshot.state = SessionState::resting;
            snapshot.board = make_record();
            break;
    }
    return snapshot;
}

std::expected<GameSession, SessionError> GameSession::restore(const SessionSnapshot& snapshot,
                                                              const PhaseIndex& phase,
                                                              const Dictionary& dictionary,
                                                              const SessionOptions& options) {
    auto zone = profiling::zone<"board.restore">();

    auto created = create(phase, dictionary, options);
    if (!created) {
        return std::unexpected(created.error());
    }
    GameSession session = *std::move(created);
    session.size_ = snapshot.size;
    session.seed_ = snapshot.seed;

    if (snapshot.state == SessionState::box) {
        if (snapshot.board || snapshot.lit) {
            return std::unexpected(SessionError::bad_snapshot);
        }
        return session;
    }
    if ((snapshot.state != SessionState::playing && snapshot.state != SessionState::resting) ||
        !snapshot.board) {
        return std::unexpected(SessionError::bad_snapshot);
    }

    const BoardRecord& record = *snapshot.board;
    if (record.phase != phase.phase()) {
        return std::unexpected(SessionError::wrong_phase);
    }
    if (record.size != snapshot.size) {
        return std::unexpected(SessionError::bad_snapshot);
    }

    auto parsed = Board::parse(record.tiles);
    if (!parsed || parsed->rows() != side(record.size) || parsed->columns() != side(record.size)) {
        return std::unexpected(SessionError::bad_snapshot);
    }
    const WordSets words = WordSets::phonics(dictionary, phase);
    std::vector<FoundWord> viable = viable_words(*parsed, words);
    if (viable.empty() || sorted_words(viable) != record.viable) {
        return std::unexpected(SessionError::board_changed);
    }

    // The finds are words of the board, each once.
    FoundWords found;
    std::vector<Find> finds;
    int stars = 0;
    for (const Find& find : record.finds) {
        const auto on_board = std::ranges::find_if(
            viable, [&](const FoundWord& word) { return same_word(word.word, find.word); });
        if (on_board == viable.end() || !found.insert(find.word)) {
            return std::unexpected(SessionError::bad_snapshot);
        }
        Find kept = find;
        kept.word = on_board->word;
        stars += kept.reward().stars;
        finds.push_back(std::move(kept));
    }
    const bool cleared = is_cleared(viable, found);
    if (cleared != (snapshot.state == SessionState::resting)) {
        return std::unexpected(SessionError::bad_snapshot);
    }

    std::optional<std::string> lit;
    if (snapshot.lit) {
        const auto on_board = std::ranges::find_if(
            viable, [&](const FoundWord& word) { return same_word(word.word, *snapshot.lit); });
        if (snapshot.state != SessionState::playing || on_board == viable.end() ||
            found.contains(on_board->word)) {
            return std::unexpected(SessionError::bad_snapshot);
        }
        lit = on_board->word;
    }

    // A dead jewel is a tile of the letter the phase leaves out.
    const auto seed_option = seed_options(phase, options, record.size);
    const std::string dead_letter(1, seed_option->dead_letter);
    for (std::size_t row = 0; row < parsed->rows(); ++row) {
        for (std::size_t column = 0; column < parsed->columns(); ++column) {
            if (parsed->tile({row, column}) == dead_letter) {
                session.dead_.push_back({row, column});
            }
        }
    }

    session.board_ = *std::move(parsed);
    session.viable_ = std::move(viable);
    session.found_ = std::move(found);
    session.finds_ = std::move(finds);
    session.stars_ = stars;
    session.lit_ = std::move(lit);
    session.played_ = true;
    session.state_ = snapshot.state;
    return session;
}

}  // namespace wjs
