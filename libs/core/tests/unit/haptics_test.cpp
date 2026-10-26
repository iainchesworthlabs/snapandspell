// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>

#include "sns/platform/haptics.hpp"

// The haptics interface, as the three desktops answer it
// (libs/platforms/detail/haptics_none.cpp): every pattern is a call the platform
// accepts, it never blocks, and there is no haptic engine, so every answer is
// false and the game carries on with its sound and its picture.
//
// A tablet target with a real backend moves this case to tests/platform and
// asserts the device's own answer; this one is about the interface existing and
// being safe where there is nothing to buzz.

TEST_CASE("every haptic pattern is answered, and a desktop has no engine", "[platform][haptics]") {
    using sns::platform::HapticPattern;
    using sns::platform::vibrate;

    // Twice each, because a cue may arrive at any time and a second call must
    // be as harmless as the first.
    for (const HapticPattern pattern :
         {HapticPattern::light, HapticPattern::medium, HapticPattern::double_tap}) {
        CHECK_FALSE(vibrate(pattern));
        CHECK_FALSE(vibrate(pattern));
    }
}
