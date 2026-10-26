// SPDX-License-Identifier: GPL-3.0-or-later
#include <emscripten.h>

#include "wjs/platform/haptics.hpp"

// Emscripten's EM_ASM names its arguments $0, $1 ...; clang calls a dollar in an identifier an
// extension and the project's warnings are errors.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"

namespace wjs::platform {

// The browser's Vibration API, which Chrome on Android has and Safari and the desktops do not:
// where it is missing, or the browser refuses (it wants a tap first, and a page in the
// background gets none), navigator.vibrate answers false, and so does this. The patterns are
// the short ones of the design's feedback table: a tap, a firmer tap, two taps.
bool vibrate(HapticPattern pattern) {
    const int index = static_cast<int>(pattern);
    // clang-format off
    return EM_ASM_INT({
        // Array.of, not literals: the preprocessor splits macro arguments at every comma that is
        // not inside parentheses.
        const patterns = Array.of(Array.of(12), Array.of(24), Array.of(14, 40, 14));
        return (typeof navigator !== 'undefined' && navigator.vibrate &&
                navigator.vibrate(patterns[$0])) ? 1 : 0;
    }, index) != 0;
    // clang-format on
}

}  // namespace wjs::platform

#pragma clang diagnostic pop
