#include "wjs/seeding.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <utility>

namespace wjs {
namespace {

// A board is made around two or three words of the phase (phonics design,
// "Seeding", step 1).
constexpr std::size_t min_targets = 2;
constexpr std::size_t max_targets = 3;

// Search steps one planting may spend on one word before it gives the word up,
// so that a crowded board cannot stall. The count is of steps, not time, so it
// is the same on every machine.
constexpr std::size_t plant_step_budget = 20000;

[[nodiscard]] constexpr char to_upper(char c) noexcept {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

[[nodiscard]] constexpr bool is_letter(char c) noexcept {
    return c >= 'A' && c <= 'Z';
}

[[nodiscard]] std::string upper(std::string_view text) {
    std::string result(text);
    std::ranges::transform(result, result.begin(), to_upper);
    return result;
}

// The only source of randomness: std::mt19937, whose output is fixed by the
// standard, taken modulo. A draw is never part of a larger expression, so the
// order the draws are made in does not depend on the compiler.
class Draw {
public:
    explicit Draw(std::uint32_t seed) : engine_(seed) {}

    // 0 to n - 1. n must be above 0.
    [[nodiscard]] std::size_t below(std::size_t n) {
        const auto value = static_cast<std::uint32_t>(engine_());
        return static_cast<std::size_t>(value % static_cast<std::uint32_t>(n));
    }

    template <typename T>
    void shuffle(std::span<T> items) {
        for (std::size_t i = items.size(); i > 1; --i) {
            const std::size_t j = below(i);
            std::swap(items[i - 1], items[j]);
        }
    }

private:
    std::mt19937 engine_;
};

// The tiles of a board, row by row. A tile that holds nothing yet is 0.
struct Grid {
    std::size_t size = 0;
    std::vector<char> cells;

    [[nodiscard]] std::size_t free_count() const {
        return static_cast<std::size_t>(std::ranges::count(cells, '\0'));
    }

    // The tiles that touch `cell`, in a fixed order.
    [[nodiscard]] std::size_t neighbours(std::size_t cell, std::array<std::size_t, 8>& out) const {
        const std::size_t row = cell / size;
        const std::size_t column = cell % size;
        std::size_t count = 0;
        for (std::size_t r = row > 0 ? row - 1 : 0; r <= row + 1 && r < size; ++r) {
            for (std::size_t c = column > 0 ? column - 1 : 0; c <= column + 1 && c < size; ++c) {
                if (r != row || c != column) {
                    out[count++] = r * size + c;
                }
            }
        }
        return count;
    }
};

// Writes a word into a grid as a path of touching tiles. A tile that already
// holds the word's letter there may be shared, and a free tile may be taken
// only while more than `reserve` free tiles remain, so the room the dead
// jewels need is left.
class Planter {
public:
    Planter(Grid& grid, Draw& draw, std::size_t reserve)
        : grid_(grid),
          draw_(draw),
          reserve_(reserve),
          free_(grid.free_count()),
          on_path_(grid.cells.size(), false) {}

    // Leaves the grid as it was and returns false if no path fits.
    [[nodiscard]] bool plant(std::string_view word) {
        word_ = word;
        steps_ = 0;
        std::vector<std::size_t> starts(grid_.cells.size());
        std::iota(starts.begin(), starts.end(), std::size_t{0});
        draw_.shuffle(std::span(starts));
        for (const std::size_t start : starts) {
            if (usable(start, 0) && walk(start, 0)) {
                return true;
            }
            if (steps_ > plant_step_budget) {
                break;
            }
        }
        return false;
    }

private:
    [[nodiscard]] bool usable(std::size_t cell, std::size_t at) const {
        if (on_path_[cell]) {
            return false;
        }
        const char held = grid_.cells[cell];
        if (held == '\0') {
            return free_ > reserve_;
        }
        return held == word_[at];
    }

    // Takes `cell` for letter `at` and carries on from it. On success the
    // letters stay in the grid; on failure everything this call did is undone.
    bool walk(std::size_t cell, std::size_t at) {
        ++steps_;
        const char previous = grid_.cells[cell];
        if (previous == '\0') {
            --free_;
        }
        grid_.cells[cell] = word_[at];
        on_path_[cell] = true;

        bool placed = at + 1 == word_.size();
        if (!placed && steps_ <= plant_step_budget) {
            std::array<std::size_t, 8> next{};
            const std::size_t count = grid_.neighbours(cell, next);
            draw_.shuffle(std::span(next.data(), count));
            for (std::size_t i = 0; i < count && !placed; ++i) {
                if (usable(next[i], at + 1)) {
                    placed = walk(next[i], at + 1);
                }
            }
        }

        on_path_[cell] = false;
        if (!placed) {
            grid_.cells[cell] = previous;
            if (previous == '\0') {
                ++free_;
            }
        }
        return placed;
    }

    Grid& grid_;
    Draw& draw_;
    std::size_t reserve_;
    std::size_t free_;
    std::vector<bool> on_path_;
    std::string word_;
    std::size_t steps_ = 0;
};

// A word of the phase that fits the board.
struct Candidate {
    const PhaseWord* word = nullptr;
    std::string letters;  // upper-case
    bool focus = false;
};

// One board and what solving it said.
struct Attempt {
    std::vector<char> letters;                // row by row
    std::vector<std::size_t> filler;          // tiles that are neither planted nor dead
    std::vector<std::size_t> target_indices;  // into the candidates
    std::vector<Position> dead;
    std::vector<FoundWord> viable;
    std::size_t distance = 0;  // how far the count of words is outside the range
};

[[nodiscard]] std::string board_text(std::span<const char> letters, std::size_t size) {
    std::string text;
    text.reserve(letters.size() * 2);
    for (std::size_t row = 0; row < size; ++row) {
        if (row > 0) {
            text += '/';
        }
        for (std::size_t column = 0; column < size; ++column) {
            if (column > 0) {
                text += ' ';
            }
            text += letters[row * size + column];
        }
    }
    return text;
}

}  // namespace

void RecentTargets::record(const std::vector<std::string>& targets) {
    if (capacity_ == 0) {
        return;
    }
    boards_.push_back(targets);
    while (boards_.size() > capacity_) {
        boards_.pop_front();
    }
}

bool RecentTargets::contains(std::string_view word) const {
    return std::ranges::any_of(boards_, [&](const std::vector<std::string>& board) {
        return std::ranges::any_of(board, [&](const std::string& target) {
            return std::ranges::equal(target, word,
                                      [](char a, char b) { return to_upper(a) == to_upper(b); });
        });
    });
}

std::string_view to_string(SeedError error) noexcept {
    switch (error) {
        case SeedError::bad_size:
            return "the board is not between 1 and 16 tiles each way";
        case SeedError::bad_range:
            return "the range of viable words is empty or no tries are allowed";
        case SeedError::empty_index:
            return "no word of the phase fits the board";
        case SeedError::bad_dead_letter:
            return "the dead letter is not A-Z, or a word of the phase uses it";
        case SeedError::bad_filler:
            return "the filler is empty, not A-Z, or holds the dead letter";
    }
    return "unknown seeding error";
}

std::expected<SeededBoard, SeedError> seed_board(const PhaseIndex& index,
                                                 const SeedOptions& options,
                                                 const RecentTargets& recent, std::uint32_t seed) {
    // --- the options -----------------------------------------------------------
    if (options.size == 0 || options.size > Board::max_dimension) {
        return std::unexpected(SeedError::bad_size);
    }
    if (options.max_viable == 0 || options.min_viable > options.max_viable ||
        options.max_tries == 0) {
        return std::unexpected(SeedError::bad_range);
    }

    const std::string& alphabet = index.alphabet();
    const char dead_letter = to_upper(options.dead_letter);
    if (options.dead_cells > 0) {
        if (!is_letter(dead_letter) || alphabet.find(dead_letter) != std::string::npos) {
            return std::unexpected(SeedError::bad_dead_letter);
        }
    }

    const std::string filler = options.filler.empty() ? alphabet : upper(options.filler);
    if (filler.empty() || !std::ranges::all_of(filler, is_letter) ||
        (options.dead_cells > 0 && filler.find(dead_letter) != std::string::npos)) {
        return std::unexpected(SeedError::bad_filler);
    }

    // --- the words that can be planted -----------------------------------------
    const std::size_t cell_count = options.size * options.size;

    std::vector<Candidate> candidates;
    std::size_t shortest = cell_count;
    for (const PhaseWord& word : index.words()) {
        if (word.word.size() > cell_count) {
            continue;
        }
        Candidate candidate;
        candidate.word = &word;
        candidate.letters = upper(word.word);
        for (const WordGrapheme& grapheme : word.graphemes) {
            const Grapheme* known = index.sound_of(grapheme);
            if (known != nullptr && std::ranges::find(options.focus_sounds, known->sound) !=
                                        options.focus_sounds.end()) {
                candidate.focus = true;
            }
        }
        shortest = std::min(shortest, word.word.size());
        candidates.push_back(std::move(candidate));
    }
    if (candidates.empty()) {
        return std::unexpected(SeedError::empty_index);
    }

    // No more dead jewels than leave room for the shortest word.
    const std::size_t dead_target = std::min(options.dead_cells, cell_count - shortest);

    SolveOptions solve_options;
    solve_options.min_length = min_phase_word_letters;

    Draw draw(seed);
    std::size_t tries = 0;
    std::optional<Attempt> best;

    const auto verify = [&](Attempt& attempt) -> bool {
        const auto board = Board::parse(board_text(attempt.letters, options.size));
        // The letters are A-Z in a square of at most 16: it parses.
        attempt.viable =
            board ? solve(*board, index.dictionary(), solve_options) : std::vector<FoundWord>{};
        ++tries;

        const std::size_t count = attempt.viable.size();
        attempt.distance = count < options.min_viable   ? options.min_viable - count
                           : count > options.max_viable ? count - options.max_viable
                                                        : 0;
        // A board that lost a target is worse than any that kept them all.
        const bool kept = std::ranges::all_of(attempt.target_indices, [&](std::size_t target) {
            return std::ranges::any_of(attempt.viable, [&](const FoundWord& found) {
                return found.word == candidates[target].letters;
            });
        });
        if (!kept) {
            attempt.distance += cell_count * cell_count;
        }

        if (!best || attempt.distance < best->distance) {
            best = attempt;
        }
        return attempt.distance == 0;
    };

    // --- make boards until one lands in range, or the tries are spent -----------
    bool accepted = false;
    while (!accepted && tries < options.max_tries) {
        // Choose and plant the targets. The pools, in the order they are drawn
        // from: words with a focus sound that are not recent, those that are,
        // then the same two for words with none.
        std::array<std::vector<std::size_t>, 4> pools;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            const bool is_recent = recent.contains(candidates[i].word->word);
            const std::size_t which = (candidates[i].focus ? 0U : 2U) + (is_recent ? 1U : 0U);
            pools[which].push_back(i);
        }

        Grid grid;
        grid.size = options.size;
        grid.cells.assign(cell_count, '\0');
        Planter planter(grid, draw, dead_target);

        const std::size_t wanted =
            std::min({min_targets + draw.below(max_targets - min_targets + 1), options.max_viable,
                      candidates.size()});

        Attempt attempt;
        while (attempt.target_indices.size() < wanted) {
            // Once the focus is being honoured it stays honoured: a third
            // word is taken from the focus pools or not at all. The first two
            // are taken from any pool, in order.
            const bool focus_only =
                !options.focus_sounds.empty() && attempt.target_indices.size() >= min_targets &&
                std::ranges::all_of(attempt.target_indices,
                                    [&](std::size_t target) { return candidates[target].focus; });
            const auto last = pools.begin() + (focus_only ? 2 : 4);
            auto from = std::find_if(pools.begin(), last, [](const auto& p) { return !p.empty(); });
            if (from == last) {
                break;
            }
            const std::size_t at = draw.below(from->size());
            const std::size_t chosen = (*from)[at];
            from->erase(from->begin() + static_cast<std::ptrdiff_t>(at));
            if (planter.plant(candidates[chosen].letters)) {
                attempt.target_indices.push_back(chosen);
            }
        }
        if (attempt.target_indices.empty()) {
            // Cannot happen on an empty grid with a word that fits, but a board
            // with nothing planted is not one to offer.
            break;
        }

        // Dead jewels among the tiles the words left, then the filler.
        std::vector<std::size_t> open;
        for (std::size_t i = 0; i < cell_count; ++i) {
            if (grid.cells[i] == '\0') {
                open.push_back(i);
            }
        }
        draw.shuffle(std::span(open));
        const std::size_t dead_count = std::min(dead_target, open.size());
        std::vector<std::size_t> dead_cells(open.begin(),
                                            open.begin() + static_cast<std::ptrdiff_t>(dead_count));
        std::ranges::sort(dead_cells);
        attempt.filler.assign(open.begin() + static_cast<std::ptrdiff_t>(dead_count), open.end());
        std::ranges::sort(attempt.filler);

        for (const std::size_t cell : dead_cells) {
            grid.cells[cell] = dead_letter;
            attempt.dead.push_back(Position{cell / options.size, cell % options.size});
        }
        for (const std::size_t cell : attempt.filler) {
            grid.cells[cell] = filler[draw.below(filler.size())];
        }
        attempt.letters = grid.cells;

        accepted = verify(attempt);

        // Then change one filler tile at a time, which moves the count by a
        // word or two and costs one solve, before starting over.
        for (std::size_t change = 0; !accepted && change < options.mutations_per_plant &&
                                     tries < options.max_tries && !attempt.filler.empty();
             ++change) {
            const std::size_t cell = attempt.filler[draw.below(attempt.filler.size())];
            char letter = filler[draw.below(filler.size())];
            for (std::size_t retry = 0; retry < 8 && letter == attempt.letters[cell]; ++retry) {
                letter = filler[draw.below(filler.size())];
            }
            attempt.letters[cell] = letter;
            accepted = verify(attempt);
        }
    }

    if (!best) {
        return std::unexpected(SeedError::empty_index);
    }

    auto board = Board::parse(board_text(best->letters, options.size));
    if (!board) {
        return std::unexpected(SeedError::bad_size);
    }

    SeededBoard result{
        *std::move(board), {}, std::move(best->viable), std::move(best->dead), tries, false, false};
    std::size_t with_focus = 0;
    for (const std::size_t target : best->target_indices) {
        result.targets.push_back(candidates[target].word->word);
        if (candidates[target].focus) {
            ++with_focus;
        }
    }
    result.fallback = best->distance != 0;
    result.focus_unmet = !options.focus_sounds.empty() && with_focus < result.targets.size();
    return result;
}

}  // namespace wjs
