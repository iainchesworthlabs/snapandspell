#include "wjs/phase_data.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <system_error>
#include <utility>

namespace wjs {
namespace {

constexpr std::string_view byte_order_mark = "\xEF\xBB\xBF";
constexpr std::string_view tricky_marker = "tricky";
constexpr std::string_view clip_prefix = "clip:";

// The keys a `#` line may carry, each as `key: value`. Several may share a line.
constexpr std::array<std::string_view, 4> header_keys = {"phase", "reviewed-by", "reviewed-on",
                                                         "source"};
constexpr std::size_t key_phase = 0;
constexpr std::size_t key_reviewed_by = 1;
constexpr std::size_t key_reviewed_on = 2;
constexpr std::size_t key_source = 3;

[[nodiscard]] constexpr bool is_blank(char c) noexcept {
    return c == ' ' || c == '\t';
}

[[nodiscard]] constexpr char to_upper(char c) noexcept {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

[[nodiscard]] constexpr char to_lower(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

[[nodiscard]] constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] std::string upper(std::string_view text) {
    std::string result(text);
    std::ranges::transform(result, result.begin(), to_upper);
    return result;
}

[[nodiscard]] std::string lower(std::string_view text) {
    std::string result(text);
    std::ranges::transform(result, result.begin(), to_lower);
    return result;
}

[[nodiscard]] std::string_view trim(std::string_view text) noexcept {
    while (!text.empty() && (is_blank(text.front()) || text.front() == '\r')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (is_blank(text.back()) || text.back() == '\r')) {
        text.remove_suffix(1);
    }
    return text;
}

[[nodiscard]] std::vector<std::string_view> fields_of(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (start < line.size()) {
        if (is_blank(line[start])) {
            ++start;
            continue;
        }
        std::size_t end = start;
        while (end < line.size() && !is_blank(line[end])) {
            ++end;
        }
        fields.push_back(line.substr(start, end - start));
        start = end;
    }
    return fields;
}

[[nodiscard]] bool is_lower_letters(std::string_view text) noexcept {
    return !text.empty() && std::ranges::all_of(text, [](char c) { return c >= 'a' && c <= 'z'; });
}

[[nodiscard]] bool is_phase_name(std::string_view text) noexcept {
    return !text.empty() && std::ranges::all_of(text, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_digit(c);
    });
}

[[nodiscard]] bool is_clip_id(std::string_view text) noexcept {
    return !text.empty() && std::ranges::all_of(text, [](char c) {
        return (c >= 'a' && c <= 'z') || is_digit(c) || c == '.' || c == '_' || c == '-';
    });
}

// `/.../`: slashes round at least one character, none inside.
[[nodiscard]] bool is_sound(std::string_view text) noexcept {
    return text.size() >= 3 && text.front() == '/' && text.back() == '/' &&
           text.substr(1, text.size() - 2).find('/') == std::string_view::npos;
}

// YYYY-MM-DD, and a day that the month has.
[[nodiscard]] bool is_iso_date(std::string_view text) noexcept {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return false;
    }
    for (const std::size_t i : {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U}) {
        if (!is_digit(text[i])) {
            return false;
        }
    }
    const auto number = [&](std::size_t at, std::size_t count) {
        int value = 0;
        for (std::size_t i = at; i < at + count; ++i) {
            value = value * 10 + (text[i] - '0');
        }
        return value;
    };
    const int year = number(0, 4);
    const int month = number(5, 2);
    const int day = number(8, 2);
    if (year < 1 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    constexpr std::array<int, 12> days = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    return day <= days[static_cast<std::size_t>(month - 1)] + ((leap && month == 2) ? 1 : 0);
}

// A value still written as the template's <placeholder> has not been given.
[[nodiscard]] bool is_placeholder(std::string_view value) noexcept {
    return value.size() >= 2 && value.front() == '<' && value.back() == '>';
}

class Report {
public:
    Report(std::string_view file, DataProblems& problems) : file_(file), problems_(problems) {}

    void operator()(std::size_t line, std::string message) const {
        problems_.push_back(DataProblem{file_, line, std::move(message)});
    }

private:
    std::string file_;
    DataProblems& problems_;
};

struct HeaderSeen {
    std::array<bool, header_keys.size()> seen{};
};

// One `#` line, with the leading `#` already removed.
void read_header_line(std::string_view comment, std::size_t line, PhaseHeader& header,
                      HeaderSeen& seen, const Report& report) {
    struct Hit {
        std::size_t at;
        std::size_t key;
    };
    std::vector<Hit> hits;
    for (std::size_t i = 0; i < comment.size(); ++i) {
        if (i > 0 && !is_blank(comment[i - 1])) {
            continue;
        }
        for (std::size_t key = 0; key < header_keys.size(); ++key) {
            const auto name = header_keys[key];
            if (comment.substr(i).starts_with(name) && comment.size() > i + name.size() &&
                comment[i + name.size()] == ':') {
                hits.push_back(Hit{i, key});
                break;
            }
        }
    }

    for (std::size_t h = 0; h < hits.size(); ++h) {
        const auto name = header_keys[hits[h].key];
        const std::size_t start = hits[h].at + name.size() + 1;
        const std::size_t end = h + 1 < hits.size() ? hits[h + 1].at : comment.size();
        auto value = trim(comment.substr(start, end - start));

        if (seen.seen[hits[h].key]) {
            report(line, "'" + std::string(name) + ":' is given twice");
            continue;
        }
        seen.seen[hits[h].key] = true;
        if (is_placeholder(value)) {
            value = {};
        }

        switch (hits[h].key) {
            case key_phase:
                if (!is_phase_name(value)) {
                    report(line, "the phase must be letters and digits, as in '# phase: A'");
                } else {
                    header.phase = upper(value);
                }
                break;
            case key_reviewed_by:
                header.reviewed_by = value;
                break;
            case key_reviewed_on:
                header.reviewed_on = value;
                break;
            case key_source:
                header.source = value;
                break;
            default:
                break;
        }
    }
}

// What the header as a whole has to say, once every line has been read.
void check_header(const PhaseHeader& header, const HeaderSeen& seen, const Report& report) {
    if (!seen.seen[key_phase]) {
        report(0, "no '# phase: <name>' header");
    }
    const bool by = !header.reviewed_by.empty();
    const bool on = !header.reviewed_on.empty();
    if (by != on) {
        report(0, "give both reviewed-by and reviewed-on, or neither (neither is test data)");
    }
    if (on && !is_iso_date(header.reviewed_on)) {
        report(0, "reviewed-on must be a date written YYYY-MM-DD");
    }
}

// The lines of a file, with the byte-order mark and the carriage returns of a
// Windows editor taken off. `#` lines go to the header; the rest, trimmed and
// not blank, go to `visit(line_number, text)`.
template <typename Visit>
void read_file(std::istream& in, PhaseHeader& header, const Report& report, Visit&& visit) {
    HeaderSeen seen;
    std::string raw;
    std::size_t number = 0;
    while (std::getline(in, raw)) {
        ++number;
        std::string_view text = raw;
        if (number == 1 && text.starts_with(byte_order_mark)) {
            text.remove_prefix(byte_order_mark.size());
        }
        text = trim(text);
        if (text.empty()) {
            continue;
        }
        if (text.front() == '#') {
            read_header_line(text.substr(1), number, header, seen, report);
            continue;
        }
        visit(number, text);
    }
    check_header(header, seen, report);
}

[[nodiscard]] std::string in_quotes(std::string_view text) {
    return "'" + std::string(text) + "'";
}

// What a file name says about the phase and the kind of file it is.
struct FileName {
    std::string phase;
    bool graphemes = false;
};

[[nodiscard]] std::optional<FileName> read_file_name(std::string_view name) {
    constexpr std::string_view prefix = "phase-";
    constexpr std::string_view graphemes_suffix = ".graphemes.txt";
    constexpr std::string_view words_suffix = ".words.txt";
    if (!name.starts_with(prefix)) {
        return std::nullopt;
    }
    name.remove_prefix(prefix.size());
    FileName result;
    if (name.ends_with(graphemes_suffix)) {
        name.remove_suffix(graphemes_suffix.size());
        result.graphemes = true;
    } else if (name.ends_with(words_suffix)) {
        name.remove_suffix(words_suffix.size());
    } else {
        return std::nullopt;
    }
    if (!is_phase_name(name)) {
        return std::nullopt;
    }
    result.phase = upper(name);
    return result;
}

// Where a word or a grapheme was first seen, for "already listed in ...".
struct Place {
    std::string file;
    std::size_t line = 0;
};

[[nodiscard]] std::string described(const Place& place) {
    return place.file + ":" + std::to_string(place.line);
}

}  // namespace

bool PhaseHeader::reviewed() const noexcept {
    return !reviewed_by.empty() && is_iso_date(reviewed_on);
}

std::string to_string(const DataProblem& problem) {
    if (problem.line == 0) {
        return problem.file + ": " + problem.message;
    }
    return problem.file + ":" + std::to_string(problem.line) + ": " + problem.message;
}

std::expected<GraphemesFile, DataProblems> parse_graphemes(std::istream& in,
                                                           std::string_view name) {
    GraphemesFile file;
    file.name = std::string(name);
    DataProblems problems;
    const Report report(name, problems);

    std::map<std::string, std::size_t> first_line;
    std::string previous;

    read_file(in, file.header, report, [&](std::size_t line, std::string_view text) {
        const auto fields = fields_of(text);
        if (fields.size() != 3) {
            report(line,
                   "a grapheme line is the letters, the sound and the clip: "
                   "'ai  /sound/  clip:ai'");
            return;
        }
        const auto letters = fields[0];
        const auto sound = fields[1];
        const auto clip = fields[2];

        bool good = true;
        if (!is_lower_letters(letters)) {
            report(line, in_quotes(letters) +
                             " is not lowercase a-z letters (a split digraph "
                             "cannot be written yet)");
            good = false;
        }
        if (!is_sound(sound)) {
            report(line, in_quotes(sound) + " is not a sound written between slashes");
            good = false;
        }
        if (!clip.starts_with(clip_prefix) || !is_clip_id(clip.substr(clip_prefix.size()))) {
            report(line, in_quotes(clip) + " is not a clip id written 'clip:<id>'");
            good = false;
        }
        if (!good) {
            return;
        }

        const std::string key(letters);
        if (const auto found = first_line.find(key); found != first_line.end()) {
            report(line, in_quotes(letters) + " is listed twice (first on line " +
                             std::to_string(found->second) + ")");
            return;
        }
        if (!previous.empty() && previous > key) {
            report(line,
                   in_quotes(letters) + " is out of order (after " + in_quotes(previous) + ")");
        }
        previous = key;
        first_line.emplace(key, line);
        file.graphemes.push_back(
            Grapheme{key, std::string(sound), std::string(clip.substr(clip_prefix.size())), line});
    });

    if (!problems.empty()) {
        return std::unexpected(std::move(problems));
    }
    return file;
}

std::expected<WordsFile, DataProblems> parse_words(std::istream& in, std::string_view name) {
    WordsFile file;
    file.name = std::string(name);
    DataProblems problems;
    const Report report(name, problems);

    std::map<std::string, std::size_t> first_line;
    std::string previous;

    read_file(in, file.header, report, [&](std::size_t line, std::string_view text) {
        const auto fields = fields_of(text);
        const auto word = fields.front();

        if (!is_lower_letters(word)) {
            report(line, in_quotes(word) + " is not a lowercase a-z word");
            return;
        }
        if (word.size() < min_phase_word_letters) {
            report(line, in_quotes(word) + " has fewer than " +
                             std::to_string(min_phase_word_letters) +
                             " letters, so no board can count it");
            return;
        }

        PhaseWord entry;
        entry.word = std::string(word);
        entry.line = line;

        if (fields.size() == 1) {
            report(line, in_quotes(word) +
                             " has no graphemes: write them after the word, "
                             "or mark the word 'tricky'");
            return;
        }
        if (fields.size() == 2 && fields[1] == tricky_marker) {
            entry.tricky = true;
        } else {
            bool good = true;
            for (const auto grapheme : std::span(fields).subspan(1)) {
                if (grapheme == tricky_marker) {
                    report(line, in_quotes(word) + ": 'tricky' stands alone, with no split");
                    good = false;
                    break;
                }
                if (!is_lower_letters(grapheme)) {
                    report(line, in_quotes(grapheme) + " in " + in_quotes(word) +
                                     " is not lowercase a-z letters (a split digraph "
                                     "cannot be written yet)");
                    good = false;
                    break;
                }
                entry.graphemes.emplace_back(grapheme);
            }
            if (!good) {
                return;
            }
        }

        if (const auto found = first_line.find(entry.word); found != first_line.end()) {
            report(line, in_quotes(word) + " is listed twice (first on line " +
                             std::to_string(found->second) + ")");
            return;
        }
        if (!previous.empty() && previous > entry.word) {
            report(line, in_quotes(word) + " is out of order (after " + in_quotes(previous) + ")");
        }
        previous = entry.word;
        first_line.emplace(entry.word, line);
        file.words.push_back(std::move(entry));
    });

    if (!problems.empty()) {
        return std::unexpected(std::move(problems));
    }
    return file;
}

const PhaseWord* PhaseIndex::find_word(std::string_view word) const {
    const std::string key = lower(word);
    const auto found = std::ranges::lower_bound(words_, key, {}, &PhaseWord::word);
    return (found != words_.end() && found->word == key) ? &*found : nullptr;
}

const Grapheme* PhaseIndex::find_grapheme(std::string_view letters) const {
    const std::string key = lower(letters);
    const auto found = std::ranges::lower_bound(graphemes_, key, {}, &Grapheme::letters);
    return (found != graphemes_.end() && found->letters == key) ? &*found : nullptr;
}

const PhaseIndex* PhaseSet::find(std::string_view phase) const {
    const std::string key = upper(phase);
    const auto found = std::ranges::find_if(
        phases_, [&](const PhaseIndex& index) { return index.phase() == key; });
    return found == phases_.end() ? nullptr : &*found;
}

std::expected<PhaseSet, DataProblems> PhaseSet::build(const std::vector<GraphemesFile>& graphemes,
                                                      const std::vector<WordsFile>& words,
                                                      const Dictionary& english) {
    DataProblems problems;

    // At most one file of each kind for a phase.
    std::map<std::string, const GraphemesFile*> grapheme_files;
    std::map<std::string, const WordsFile*> word_files;
    std::set<std::string> names;
    for (const auto& file : graphemes) {
        names.insert(file.header.phase);
        if (!grapheme_files.emplace(file.header.phase, &file).second) {
            problems.push_back({file.name, 0,
                                "phase " + file.header.phase +
                                    " has a second graphemes file (the first is " +
                                    grapheme_files.at(file.header.phase)->name + ")"});
        }
    }
    for (const auto& file : words) {
        names.insert(file.header.phase);
        if (!word_files.emplace(file.header.phase, &file).second) {
            problems.push_back({file.name, 0,
                                "phase " + file.header.phase +
                                    " has a second words file (the first is " +
                                    word_files.at(file.header.phase)->name + ")"});
        }
    }

    PhaseSet result;
    std::map<std::string, std::pair<Grapheme, Place>> known_graphemes;  // by letters
    std::map<std::string, std::pair<PhaseWord, Place>> known_words;     // by word

    for (const auto& phase : names) {
        if (const auto found = grapheme_files.find(phase); found != grapheme_files.end()) {
            const auto& file = *found->second;
            for (const auto& grapheme : file.graphemes) {
                const Place here{file.name, grapheme.line};
                const auto [at, added] =
                    known_graphemes.emplace(grapheme.letters, std::pair{grapheme, here});
                if (!added) {
                    problems.push_back({file.name, grapheme.line,
                                        in_quotes(grapheme.letters) + " is already defined (" +
                                            described(at->second.second) + ")"});
                }
            }
        }

        if (const auto found = word_files.find(phase); found != word_files.end()) {
            const auto& file = *found->second;
            for (const auto& entry : file.words) {
                const auto where = [&](std::string message) {
                    problems.push_back({file.name, entry.line, std::move(message)});
                };
                if (!english.contains(entry.word)) {
                    where(in_quotes(entry.word) + " is not in the dictionary");
                }
                if (!entry.tricky) {
                    std::string spelt;
                    for (const auto& grapheme : entry.graphemes) {
                        spelt += grapheme;
                        if (!known_graphemes.contains(grapheme)) {
                            where("grapheme " + in_quotes(grapheme) + " of " +
                                  in_quotes(entry.word) + " is not in phase " + phase +
                                  " or an earlier phase");
                        }
                    }
                    if (spelt != entry.word) {
                        where("the graphemes of " + in_quotes(entry.word) + " spell " +
                              in_quotes(spelt));
                    }
                }
                const Place here{file.name, entry.line};
                const auto [at, added] = known_words.emplace(entry.word, std::pair{entry, here});
                if (!added) {
                    where(in_quotes(entry.word) + " is already listed (" +
                          described(at->second.second) + ")");
                }
            }
        }

        // The index of this phase: everything known so far, which is every
        // earlier phase and this one. std::map keeps both sorted by key.
        PhaseIndex index;
        index.phase_ = phase;
        std::set<char> letters;
        for (const auto& [key, entry] : known_graphemes) {
            index.graphemes_.push_back(entry.first);
            for (const char c : key) {
                letters.insert(to_upper(c));
            }
        }
        for (const auto& [key, entry] : known_words) {
            index.words_.push_back(entry.first);
            index.dictionary_.insert(key);
            for (const char c : key) {
                letters.insert(to_upper(c));
            }
        }
        index.alphabet_.assign(letters.begin(), letters.end());
        result.phases_.push_back(std::move(index));
    }

    if (!problems.empty()) {
        return std::unexpected(std::move(problems));
    }

    for (const auto& file : graphemes) {
        ++result.file_count_;
        if (!file.header.reviewed()) {
            result.unreviewed_.push_back(file.name);
        }
    }
    for (const auto& file : words) {
        ++result.file_count_;
        if (!file.header.reviewed()) {
            result.unreviewed_.push_back(file.name);
        }
    }
    return result;
}

std::expected<PhaseSet, DataProblems> PhaseSet::load_directory(
    const std::filesystem::path& directory, const Dictionary& english) {
    DataProblems problems;
    const auto fail = [&](const std::string& file, std::string message) {
        problems.push_back({file, 0, std::move(message)});
    };

    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        fail(directory.generic_string(), "no such directory");
        return std::unexpected(std::move(problems));
    }

    std::vector<std::filesystem::path> paths;
    std::filesystem::directory_iterator entry(directory, error);
    for (; !error && entry != std::filesystem::directory_iterator(); entry.increment(error)) {
        if (entry->is_regular_file(error) && entry->path().extension() == ".txt") {
            paths.push_back(entry->path());
        }
    }
    if (error) {
        fail(directory.generic_string(), "the directory could not be read");
        return std::unexpected(std::move(problems));
    }
    std::ranges::sort(paths);

    std::vector<GraphemesFile> grapheme_files;
    std::vector<WordsFile> word_files;
    for (const auto& path : paths) {
        const std::string name = path.filename().string();
        const auto kind = read_file_name(name);
        if (!kind) {
            fail(name, "not named phase-<x>.graphemes.txt or phase-<x>.words.txt");
            continue;
        }
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open()) {
            fail(name, "could not be opened");
            continue;
        }
        if (kind->graphemes) {
            auto parsed = parse_graphemes(stream, name);
            if (!parsed) {
                problems.insert(problems.end(), parsed.error().begin(), parsed.error().end());
                continue;
            }
            if (parsed->header.phase != kind->phase) {
                fail(name, "its header says phase " + parsed->header.phase +
                               ", its name says phase " + kind->phase);
                continue;
            }
            grapheme_files.push_back(std::move(*parsed));
        } else {
            auto parsed = parse_words(stream, name);
            if (!parsed) {
                problems.insert(problems.end(), parsed.error().begin(), parsed.error().end());
                continue;
            }
            if (parsed->header.phase != kind->phase) {
                fail(name, "its header says phase " + parsed->header.phase +
                               ", its name says phase " + kind->phase);
                continue;
            }
            word_files.push_back(std::move(*parsed));
        }
    }
    if (!problems.empty()) {
        return std::unexpected(std::move(problems));
    }
    return build(grapheme_files, word_files, english);
}

}  // namespace wjs
