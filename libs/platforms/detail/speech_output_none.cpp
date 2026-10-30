// SPDX-License-Identifier: GPL-3.0-or-later
#include "sns/platform/speech_output.hpp"

// A platform with no voice of its own to offer. The other platforms speak through Qt TextToSpeech
// where the kit has it (apps/app/src/variants/speech-texttospeech), so this interface is only a way
// in for the browser's build; here there is no voice and every request is a harmless nothing.

namespace sns::platform {

bool speech_output_available() {
    return false;
}

void speak([[maybe_unused]] std::string_view utf8, [[maybe_unused]] double volume) {}

void stop_speaking() {}

}  // namespace sns::platform
