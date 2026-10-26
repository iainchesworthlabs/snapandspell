// SPDX-License-Identifier: GPL-3.0-or-later
#include <emscripten.h>

#include <iostream>
#include <string>
#include <string_view>

#include "sns/platform/fatal_error.hpp"

// Emscripten's EM_ASM names its arguments $0, $1 ...; clang calls a dollar in an identifier an
// extension and the project's warnings are errors.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

namespace sns::platform {

// A page has no console a person looks at: the message goes to the browser's console, where a
// bug report can quote it, and into the page's status line (sns-boot.js's #status, which is
// what the person sees in place of the game), as text and never as markup. There is no dialog:
// alert() blocks the page and a program that cannot start has nothing else to show.
void show_fatal_error(std::string_view title, std::string_view message,
                      [[maybe_unused]] bool interactive) {
    const std::string text = std::string{title} + ": " + std::string{message};
    std::cerr << text << '\n';
    std::cerr.flush();

    // clang-format off
    EM_ASM({
        const text = UTF8ToString($0);
        console.error(text);
        const status = typeof document !== 'undefined' ? document.getElementById('status') : null;
        if (status) {
            status.textContent = text;
            document.body.classList.add('failed');
        }
    }, text.c_str());
    // clang-format on
}

}  // namespace sns::platform

#pragma clang diagnostic pop
