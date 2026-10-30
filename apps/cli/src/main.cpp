// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstddef>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "sns/board.hpp"
#include "sns/cli_arguments.hpp"
#include "sns/dictionary.hpp"
#include "sns/platform/console.hpp"
#include "sns/platform/paths.hpp"
#include "sns/profiling.hpp"
#include "sns/solver.hpp"
#include "sns/version.hpp"

namespace {

// The exit codes are the program's contract (tests/system/check_cli.cmake): 0 for
// a board solved, and one code a reason for any other end, so that a script can
// tell them apart. Usage is 1; a failure the program did not foresee is not
// usage, and has a code of its own.
constexpr int exit_usage = 1;
constexpr int exit_dictionary = 2;
constexpr int exit_board = 3;
constexpr int exit_internal = 4;  // an exception the program did not foresee
constexpr int exit_output = 5;    // the result could not be written to standard output

constexpr std::string_view usage_text =
    "Usage: sns-cli [options] <row>...\n"
    "\n"
    "Finds every word on a Boggle board. Each <row> is one row of the board. A row\n"
    "with no spaces is read one letter per tile (TAPE); otherwise tiles are\n"
    "separated by spaces or commas (\"Qu I T\"), so quote such a row.\n"
    "\n"
    "Options:\n"
    "  -d, --dictionary <file>  Word list, one word per line. Defaults to\n"
    "                           dictionaries/en.txt beside the executable.\n"
    "  -m, --min-length <n>     Shortest word to report (default 3).\n"
    "  -s, --scoring <scheme>   boggle (default) or rarity.\n"
    "  -r, --repeat <n>         Solve the board n times and print one result\n"
    "                           (default 1). For timing and for profiling: a\n"
    "                           profiler needs about a second to attach.\n"
    "  -h, --help               Show this help.\n"
    "      --version            Show the version.\n";

[[nodiscard]] std::string describe_path(const std::vector<sns::Position>& path) {
    std::string text;
    for (const auto& position : path) {
        if (!text.empty()) {
            text += ' ';
        }
        text += std::format("r{}c{}", position.row + 1, position.column + 1);
    }
    return text;
}

// Brings what has been written to standard output to its destination and says
// whether it got there. The stream is flushed here, by the program, and not left
// to the flush at exit: in the Debug build (AddressSanitizer and the profiler's
// client) the exit-time flush was lost about one run in ten, so that --version
// and --help left 0 with nothing printed. A closed pipe or a full disk fails the
// stream without a word; a program whose output is the point says so and does not
// leave 0 for a result that never arrived.
[[nodiscard]] int finish_output() {
    std::cout.flush();
    if (!std::cout) {
        std::cerr << "sns-cli: the result could not be written to standard output\n";
        return exit_output;
    }
    return 0;
}

[[nodiscard]] int run(const sns::CliArguments& arguments) {
    auto zone = sns::profiling::zone<"cli.run">();

    const auto board = sns::Board::parse(arguments.board_text);
    if (!board) {
        std::cerr << std::format("sns-cli: {}\n", sns::to_string(board.error()));
        return exit_board;
    }

    const auto dictionary_path = arguments.dictionary.value_or(
        sns::platform::executable_path().parent_path() / "dictionaries" / "en.txt");
    const auto dictionary = sns::Dictionary::load_file(dictionary_path);
    if (!dictionary) {
        std::cerr << std::format("sns-cli: {}: {}\n", dictionary_path.string(),
                                 sns::to_string(dictionary.error()));
        return exit_dictionary;
    }

    // One result is printed whatever the count: the repeats are for a clock
    // or a profiler (docs/profiling.md), not for the person reading the list.
    auto words = sns::solve(*board, *dictionary, arguments.options);
    for (std::size_t again = 1; again < arguments.repeat; ++again) {
        words = sns::solve(*board, *dictionary, arguments.options);
    }

    int total = 0;
    for (const auto& found : words) {
        total += found.score;
        std::cout << std::format("{:<16}{:>5}   {}\n", found.word, found.score,
                                 describe_path(found.path));
    }
    std::cout << std::format("{} words, {} points\n", words.size(), total);
    return finish_output();
}

int run_cli(int argc, char** argv) {
    sns::platform::use_utf8_console();
    sns::profiling::thread_name("main");

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    const auto arguments = sns::parse_cli_arguments(args);
    if (!arguments) {
        std::cerr << std::format("sns-cli: {}\n\n{}", arguments.error(), usage_text);
        return exit_usage;
    }
    if (arguments->version) {
        std::cout << std::format("sns-cli {} ({})\n", sns::project_version, sns::target_platform);
        return finish_output();
    }
    if (arguments->help) {
        std::cout << usage_text;
        return finish_output();
    }
    if (arguments->board_text.empty()) {
        std::cerr << usage_text;
        return exit_usage;
    }
    return run(*arguments);
}

}  // namespace

// The catch-all below is the handler; the check does not model one in main.
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char** argv) {
    // std::format and the standard library can throw (allocation, a bad format
    // string): a command-line program reports it and exits, rather than
    // terminating through std::terminate.
    try {
        return run_cli(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "sns-cli: " << error.what() << '\n';
        return exit_internal;
    } catch (...) {
        std::cerr << "sns-cli: unknown error\n";
        return exit_internal;
    }
}
