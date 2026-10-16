#include "wjs/dictionary.hpp"

#include <algorithm>
#include <fstream>
#include <optional>
#include <string>

#include "wjs/profiling.hpp"

namespace wjs {
namespace {

[[nodiscard]] constexpr char to_upper(char c) noexcept {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

// The word upper-cased, or nothing if it is empty or holds a non-letter.
[[nodiscard]] std::optional<std::string> normalise(std::string_view word) {
    if (word.empty()) {
        return std::nullopt;
    }
    std::string upper;
    upper.reserve(word.size());
    for (const char c : word) {
        const char u = to_upper(c);
        if (u < 'A' || u > 'Z') {
            return std::nullopt;
        }
        upper.push_back(u);
    }
    return upper;
}

[[nodiscard]] std::string_view trim(std::string_view line) noexcept {
    constexpr std::string_view blanks = " \t\r\n";
    const auto first = line.find_first_not_of(blanks);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = line.find_last_not_of(blanks);
    return line.substr(first, last - first + 1);
}

}  // namespace

std::string_view to_string(DictionaryError error) noexcept {
    switch (error) {
        case DictionaryError::cannot_open:
            return "the dictionary file could not be opened";
        case DictionaryError::read_failed:
            return "reading the dictionary failed";
    }
    return "unknown dictionary error";
}

Dictionary::Dictionary() : nodes_(1) {}

bool Dictionary::insert(std::string_view word) {
    const auto upper = normalise(word);
    if (!upper) {
        return false;
    }

    Cursor at = root;
    for (const char letter : *upper) {
        const auto& edges = nodes_[at].edges;
        const auto found = std::ranges::lower_bound(edges, letter, {}, &Edge::letter);
        if (found != edges.end() && found->letter == letter) {
            at = found->target;
            continue;
        }

        // emplace_back may reallocate nodes_, so find the edge again after it.
        const auto offset = found - edges.begin();
        const auto target = static_cast<Cursor>(nodes_.size());
        nodes_.emplace_back();
        auto& grown = nodes_[at].edges;
        grown.insert(grown.begin() + offset, Edge{letter, target});
        at = target;
    }

    if (!nodes_[at].terminal) {
        nodes_[at].terminal = true;
        ++word_count_;
    }
    return true;
}

std::expected<Dictionary, DictionaryError> Dictionary::load(std::istream& words) {
    auto zone = profiling::zone<"data.load_dictionary">();

    // A file saved by some editors starts with a UTF-8 byte-order mark; left in
    // place it would make the first word not a word.
    constexpr std::string_view byte_order_mark = "\xEF\xBB\xBF";

    Dictionary dictionary;
    std::string line;
    bool first_line = true;
    while (std::getline(words, line)) {
        std::string_view text = line;
        if (first_line && text.starts_with(byte_order_mark)) {
            text.remove_prefix(byte_order_mark.size());
        }
        first_line = false;
        dictionary.insert(trim(text));
    }
    if (words.bad()) {
        return std::unexpected(DictionaryError::read_failed);
    }
    zone.Value(dictionary.size());
    return dictionary;
}

std::expected<Dictionary, DictionaryError> Dictionary::load_file(
    const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::unexpected(DictionaryError::cannot_open);
    }
    return load(file);
}

bool Dictionary::contains(std::string_view word) const {
    const auto upper = normalise(word);
    if (!upper) {
        return false;
    }
    Cursor at = root;
    for (const char letter : *upper) {
        at = step(at, letter);
        if (at == no_node) {
            return false;
        }
    }
    return is_word(at);
}

bool Dictionary::has_prefix(std::string_view prefix) const {
    if (prefix.empty()) {
        return true;
    }
    const auto upper = normalise(prefix);
    if (!upper) {
        return false;
    }
    Cursor at = root;
    for (const char letter : *upper) {
        at = step(at, letter);
        if (at == no_node) {
            return false;
        }
    }
    return true;
}

Dictionary::Cursor Dictionary::step(Cursor from, char letter) const noexcept {
    const auto& edges = nodes_[from].edges;
    const auto found = std::ranges::lower_bound(edges, letter, {}, &Edge::letter);
    return (found != edges.end() && found->letter == letter) ? found->target : no_node;
}

bool Dictionary::is_word(Cursor at) const noexcept {
    return at != no_node && nodes_[at].terminal;
}

}  // namespace wjs
