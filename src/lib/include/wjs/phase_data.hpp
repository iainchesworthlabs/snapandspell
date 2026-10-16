#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <istream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "wjs/dictionary.hpp"

// Reading and checking the data of a phonics phase: which graphemes a phase
// teaches and which words it offers, each split into graphemes. The library
// holds no phase of its own and knows no letter, sound or word of one; what is
// here is the format, the checks and the cumulative view a board is made from.
// The files are written and reviewed by a person qualified to do it.
//
// Two kinds of file, one line a grapheme or a word, `#` lines being the header
// or a comment, blank lines skipped:
//
//     # phase: A
//     # reviewed-by: <name>   reviewed-on: <date>   source: <where it came from>
//
//     data/phonics/phase-<x>.graphemes.txt    ai  /sound/  clip:ai
//     data/phonics/phase-<x>.words.txt        pain  p ai n
//                                             <word>  tricky
//
// A file with no reviewed-by and reviewed-on is test data (see
// PhaseHeader::reviewed); a release build refuses to carry one.

namespace wjs {

// A word of fewer letters than this has no tier and can never be counted, so a
// words file may not hold one. It is the solver's default minimum length.
inline constexpr std::size_t min_phase_word_letters = 3;

// One line of a graphemes file.
struct Grapheme {
    std::string letters;  // lower-case a-z, as written
    std::string sound;    // as written, slashes included
    std::string clip;     // the clip id, without the "clip:" prefix
    std::size_t line = 0;
};

// One line of a words file.
struct PhaseWord {
    std::string word;                    // lower-case a-z, as written
    std::vector<std::string> graphemes;  // each one's letters, in order; none if tricky
    bool tricky = false;                 // spoken whole, never sounded out
    std::size_t line = 0;
};

// What a file says about itself in its `#` lines.
struct PhaseHeader {
    std::string phase;        // upper-case, "A"
    std::string reviewed_by;  // empty when absent or still a <placeholder>
    std::string reviewed_on;
    std::string source;

    // Both reviewed-by and reviewed-on are given and reviewed-on is a date
    // written YYYY-MM-DD. Anything less is test data.
    [[nodiscard]] bool reviewed() const noexcept;
};

struct GraphemesFile {
    std::string name;  // what the file was called when it was read
    PhaseHeader header;
    std::vector<Grapheme> graphemes;  // sorted by letters, each once
};

struct WordsFile {
    std::string name;
    PhaseHeader header;
    std::vector<PhaseWord> words;  // sorted by word, each once
};

// One thing wrong with a file. `line` is 1-based, or 0 for the file as a whole.
struct DataProblem {
    std::string file;
    std::size_t line = 0;
    std::string message;
};

using DataProblems = std::vector<DataProblem>;

// "file:line: message", or "file: message" for the file as a whole.
[[nodiscard]] std::string to_string(const DataProblem& problem);

// Parsing holds a file to its own shape: the header, the line grammar, no
// word or grapheme twice, sorted. Every problem is reported, not the first.
// `name` only labels the problems.
[[nodiscard]] std::expected<GraphemesFile, DataProblems> parse_graphemes(std::istream& in,
                                                                         std::string_view name);
[[nodiscard]] std::expected<WordsFile, DataProblems> parse_words(std::istream& in,
                                                                 std::string_view name);

// The words and graphemes of one phase and of every phase before it: phases
// are cumulative, so a word is in a phase when all its graphemes are.
class PhaseIndex {
public:
    [[nodiscard]] const std::string& phase() const noexcept { return phase_; }

    // Sorted by letters.
    [[nodiscard]] const std::vector<Grapheme>& graphemes() const noexcept { return graphemes_; }

    // Sorted by word.
    [[nodiscard]] const std::vector<PhaseWord>& words() const noexcept { return words_; }

    // The words as a dictionary, which is what a board is solved against.
    [[nodiscard]] const Dictionary& dictionary() const noexcept { return dictionary_; }

    // Case-insensitive; null when the phase has no such word or grapheme.
    [[nodiscard]] const PhaseWord* find_word(std::string_view word) const;
    [[nodiscard]] const Grapheme* find_grapheme(std::string_view letters) const;

    // The letters, A-Z, of every grapheme and every word of the phase, sorted
    // and each once. A letter outside this is one the phase does not use.
    [[nodiscard]] const std::string& alphabet() const noexcept { return alphabet_; }

private:
    friend class PhaseSet;

    std::string phase_;
    std::vector<Grapheme> graphemes_;
    std::vector<PhaseWord> words_;
    Dictionary dictionary_;
    std::string alphabet_;
};

// Every phase of a data set, checked against the English dictionary: each word
// is in it, each word's graphemes spell it, each grapheme is in the word's phase
// or an earlier one, and nothing is listed twice across phases.
class PhaseSet {
public:
    // Phases are ordered by name (A before B), and a phase may have either file
    // or both. `english` is the dictionary the solver accepts.
    [[nodiscard]] static std::expected<PhaseSet, DataProblems> build(
        const std::vector<GraphemesFile>& graphemes, const std::vector<WordsFile>& words,
        const Dictionary& english);

    // Reads phase-<x>.graphemes.txt and phase-<x>.words.txt from a directory
    // and builds from them. A `.txt` file of another name, an unreadable file
    // and a header that names another phase than the file name are problems.
    [[nodiscard]] static std::expected<PhaseSet, DataProblems> load_directory(
        const std::filesystem::path& directory, const Dictionary& english);

    // In order. Each is cumulative.
    [[nodiscard]] std::span<const PhaseIndex> phases() const noexcept { return phases_; }

    // By name, case-insensitive; null when there is no such phase.
    [[nodiscard]] const PhaseIndex* find(std::string_view phase) const;

    // Every file was read with a complete review header, and there was a file.
    [[nodiscard]] bool reviewed() const noexcept { return file_count_ > 0 && unreviewed_.empty(); }

    // The names of the files that are test data.
    [[nodiscard]] const std::vector<std::string>& unreviewed_files() const noexcept {
        return unreviewed_;
    }

private:
    PhaseSet() = default;

    std::vector<PhaseIndex> phases_;
    std::vector<std::string> unreviewed_;
    std::size_t file_count_ = 0;
};

}  // namespace wjs
