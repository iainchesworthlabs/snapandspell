// SPDX-License-Identifier: GPL-3.0-or-later
#include "assist_game.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <utility>

#include "sns/profiling.hpp"
#include "sns/session_store.hpp"

namespace sns::app {
namespace {

[[nodiscard]] std::string lower(std::string word) {
    std::ranges::transform(word, word.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return word;
}

[[nodiscard]] std::string upper(std::string_view word) {
    std::string out(word);
    std::ranges::transform(out, out.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

[[nodiscard]] sns::Tier tier_of(std::string_view word) noexcept {
    return sns::tier_for_letters(word.size()).value_or(sns::Tier::spark);
}

// Every word of the word set that the board holds: the one long piece of work a
// confirmed board starts, and the reason "Yes, play" is not instant.
[[nodiscard]] std::vector<sns::FoundWord> solve_board(const sns::Board& board,
                                                      const sns::WordSets& words) {
    const auto zone = sns::profiling::zone<"assist.solve">();
    return sns::viable_words(board, words);
}

}  // namespace

AssistGame::AssistGame(sns::Board board, const sns::Dictionary& dictionary, sns::Store& store,
                       std::function<sns::Day()> today)
    : AssistGame(std::move(board), dictionary, dictionary, store, std::move(today)) {}

AssistGame::AssistGame(sns::Board board, const sns::Dictionary& dictionary,
                       const sns::Dictionary& active, sns::Store& store,
                       std::function<sns::Day()> today)
    : board_(std::move(board)),
      dictionary_(&dictionary),
      active_(&active),
      store_(&store),
      today_(std::move(today)),
      viable_(solve_board(board_, words())) {}

sns::WordSets AssistGame::words() const noexcept {
    return {*dictionary_, *active_};
}

std::string AssistGame::tiles_text() const {
    std::string text;
    for (std::size_t row = 0; row < board_.rows(); ++row) {
        if (row > 0) {
            text.push_back('/');
        }
        for (std::size_t column = 0; column < board_.columns(); ++column) {
            if (column > 0) {
                text.push_back(' ');
            }
            text += board_.tile({row, column});
        }
    }
    return text;
}

const AssistFound* AssistGame::tray_word(std::string_view word) const {
    const std::string wanted = lower(std::string(word));
    for (const AssistFound& found : found_) {
        if (found.word == wanted) {
            return &found;
        }
    }
    return nullptr;
}

AssistTraceOutcome AssistGame::submit(std::span<const sns::Position> path) {
    AssistTraceOutcome outcome;
    const sns::Classification classified = sns::classify_path(board_, path, words());
    switch (classified.state) {
        case sns::WordState::viable: {
            outcome.kind = AssistWordKind::viable;
            outcome.alreadyFound = !found_set_.insert(classified.word);
            if (outcome.alreadyFound) {
                if (const AssistFound* existing = tray_word(classified.word)) {
                    outcome.found = *existing;
                }
                return outcome;
            }
            AssistFound found;
            found.word = lower(classified.word);
            found.tier = tier_of(classified.word);
            found.stars = sns::reward_for(found.tier).stars;
            found.assisted = lit_word_ && *lit_word_ == classified.word;
            outcome.found = found;
            found_.push_back(found);
            // A shown start is gone once its word is found; another word found
            // while it is lit leaves it where it was.
            if (found.assisted) {
                lit_word_.reset();
            }
            return outcome;
        }
        case sns::WordState::real_not_todays:
            outcome.kind = AssistWordKind::realNotTodays;
            return outcome;
        case sns::WordState::not_a_word:
            outcome.kind = AssistWordKind::notAWord;
            return outcome;
    }
    return outcome;
}

std::optional<sns::Position> AssistGame::showStart() {
    const std::optional<sns::StartTileHelp> help = sns::start_tile_help(viable_, found_set_);
    if (!help) {
        lit_word_.reset();
        return std::nullopt;
    }
    lit_word_ = help->word;
    return help->tile;
}

std::optional<sns::Position> AssistGame::litTile() const {
    if (!lit_word_) {
        return std::nullopt;
    }
    // The design keeps the light until that word is found. The answer depends
    // only on the words left, so asking again names the same word while it is
    // still there, and the next one once it is not.
    const std::optional<sns::StartTileHelp> help = sns::start_tile_help(viable_, found_set_);
    if (!help || help->word != *lit_word_) {
        return std::nullopt;
    }
    return help->tile;
}

bool AssistGame::helpAllowed() const {
    return store_->settings().help != sns::HelpLevel::off;
}

int AssistGame::stars() const noexcept {
    int total = 0;
    for (const AssistFound& found : found_) {
        total += found.stars;
    }
    return total;
}

std::vector<AssistHistoryEntry> AssistGame::history() const {
    const sns::StoreData& data = store_->data();
    std::vector<AssistHistoryEntry> entries;
    entries.reserve(data.boards.size());
    for (auto stored = data.boards.rbegin(); stored != data.boards.rend(); ++stored) {
        const auto parsed = sns::Board::parse(stored->tiles);
        if (!parsed) {
            continue;
        }
        AssistHistoryEntry entry;
        entry.rows = parsed->rows();
        entry.columns = parsed->columns();
        entry.tiles.reserve(entry.rows * entry.columns);
        for (std::size_t row = 0; row < entry.rows; ++row) {
            for (std::size_t column = 0; column < entry.columns; ++column) {
                entry.tiles.push_back(parsed->tile({row, column}));
            }
        }
        for (const sns::FindRecord& find : sns::finds_on_board(data, stored->serial)) {
            AssistFound found;
            found.word = find.word;  // the store keeps its words lower case
            found.tier = tier_of(find.word);
            found.stars = find.stars;
            found.assisted = find.start_tile_used;
            entry.found.push_back(std::move(found));
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

void AssistGame::leave() {
    if (recorded_) {
        return;
    }
    recorded_ = true;

    // The store keeps a board by its side, 3x3 to 6x6, with the tiles parseable
    // and the count of the words it held. A board that is not such a square has
    // no form in the history and is not recorded: the check screen only confirms
    // one that does.
    const std::optional<sns::BoardSize> size = sns::board_size_for(board_.rows());
    if (!size || board_.rows() != board_.columns()) {
        return;
    }

    const sns::Day day = today_ ? today_() : sns::Day{0};
    sns::BoardRecord record;
    record.mode = sns::BoardMode::standard;
    record.phase.clear();
    record.size = *size;
    record.tiles = tiles_text();
    // A standard board lists none of its words: the count is what the store
    // keeps, and what the dashboard counts.
    record.viable.clear();
    record.viable_total = viable_.size();
    record.finds.reserve(found_.size());
    for (const AssistFound& found : found_) {
        sns::Find find;
        find.word = upper(found.word);
        find.assisted = found.assisted;
        find.when = day;
        record.finds.push_back(std::move(find));
    }

    if (const std::optional<sns::BoardSnapshot> snapshot = sns::snapshot_of(record)) {
        const sns::RecordOutcome outcome = store_->record(*snapshot, day);
        save_failed_ = !outcome.stored || store_->last_save_error().has_value();
    }
}

}  // namespace sns::app
