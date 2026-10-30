// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <span>
#include <string_view>

// A platform's own way to play a short sound, for a build whose Qt has no Multimedia (the
// browser's: Qt for WebAssembly). The application plays its cues through Qt Multimedia where the
// kit has it (apps/app/src/variants/sound-multimedia); where it has not, the sound variant forwards
// here, so the code that reads the files and the cues' policy are the same on every platform and
// only this call differs. There is exactly one definition per platform, named in its
// platform.cmake:
//
//   - the web: Web Audio (libs/platforms/web/audio_output_web.cpp);
//   - the others: detail/audio_output_none.cpp, which has no output and answers false.
//
// A cue is punctuation and never the only way something is said: every sound is paired with a
// picture (docs/design/application.md, "Audio and touch feedback"), so an answer of "no output" is
// harmless.

namespace sns::platform {

// Whether this platform can play a sound of its own at all, at this moment.
[[nodiscard]] bool audio_output_available();

// Plays the WAV file whose bytes are `wav` at `volume` (0 to 1). `name` identifies the sound, so a
// platform that decodes it can keep the result for next time. Never blocks, plays nothing where
// there is no output, and is safe to call at any time.
void play_wav(std::string_view name, std::span<const std::byte> wav, double volume);

}  // namespace sns::platform
