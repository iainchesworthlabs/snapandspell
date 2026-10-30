// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "fuzz_input.hpp"
#include "sns/cli_arguments.hpp"

// The command line of sns-cli. The input is the arguments, one to a line, so that a mutated file is
// a different command line. A line that is not a right command line is an error with a sentence to
// print; one that is has a repeat count of at least one, and a board of as many rows as it had
// words that were not options or their values.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::vector<std::string> args;
    std::string_view rest = sns::test::fuzz_text(data, size);
    while (!rest.empty()) {
        const auto end = rest.find('\n');
        args.emplace_back(rest.substr(0, end));
        rest = end == std::string_view::npos ? std::string_view{} : rest.substr(end + 1);
    }

    const auto parsed = sns::parse_cli_arguments(args);
    if (!parsed) {
        if (parsed.error().empty()) {
            std::abort();
        }
        return 0;
    }
    if (parsed->repeat == 0) {
        std::abort();
    }
    if (!parsed->board_text.empty() && parsed->board_text.back() != '\n') {
        std::abort();
    }
    return 0;
}
