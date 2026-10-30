// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/platform/audio_output.hpp"

// A platform with no sound output of its own to offer. The desktops, iOS and Android play their cues
// through Qt Multimedia where the kit has it (apps/app/src/variants/sound-multimedia), so this
// interface is only a way in for the browser's build; here there is nothing to play and the answer
// is always "no output".

namespace sns::platform {

bool audio_output_available() {
    return false;
}

void play_wav([[maybe_unused]] std::string_view name, [[maybe_unused]] std::span<const std::byte> wav,
              [[maybe_unused]] double volume) {}

}  // namespace sns::platform
