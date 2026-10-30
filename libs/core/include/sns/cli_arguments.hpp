// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

#include "sns/solver.hpp"

namespace sns {

// What the command line of sns-cli asks for. It is read here, in the library, and not in the
// program's main, so that a unit test and a fuzzer can reach it (docs/fuzzing.md).
struct CliArguments {
    std::optional<std::filesystem::path> dictionary;
    SolveOptions options;
    std::size_t repeat = 1;
    // The rows of the board, one per line.
    std::string board_text;
    bool help = false;
    bool version = false;
};

// The arguments after the program's name. The error is the sentence the program prints after
// "sns-cli: ": it names the option or the value that was wrong.
[[nodiscard]] std::expected<CliArguments, std::string> parse_cli_arguments(
    std::span<const std::string> args);

}  // namespace sns
