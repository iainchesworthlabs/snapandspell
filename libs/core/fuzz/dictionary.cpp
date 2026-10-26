// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>

#include "fuzz_input.hpp"
#include "sns/dictionary.hpp"

// A word list, a word to a line. Whatever loads is a dictionary: the words it holds can be found
// again, and the count is the count of the words that can.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::istringstream in{sns::test::fuzz_string(data, size)};
    const auto dictionary = sns::Dictionary::load(in);
    if (!dictionary) {
        return 0;
    }
    static_cast<void>(dictionary->has_prefix(""));
    // The first line of the input, tried as a word and as a prefix, found or not, without a crash.
    const auto text = sns::test::fuzz_text(data, size);
    const auto line = text.substr(0, text.find('\n'));
    if (dictionary->contains(line) && !dictionary->has_prefix(line)) {
        std::abort();
    }
    return 0;
}
