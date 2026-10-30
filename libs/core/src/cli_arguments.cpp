// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/cli_arguments.hpp"

#include <charconv>
#include <format>
#include <string_view>
#include <system_error>

#include "sns/scoring.hpp"

namespace sns {

std::expected<CliArguments, std::string> parse_cli_arguments(std::span<const std::string> args) {
    CliArguments parsed;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        // The value after an option that takes one.
        const auto value = [&] -> std::expected<std::string_view, std::string> {
            if (i + 1 >= args.size()) {
                return std::unexpected(std::format("{} needs a value", arg));
            }
            return std::string_view(args[++i]);
        };

        if (arg == "-h" || arg == "--help") {
            parsed.help = true;
        } else if (arg == "--version") {
            parsed.version = true;
        } else if (arg == "-d" || arg == "--dictionary") {
            const auto path = value();
            if (!path) {
                return std::unexpected(path.error());
            }
            parsed.dictionary = std::filesystem::path{std::string(*path)};
        } else if (arg == "-m" || arg == "--min-length") {
            const auto text = value();
            if (!text) {
                return std::unexpected(text.error());
            }
            const char* const first = text->data();
            const char* const last = first + text->size();
            const auto [end, error] = std::from_chars(first, last, parsed.options.min_length);
            if (error != std::errc{} || end != last) {
                return std::unexpected(std::format("'{}' is not a length", *text));
            }
        } else if (arg == "-r" || arg == "--repeat") {
            const auto text = value();
            if (!text) {
                return std::unexpected(text.error());
            }
            const char* const first = text->data();
            const char* const last = first + text->size();
            const auto [end, error] = std::from_chars(first, last, parsed.repeat);
            if (error != std::errc{} || end != last || parsed.repeat == 0) {
                return std::unexpected(std::format("'{}' is not a count of one or more", *text));
            }
        } else if (arg == "-s" || arg == "--scoring") {
            const auto scheme = value();
            if (!scheme) {
                return std::unexpected(scheme.error());
            }
            if (*scheme == "boggle") {
                parsed.options.scoring = ScoringScheme::standard_boggle;
            } else if (*scheme == "rarity") {
                parsed.options.scoring = ScoringScheme::letter_rarity;
            } else {
                return std::unexpected(
                    std::format("'{}' is not a scoring scheme (boggle or rarity)", *scheme));
            }
        } else if (arg.size() > 1 && arg.front() == '-') {
            return std::unexpected(std::format("unknown option '{}'", arg));
        } else {
            parsed.board_text += arg;
            parsed.board_text += '\n';
        }
    }
    return parsed;
}

}  // namespace sns
