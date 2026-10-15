#include "wjs/solver.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <unordered_map>
#include <utility>

namespace wjs {
namespace {

struct Offset {
    std::ptrdiff_t row;
    std::ptrdiff_t column;
};

constexpr std::array<Offset, 8> neighbours = {{
    {-1, -1},
    {-1, 0},
    {-1, 1},
    {0, -1},
    {0, 1},
    {1, -1},
    {1, 0},
    {1, 1},
}};

// A depth-first walk of the board that carries a position in the dictionary's
// trie alongside the path, so a branch that no word can continue is dropped as
// soon as it is known, rather than after the letters have been collected.
class Search {
public:
    Search(const Board& board, const Dictionary& dictionary, const SolveOptions& options)
        : board_(board),
          dictionary_(dictionary),
          options_(options),
          visited_(board.rows() * board.columns(), false) {}

    [[nodiscard]] std::vector<FoundWord> run() {
        for (std::size_t row = 0; row < board_.rows(); ++row) {
            for (std::size_t column = 0; column < board_.columns(); ++column) {
                extend(Position{row, column}, Dictionary::root);
            }
        }

        std::vector<FoundWord> result;
        result.reserve(found_.size());
        for (auto& entry : found_) {
            result.push_back(std::move(entry.second));
        }
        std::ranges::sort(result, [](const FoundWord& lhs, const FoundWord& rhs) {
            if (lhs.score != rhs.score) {
                return lhs.score > rhs.score;
            }
            if (lhs.word.size() != rhs.word.size()) {
                return lhs.word.size() > rhs.word.size();
            }
            return lhs.word < rhs.word;
        });
        return result;
    }

private:
    void extend(Position at, Dictionary::Cursor from) {
        const std::string& tile = board_.tile(at);

        Dictionary::Cursor cursor = from;
        for (const char letter : tile) {
            cursor = dictionary_.step(cursor, letter);
            if (cursor == Dictionary::no_node) {
                return;
            }
        }

        const std::size_t index = at.row * board_.columns() + at.column;
        visited_[index] = true;
        word_ += tile;
        path_.push_back(at);

        if (dictionary_.is_word(cursor) && word_.size() >= options_.min_length) {
            record();
        }

        for (const Offset offset : neighbours) {
            const auto row = static_cast<std::ptrdiff_t>(at.row) + offset.row;
            const auto column = static_cast<std::ptrdiff_t>(at.column) + offset.column;
            if (row < 0 || column < 0) {
                continue;
            }
            const Position next{static_cast<std::size_t>(row), static_cast<std::size_t>(column)};
            if (!board_.contains(next) || visited_[next.row * board_.columns() + next.column]) {
                continue;
            }
            extend(next, cursor);
        }

        path_.pop_back();
        word_.resize(word_.size() - tile.size());
        visited_[index] = false;
    }

    void record() {
        if (found_.contains(word_)) {
            return;
        }
        found_.emplace(word_, FoundWord{word_, path_, score_word(word_, options_.scoring)});
    }

    const Board& board_;
    const Dictionary& dictionary_;
    const SolveOptions& options_;

    std::vector<bool> visited_;
    std::string word_;
    std::vector<Position> path_;
    std::unordered_map<std::string, FoundWord> found_;
};

}  // namespace

std::vector<FoundWord> solve(const Board& board, const Dictionary& dictionary,
                             const SolveOptions& options) {
    return Search(board, dictionary, options).run();
}

}  // namespace wjs
