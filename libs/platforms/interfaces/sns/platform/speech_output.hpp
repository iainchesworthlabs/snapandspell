// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string_view>

// A platform's own voice, for a build whose Qt has no TextToSpeech (the browser's). Where the kit has
// Qt TextToSpeech the application uses that (apps/app/src/variants/speech-texttospeech); where it has
// not, the speech variant forwards here. There is exactly one definition per platform, named in its
// platform.cmake:
//
//   - the web: the browser's speech synthesis (libs/platforms/web/speech_output_web.cpp), through a
//     voice that runs on the device only: some browsers list voices that send the text to a server,
//     and the program sends nothing anywhere;
//   - the others: detail/speech_output_none.cpp, which has no voice and answers false.
//
// The word said is also on the screen, so a platform with no voice loses nothing.

namespace sns::platform {

// Whether there is a voice to say a word with, at this moment.
[[nodiscard]] bool speech_output_available();

// Says `utf8` (a word or a sentence) at `volume` (0 to 1), in place of whatever is being said. Never
// blocks, says nothing where there is no voice, and is safe to call at any time.
void speak(std::string_view utf8, double volume);

// Stops whatever is being said. Safe when nothing is.
void stop_speaking();

}  // namespace sns::platform
