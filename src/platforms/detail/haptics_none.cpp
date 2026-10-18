#include "wjs/platform/haptics.hpp"

// The desktop haptics: there is no haptic engine assumed on Windows, macOS or
// Linux, so every pattern is answered false and the game carries on with its
// sound and its picture. The tablets' backends are named in the interface
// (wjs/platform/haptics.hpp) and wait for those targets.

namespace wjs::platform {

bool vibrate(HapticPattern) {
    return false;
}

}  // namespace wjs::platform
