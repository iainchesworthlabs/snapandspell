// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/platform/haptics.hpp"

// The desktop haptics: there is no haptic engine assumed on Windows, macOS or
// Linux, so every pattern is answered false and the game carries on with its
// sound and its picture. The tablets' backends are named in the interface
// (sns/platform/haptics.hpp) and wait for those targets.

namespace sns::platform {

bool vibrate([[maybe_unused]] HapticPattern pattern) {
    return false;
}

}  // namespace sns::platform
