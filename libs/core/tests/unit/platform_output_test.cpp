// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <span>

#include "sns/platform/audio_output.hpp"
#include "sns/platform/speech_output.hpp"

// The sound and the voice a platform offers where Qt has none (libs/platforms/detail/*_output_none.cpp,
// on every platform but the browser): there is no output and no voice, and a request for either is a
// harmless nothing, so the build that uses them is a working game with no sound.

TEST_CASE("a platform with no output of its own plays nothing, harmlessly", "[platform][audio]") {
    CHECK_FALSE(sns::platform::audio_output_available());
    const std::byte bytes[4] = {};
    // Twice, and with nothing at all: a cue may arrive at any time.
    sns::platform::play_wav("rattle", std::span<const std::byte>(bytes), 1.0);
    sns::platform::play_wav("rattle", std::span<const std::byte>(bytes), 0.5);
    sns::platform::play_wav("", std::span<const std::byte>(), 0.0);
    CHECK_FALSE(sns::platform::audio_output_available());
}

TEST_CASE("a platform with no voice of its own says nothing, harmlessly", "[platform][speech]") {
    CHECK_FALSE(sns::platform::speech_output_available());
    sns::platform::speak("cat", 1.0);
    sns::platform::speak("", 1.0);
    sns::platform::stop_speaking();
    sns::platform::stop_speaking();
    CHECK_FALSE(sns::platform::speech_output_available());
}
