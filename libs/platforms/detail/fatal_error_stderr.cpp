// SPDX-License-Identifier: GPL-3.0-or-later
#include <iostream>

#include "sns/platform/fatal_error.hpp"

namespace sns::platform {

// A program started from a terminal or a launcher that keeps the error stream is
// told there; no desktop that uses this file has a dialog for it that every
// session can show.
void show_fatal_error(std::string_view title, std::string_view message,
                      [[maybe_unused]] bool interactive) {
    std::cerr << title << ": " << message << '\n';
    std::cerr.flush();
}

}  // namespace sns::platform
