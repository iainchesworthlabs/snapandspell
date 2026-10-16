#include "wjs/session_store.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace wjs {

namespace {

[[nodiscard]] std::string upper(std::string_view text) {
    std::string result{text};
    for (char& c : result) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return result;
}

[[nodiscard]] std::string lower(std::string_view text) {
    std::string result{text};
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return result;
}

[[nodiscard]] std::optional<Day> day_of(Stamp stamp) {
    if (stamp < std::numeric_limits<Day>::min() || stamp > std::numeric_limits<Day>::max()) {
        return std::nullopt;
    }
    return static_cast<Day>(stamp);
}

// The session's own text for tiles: a space between tiles, '/' between rows. The
// store writes commas between tiles; text that already has spaces is unchanged.
[[nodiscard]] std::string session_tiles(std::string_view stored) {
    std::string tiles{stored};
    std::replace(tiles.begin(), tiles.end(), ',', ' ');
    return tiles;
}

[[nodiscard]] std::optional<SavedState> saved_state_of(SessionState state) {
    switch (state) {
        case SessionState::box:
            return SavedState::box;
        case SessionState::playing:
            return SavedState::playing;
        case SessionState::resting:
            return SavedState::resting;
        case SessionState::shaking:
        case SessionState::dropping:
        case SessionState::cleared:
            return std::nullopt;
    }
    return std::nullopt;
}

[[nodiscard]] SessionState session_state_of(SavedState state) {
    switch (state) {
        case SavedState::box:
            return SessionState::box;
        case SavedState::playing:
            return SessionState::playing;
        case SavedState::resting:
            return SessionState::resting;
    }
    return SessionState::box;
}

}  // namespace

std::optional<BoardSnapshot> snapshot_of(const BoardRecord& record) {
    BoardSnapshot snapshot;
    snapshot.phase = record.phase;
    snapshot.size = static_cast<int>(side(record.size));
    snapshot.tiles = record.tiles;
    snapshot.viable = record.viable;
    snapshot.viable_total = static_cast<int>(record.viable.size());
    for (const Find& find : record.finds) {
        const std::optional<Day> day = day_of(find.when);
        if (!day) {
            return std::nullopt;
        }
        snapshot.finds.push_back({find.word, find.assisted, *day, find.reward().stars});
    }
    return snapshot;
}

std::optional<BoardRecord> record_of(const BoardSnapshot& snapshot) {
    if (snapshot.size < 0) {
        return std::nullopt;
    }
    const std::optional<BoardSize> size = board_size_for(static_cast<std::size_t>(snapshot.size));
    // The session needs every word the board held, and the store lists them only
    // up to a bound: a board whose list is not the whole of it cannot be brought back.
    if (!size || snapshot.viable.empty() ||
        (snapshot.viable_total != 0 &&
         static_cast<std::size_t>(snapshot.viable_total) != snapshot.viable.size())) {
        return std::nullopt;
    }

    BoardRecord record;
    record.phase = snapshot.phase;
    record.size = *size;
    record.tiles = session_tiles(snapshot.tiles);
    for (const std::string& word : snapshot.viable) {
        record.viable.push_back(upper(word));
    }
    std::sort(record.viable.begin(), record.viable.end());
    for (const FoundSnapshot& find : snapshot.finds) {
        record.finds.push_back({upper(find.word), find.assisted, static_cast<Stamp>(find.day)});
    }
    return record;
}

std::optional<SavedSession> saved_session_of(const SessionSnapshot& snapshot) {
    const std::optional<SavedState> state = saved_state_of(snapshot.state);
    if (!state) {
        return std::nullopt;
    }
    SavedSession saved;
    saved.state = *state;
    saved.size = static_cast<int>(side(snapshot.size));
    saved.seed = snapshot.seed;
    if (snapshot.board) {
        saved.board = snapshot_of(*snapshot.board);
        if (!saved.board) {
            return std::nullopt;
        }
    }
    if (snapshot.lit) {
        saved.lit = lower(*snapshot.lit);
    }
    return saved;
}

std::optional<SessionSnapshot> session_snapshot_of(const SavedSession& saved) {
    if (saved.size < 0) {
        return std::nullopt;
    }
    const std::optional<BoardSize> size = board_size_for(static_cast<std::size_t>(saved.size));
    if (!size) {
        return std::nullopt;
    }
    SessionSnapshot snapshot;
    snapshot.state = session_state_of(saved.state);
    snapshot.size = *size;
    snapshot.seed = saved.seed;
    if (saved.board) {
        snapshot.board = record_of(*saved.board);
        if (!snapshot.board) {
            return std::nullopt;
        }
    }
    if (saved.lit) {
        snapshot.lit = upper(*saved.lit);
    }
    return snapshot;
}

}  // namespace wjs
